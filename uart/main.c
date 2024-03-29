#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/uart.h"

#define SW_0 9

#define UART_ID uart1
#define BAUD_RATE 9600
#define UART_TX_PIN 4
#define UART_RX_PIN 5

#define DEBOUNCE_DELAY_MS 100

int main(void)
{
    stdio_init_all();
    printf("Booting...\n");

    gpio_init(SW_0);
    gpio_set_dir(SW_0, GPIO_IN);
    gpio_pull_up(SW_0);

    uart_init(UART_ID, BAUD_RATE);

    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    const char send[] = "AT\n";
    char str[80];
    int pos = 0;

    while(true) {
        if(!gpio_get(SW_0)) {
            while(!gpio_get(SW_0)) {
                sleep_ms(50);
            }
            uart_write_blocking(UART_ID, send, strlen(send));
        }
        while(uart_is_readable(UART_ID)) {
            char c = uart_getc(UART_ID);
            if(c == '\r' || c == '\n') {
                str[pos] = '\0';
                printf("received: %s\n", str);
                pos = 0;
            } else {
                if(pos < 80 - 1) {
                    str[pos++] = c;
                }
            }
        }
    }

    return 0;
}
