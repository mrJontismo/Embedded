#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/uart.h"

#define I2C1_SDA_PIN 14
#define I2C1_SCL_PIN 15
#define I2C_ID i2c1

#define UART_ID uart0

#define DEV_ADDR 0x50
#define HIGHEST_ADDR (0x7FFF)
#define HIGHEST_STR_ADDR (0x800)
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

void led_set_state(LED_State *state, uint8_t value)
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
    sleep_ms(5);
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
    sleep_ms(5);
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

void eeprom_store_led_state_from_struct(LED_State *leds)
{
    uint16_t highest_address = HIGHEST_ADDR - sizeof(LED_State) * 3 + 1;

    if(highest_address != 0) {
        eeprom_write_multi_byte(highest_address, (uint8_t *)leds, sizeof(LED_State) * 3);
    }
}

bool eeprom_leds_are_initialized(void)
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

void led_read_state_to_struct(LED_State *leds)
{
    LED_State eeprom_leds[3];

    eeprom_read_multi_byte(HIGHEST_ADDR - sizeof(LED_State) * 3 + 1, (uint8_t *)eeprom_leds, sizeof(LED_State) * 3);

    for(uint8_t i = 0; i < 3; i++) {
        if(led_state_is_valid(&eeprom_leds[i])) {
            led_set_state(&leds[i], eeprom_leds[i].state);
        }
    }
}

void led_apply_state(LED_State *leds)
{
    for(uint8_t i = 0; i < 3; i++) {
        if(led_state_is_valid(&leds[i])) {
            if(leds[i].state == LED_ON) {
                gpio_put(leds[i].pin, 1);
            } else {
                gpio_put(leds[i].pin, 0);
            }
        }
    }
}

void led_turn_on(LED_State *leds, uint8_t index)
{
    bool led_current_state = leds[index].state;

    if(led_current_state == LED_ON) {
        led_set_state(&leds[index], LED_OFF);
        eeprom_store_led_state_from_struct(leds);
        led_apply_state(leds);
    } else {
        led_set_state(&leds[index], LED_ON);
        eeprom_store_led_state_from_struct(leds);
        led_apply_state(leds);
    }
}

uint16_t eeprom_find_lowest_free_str_address(void) {
    for(uint16_t address = 0; address < HIGHEST_STR_ADDR; address += 64) {
        if(eeprom_read_byte(address) == MAGIC_BYTE) {
            return address;
        }
    }
    printf("ERROR: EEPROM is full.\n");
    return HIGHEST_STR_ADDR;
}

uint16_t crc16(const uint8_t *data_p, size_t length) {
    uint8_t x;
    uint16_t crc = 0xFFFF;
    while(length--) {
        x = crc >> 8 ^ *data_p++;
        x ^= x >> 4;
        crc = (crc << 8) ^ ((uint16_t) (x << 12)) ^ ((uint16_t) (x << 5)) ^ ((uint16_t) x);
    }
    return crc;
}

void eeprom_clear_str_space(void)
{
    printf("Clearing EEPROM log entries.\n");
    for(uint16_t address = 0; address < HIGHEST_STR_ADDR + 1; address += PAGE_SIZE) {
        uint8_t buffer[PAGE_SIZE];
        memset(buffer, MAGIC_BYTE, PAGE_SIZE);
        eeprom_write_multi_byte(address, buffer, PAGE_SIZE);
    }
    printf("EEPROM clear.\n");
}

void eeprom_write_string(char *data)
{
    size_t len = strlen(data);
    if(len > 61) {
        printf("Log entry is too long.\n");
        return;
    }

    uint8_t buf[64];
    uint16_t crc = crc16((uint8_t *)data, len);

    memcpy(buf, data, len + 1);
    buf[len] = '\0';
    buf[len + 1] = (crc >> 8);
    buf[len + 2] = crc;

    uint16_t address = eeprom_find_lowest_free_str_address();
    if(address + 64 >= HIGHEST_STR_ADDR) {
        printf("Too many log entries.\n");
        eeprom_clear_str_space();

        address = eeprom_find_lowest_free_str_address();
    }

    eeprom_write_multi_byte(address, buf, len + 3);
}

void led_state_print_and_store(LED_State *leds, uint8_t index)
{
    char log_message[64];
    sprintf(log_message, "LED_%d status: %s. State changed at: %llu seconds.\n", index, leds[index].state == LED_ON ? "on" : "off", time_us_64() / 1000000);
    printf("%s", log_message);
    eeprom_write_string(log_message);
}

void eeprom_read_string(void)
{
    printf("-----READING LOG-----\n");

    for(uint16_t address = 0; address < HIGHEST_STR_ADDR; address += 64) {
        if(eeprom_read_byte(address) != MAGIC_BYTE) {
            uint8_t buf[64];
            eeprom_read_multi_byte(address, buf, 64);

            size_t len = strlen(buf);

            uint16_t crc = crc16(buf, len);
            buf[len] = (uint8_t) (crc >> 8);
            buf[len + 1] = (uint8_t) crc;

            if(crc16(buf, len + 2) == 0) {
                buf[len] = '\0';
                printf("%s", buf);
            } else {
                printf("Invalid CRC checksum.\n");
                return;
            }
        }
    }
    printf("-----REACHED END OF LOG-----\n");
}

int main(void)
{
    stdio_init_all();

    const char *boot_msg = "Booting...\n";
    printf("%s", boot_msg);

    init_gpio();

    i2c_init(i2c1, 100000);
    gpio_set_function(I2C1_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C1_SCL_PIN, GPIO_FUNC_I2C);

    eeprom_write_string(boot_msg);

    LED_State leds[3] = {
            {LED_0, LED_OFF, ~LED_OFF},
            {LED_1, LED_ON, ~LED_ON},
            {LED_2, LED_OFF, ~LED_OFF},
    };

    if(!eeprom_leds_are_initialized()) {
        eeprom_store_led_state_from_struct(leds);
        led_apply_state(leds);
    } else {
        led_read_state_to_struct(leds);
        led_apply_state(leds);
    }

    printf("Program started at %llu seconds.\n", time_us_64() / 1000000);

    char buf[20];
    uint8_t index = 0;

    while(true) {

        while(uart_is_readable(UART_ID)) {
            char input_char = uart_getc(UART_ID);
            if(input_char == '\r') {
                buf[index] = '\0';
                if(strcmp(buf, "read") == 0) {
                    eeprom_read_string();
                } else if(strcmp(buf, "erase") == 0) {
                    eeprom_clear_str_space();
                }
                index = 0;
            } else {
                buf[index] = input_char;
                index++;
            }
            sleep_ms(100);
        }

        if(!gpio_get(SW_0)) {
            while(!gpio_get(SW_0)) {
                sleep_ms(50);
            }
            led_turn_on(leds, 0);
            led_state_print_and_store(leds, 0);
        }

        if(!gpio_get(SW_1)) {
            while(!gpio_get(SW_1)) {
                sleep_ms(50);
            }
            led_turn_on(leds, 1);
            led_state_print_and_store(leds, 1);
        }

        if(!gpio_get(SW_2)) {
            while(!gpio_get(SW_2)) {
                sleep_ms(50);
            }
            led_turn_on(leds, 2);
            led_state_print_and_store(leds, 2);
        }
    }

    return 0;
}