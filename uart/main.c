#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/uart.h"

#define SW_0 7

#define DEBOUNCE_DELAY_US 100000

int main(void)
{
    stdio_init_all();

    printf("Booting...\n");

    while(true) {
        tight_loop_contents();
    }

    return 0;
}
