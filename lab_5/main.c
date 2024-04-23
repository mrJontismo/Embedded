#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"

#define IN1 2
#define IN2 3
#define IN3 6
#define IN4 13

#define OPTO_FORK 28

#define UART_ID uart0

void init_gpio(void)
{
    gpio_init(OPTO_FORK);
    gpio_set_dir(OPTO_FORK, GPIO_IN);
    gpio_pull_up(OPTO_FORK);

    gpio_init(IN1);
    gpio_set_dir(IN1, GPIO_OUT);

    gpio_init(IN2);
    gpio_set_dir(IN2, GPIO_OUT);

    gpio_init(IN3);
    gpio_set_dir(IN3, GPIO_OUT);

    gpio_init(IN4);
    gpio_set_dir(IN4, GPIO_OUT);
}

void turn_off(void)
{
    gpio_put(IN1, 0);
    gpio_put(IN2, 0);
    gpio_put(IN3, 0);
    gpio_put(IN4, 0);
    sleep_ms(1);
}

void go_to_opto_fork(uint8_t (*steps)[4], uint8_t step_count)
{
    bool opto_fork_state = gpio_get(OPTO_FORK);
    bool last_opto_fork_state = opto_fork_state;

    while(true) {
        opto_fork_state = gpio_get(OPTO_FORK);
        if(!opto_fork_state && last_opto_fork_state) {
            break;
        }

        for(uint32_t i = 0; i < step_count; i++) {
            gpio_put(IN1, steps[i][0]);
            gpio_put(IN2, steps[i][1]);
            gpio_put(IN3, steps[i][2]);
            gpio_put(IN4, steps[i][3]);
            sleep_ms(1);
        }

        last_opto_fork_state = opto_fork_state;
    }

    turn_off();
}
void calib(uint8_t (*steps)[4], uint8_t step_count)
{
    go_to_opto_fork(steps, step_count);
}

int main(void)
{
    stdio_init_all();
    printf("Booting...\n");

    init_gpio();

    uint8_t steps[][4] = {
            {1, 0, 0, 0},
            {1, 1, 0, 0},
            {0, 1, 0, 0},
            {0, 1, 1, 0},
            {0, 0, 1, 0},
            {0, 0, 1, 1},
            {0, 0, 0, 1},
            {1, 0, 0, 1}
    };

    uint8_t step_count = sizeof(steps) / sizeof(steps[0]);

    char buf[20];
    uint8_t index = 0;

    while(true) {
        while(uart_is_readable(UART_ID)) {
            char input_char = uart_getc(UART_ID);
            if(input_char == '\r') {
                buf[index] = '\0';
                if(strcmp(buf, "status") == 0) {
                    printf("status\n");
                } else if(strcmp(buf, "calib") == 0) {
                    calib(steps, step_count);
                }
                index = 0;
            } else {
                buf[index] = input_char;
                index++;
            }
            sleep_ms(100);
        }
    }
    return 0;
}