#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#define I2C1_SDA_PIN 14
#define I2C1_SCL_PIN 15
#define I2C_ID i2c1

#define DEV_ADDR 0x50
#define HIGHEST_ADDR (0x7FFF)
#define PAGE_SIZE 64

#define MAGIC_BYTE 0xA5
#define LED_ON 0x01
#define LED_OFF 0x00

#define SW_0 9
#define SW_1 8
#define SW_2 7

#define LED_0 20
#define LED_1 21
#define LED_2 22

typedef struct LED_State {
    uint8_t pin;
    uint8_t state;
    uint8_t not_state;
} LED_State;

void set_led_state(LED_State *state, uint8_t value)
{
    state->state = value;
    state->not_state = ~value;
}

bool led_state_is_valid(LED_State *state)
{
    return state->state == (uint8_t) ~state->not_state;
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

void eeprom_clear(void)
{
    for(uint16_t address = 0; address < HIGHEST_ADDR + 1; address += PAGE_SIZE) {
        uint8_t buffer[PAGE_SIZE];
        memset(buffer, MAGIC_BYTE, PAGE_SIZE);
        eeprom_write_multi_byte(address, buffer, PAGE_SIZE);
        printf("Cleared address: %02X\n", address);
    }
}

void eeprom_store_led_state(LED_State *leds)
{
    uint16_t highest_address = HIGHEST_ADDR - sizeof(LED_State) * 3 + 1;

    if(highest_address != 0) {
        eeprom_write_multi_byte(highest_address, (uint8_t *)leds, sizeof(LED_State) * 3);
    }
}

bool eeprom_leds_initialized(void)
{
    uint16_t first_address = HIGHEST_ADDR - sizeof(LED_State) * 3 + 1;

    for(uint16_t address = first_address; address <= HIGHEST_ADDR; address++) {
        uint8_t value = eeprom_read_byte(first_address);
        if(value == MAGIC_BYTE) {
            return false;
        }
    }
    return true;
}

void eeprom_check_contents(void)
{
    for(uint16_t address = 0; address <= HIGHEST_ADDR; address++) {
        uint8_t value = eeprom_read_byte(address);
        if(value != MAGIC_BYTE) {
            printf("Address 0x%04X: Value 0x%02X\n", address, value);
        }
    }
    printf("End of EEPROM.\n");
}

int main(void)
{
    stdio_init_all();
    printf("Booting...\n");

    init_gpio();

    i2c_init(i2c1, 100000);
    gpio_set_function(I2C1_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C1_SCL_PIN, GPIO_FUNC_I2C);

    LED_State leds[3] = {
            {LED_0, LED_OFF, ~LED_OFF},
            {LED_1, LED_OFF, ~LED_OFF},
            {LED_2, LED_OFF, ~LED_OFF},
    };

    eeprom_check_contents();
    return 0;
}
/*uint16_t eeprom_find_highest_free_address(void)
{
    for(uint16_t address = HIGHEST_ADDR; address > 0; address--) {
        if(eeprom_read_byte(address) == MAGIC_BYTE) {
            return address;
        }
    }
    printf("ERROR: EEPROM is full.\n");
    return 0;
}

uint16_t eeprom_find_lowest_free_address(void)
{
    for(uint16_t address = 0; address < HIGHEST_ADDR; address++) {
        if(eeprom_read_byte(address) == MAGIC_BYTE) {
            return address;
        }
    }
    printf("ERROR: EEPROM is full.\n");
    return 0;
}*/
