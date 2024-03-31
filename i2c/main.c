#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#define I2C1_SDA_PIN 14
#define I2C1_SCL_PIN 15

#define DEVADDR 0x20

int main() {

    const uint led_pin = 22;

    // Initialize LED pin
    gpio_init(led_pin);
    gpio_set_dir(led_pin, GPIO_OUT);

    // Initialize chosen serial port
    stdio_init_all();

    i2c_init(i2c1, 100000);
    gpio_set_function(I2C1_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C1_SCL_PIN, GPIO_FUNC_I2C);
    uint8_t buf[8] = { 0x00, 0x20 };
    i2c_write_blocking(i2c1, DEVADDR, buf, 2, false);
    buf[0] = 0x12;
    buf[1] = 0x01;
    i2c_write_blocking(i2c1, DEVADDR, buf, 2, false);

    buf[0] = 0x0C;
    buf[1] = 0x20;
    i2c_write_blocking(i2c1, DEVADDR, buf, 2, false);


    // Loop forever
    while (true) {
        uint8_t value;
        buf[0] = 0x12;
        buf[1] = 0x81;
        i2c_write_blocking(i2c1, DEVADDR, buf, 1, true);
        i2c_read_blocking(i2c1, DEVADDR, &value, 1, false);

        // Blink LED
        printf("value: %02X\r\n", value & 0x20);
        gpio_put(led_pin, true);
        buf[0] = 0x12;
        buf[1] = 0x81;
        i2c_write_blocking(i2c1, DEVADDR, buf, 2, false);
        sleep_ms(500);
        gpio_put(led_pin, false);
        buf[0] = 0x12;
        buf[1] = 0x80;
        i2c_write_blocking(i2c1, DEVADDR, buf, 2, false);
        sleep_ms(500);
    }
}
