#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#define I2C1_SDA_PIN 14
#define I2C1_SCL_PIN 15
#define I2C_ID i2c1

#define DEV_ADDR 0x50

#define SW_0 9
#define SW_1 8
#define SW_2 7

#define LED_0 20
#define LED_1 21
#define LED_2 22

typedef struct LED_State {
    uint8_t state;
    uint8_t not_state;
} LED_State;

void set_led_state(LED_State *state, uint8_t value)
{
    state->state = value;
    state->not_state = ~value;
}

bool led_state_is_valid(LED_State *ls)
{
    return ls->state == (uint8_t) ~ls->not_state;
}

void eeprom_write_byte(uint16_t address, uint8_t byte)
{
    uint8_t first = (address >> 8) & 0xFF;
    uint8_t second = address & 0xFF;

    uint8_t frame[] = { first, second, byte };
    i2c_write_blocking(I2C_ID, DEV_ADDR, frame, 3, false);
    sleep_ms(10);
}

uint8_t eeprom_read_byte(uint16_t address)
{
    uint8_t first = (address >> 8) & 0xFF;
    uint8_t second = address & 0xFF;

    uint8_t addr[] = { first, second };

    i2c_write_blocking(I2C_ID, DEV_ADDR, addr, 2, true);
    uint8_t buf;
    i2c_read_blocking(I2C_ID, DEV_ADDR, &buf, 1, false);
    return buf;
}

void eeprom_write_multi_byte(uint16_t address, uint8_t *data, size_t len)
{
    uint8_t first = (address >> 8) & 0xFF;
    uint8_t second = address & 0xFF;

    uint8_t frame[len + 2];
    frame[0] = first;
    frame[1] = second;
    memcpy(&frame[2], data, len);
    i2c_write_blocking(I2C_ID, DEV_ADDR, frame, len + 2, false);
    sleep_ms(10);
}

void eeprom_read_multi_byte(uint16_t address, uint8_t *buffer, size_t len)
{
    uint8_t first = (address >> 8) & 0xFF;
    uint8_t second = address & 0xFF;

    uint8_t addr[] = { first, second };

    i2c_write_blocking(I2C_ID, DEV_ADDR, addr, 2, true);
    i2c_read_blocking(I2C_ID, DEV_ADDR, buffer, len, false);
}

void init_gpio(void)
{
    gpio_init(LED_0);
    gpio_set_dir(LED_0, GPIO_OUT);

    gpio_init(LED_1);
    gpio_set_dir(LED_1, GPIO_OUT);
    
    gpio_init(LED_2);
    gpio_set_dir(LED_2, GPIO_OUT);

    gpio_init(SW_0);
    gpio_set_dir(SW_0, GPIO_IN);
    gpio_pull_up(SW_0);

    gpio_init(SW_1);
    gpio_set_dir(SW_1, GPIO_IN);
    gpio_pull_up(SW_1);

    gpio_init(SW_2);
    gpio_set_dir(SW_2, GPIO_IN);
    gpio_pull_up(SW_2);
}

int main(void)
{
    stdio_init_all();
    printf("Booting...\n");

    init_gpio();

    i2c_init(i2c1, 100000);
    gpio_set_function(I2C1_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C1_SCL_PIN, GPIO_FUNC_I2C);

    while(true) {
        if(!gpio_get(SW_0)) {
            while(!gpio_get(SW_0)) {
                sleep_ms(50);
            }
            gpio_put(LED_0, 1);
        }
        if(!gpio_get(SW_1)) {
            while(!gpio_get(SW_1)) {
                sleep_ms(50);
            }
            gpio_put(LED_1, 1);
        }
        if(!gpio_get(SW_2)) {
            while(!gpio_get(SW_2)) {
                sleep_ms(50);
            }
            gpio_put(LED_2, 1);
        }
    }

    return 0;
}