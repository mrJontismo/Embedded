#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/uart.h"

#define SW_0 9

#define UART_ID uart1
#define BAUD_RATE 9600
#define UART_TX_PIN 4
#define UART_RX_PIN 5

#define STR_LEN (80)
#define LORA_ID_LEN (16)
#define UART_TIMEOUT_US 500000

typedef enum LoRaState {
    STATE_IDLE,
    STATE_CONNECTING,
    STATE_READING_FIRMWARE,
    STATE_READING_DEVEUI,
    STATE_ERROR
} LoRaState;

char *uart_read_string_with_timeout(uart_inst_t *uart, const char *desired_output, uint8_t max_attempts) {
    uint32_t t = 0;
    uint8_t pos = 0;
    uint8_t attempt = 0;

    char *str = (char *) malloc(STR_LEN * sizeof(char));
    if(str == NULL) {
        return NULL;
    }

    for(attempt = 0; attempt < max_attempts; attempt++) {
        pos = 0;
        t = time_us_32();

        do {
            if(uart_is_readable(uart)) {
                char c = uart_getc(uart);
                if(c == '\r' || c == '\n') {
                    str[pos] = '\0';
                    if(strstr(str, desired_output) != NULL) {
                        return str;
                    }
                    break;
                } else {
                    if(pos < STR_LEN - 1) {
                        str[pos++] = c;
                    }
                }
            }
        } while((time_us_32() - t) <= UART_TIMEOUT_US);

        if(attempt == max_attempts - 1) {
            break;
        }
    }

    free(str);
    return NULL;
}


bool lora_test_uart_connection(uart_inst_t *uart)
{
    const char *send = "AT\r\n";

    uart_write_blocking(uart, (uint8_t *) send, strlen(send));
    char *received_string = uart_read_string_with_timeout(uart, "OK", 5);
    if(received_string != NULL) {
        printf("Connected to LoRa module.\n");;
        free(received_string);
        return true;
    } else {
        return false;
    }
}

bool lora_get_fw_version(uart_inst_t *uart)
{
    const char *send = "AT+VER\r\n";

    uart_write_blocking(uart, (uint8_t *) send, strlen(send));
    char *received_string = uart_read_string_with_timeout(uart, "VER", 2);
    if(received_string != NULL) {
        printf("%s\n", received_string);
        free(received_string);
        return true;
    } else {
        return false;
    }
}

void lora_fmt_deveui(char *input, char *output)
{
    char *start = strstr(input, ", ");
    if(start != NULL) {
        start += 2;
        int j = 0;
        for(int i = 0; start[i] != '\0'; i++) {
            if(isxdigit(start[i])) {
                output[j++] = tolower(start[i]);
            }
        }
        output[j] = '\0';
    }
}

bool lora_get_deveui(uart_inst_t *uart)
{
    const char *send = "AT+ID=DevEui\r\n";
    char lora_deveui[LORA_ID_LEN + 1];

    uart_write_blocking(uart, (uint8_t *) send, strlen(send));
    char *received_string = uart_read_string_with_timeout(uart, "ID", 2);
    if(received_string != NULL) {
        lora_fmt_deveui(received_string, lora_deveui);
        printf("%s\n", lora_deveui);
        free(received_string);
        return true;
    } else {
        return false;
    }
}

int main(void) {
    stdio_init_all();
    printf("Booting...\n");

    gpio_init(SW_0);
    gpio_set_dir(SW_0, GPIO_IN);
    gpio_pull_up(SW_0);

    uart_init(UART_ID, BAUD_RATE);

    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    LoRaState state = STATE_IDLE;

    while(true) {
        switch(state) {
            case STATE_IDLE:
                if(!gpio_get(SW_0)) {
                    while(!gpio_get(SW_0)) {
                        sleep_ms(50);
                    }
                    state = STATE_CONNECTING;
                }
                break;
            case STATE_CONNECTING:
                if(lora_test_uart_connection(UART_ID)) {
                    state = STATE_READING_FIRMWARE;
                } else {
                    state = STATE_ERROR;
                }
                break;
            case STATE_READING_FIRMWARE:
                if(lora_get_fw_version(UART_ID)) {
                    state = STATE_READING_DEVEUI;
                } else {
                    state = STATE_ERROR;
                }
                break;
            case STATE_READING_DEVEUI:
                if(lora_get_deveui(UART_ID)) {
                    state = STATE_IDLE;
                } else {
                    state = STATE_ERROR;
                }
                break;
            case STATE_ERROR:
                printf("Module not responding.\n");
                state = STATE_IDLE;
                break;
        }
    }

    return 0;
}