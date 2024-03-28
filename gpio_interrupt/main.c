#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/irq.h"

#define LED_0 20
#define LED_1 21
#define LED_2 22

#define ROT_A 10
#define ROT_B 11
#define ROT_SW 12

#define DEBOUNCE_DELAY_US 100000

bool clockwise = false;
bool counterclockwise = false;
bool leds_on = false;

void init_gpio();
void init_pwm();
void toggle_leds(bool *leds_on, uint16_t *cc);
void mutate_leds(uint16_t cc);
void isr_rotate();

int main(void)
{
    stdio_init_all();

    printf("Booting...\n");

    init_gpio();
    init_pwm();

    bool prev_rot_sw_state = gpio_get(ROT_SW);

    uint16_t cc = 500;
    uint16_t cc_max = 1000;
    uint8_t cc_step = 50;

    gpio_set_irq_enabled_with_callback(ROT_A, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &isr_rotate);
    gpio_set_irq_enabled_with_callback(ROT_B, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &isr_rotate);

    while(true) {
        bool rot_sw_state = !gpio_get(ROT_SW);

        if(rot_sw_state != prev_rot_sw_state) {
            busy_wait_us_32(DEBOUNCE_DELAY_US);
            prev_rot_sw_state = rot_sw_state;
            if(rot_sw_state) {
                toggle_leds(&leds_on, &cc);
            }
        }
        if(clockwise) {
            cc += cc_step;
            if(cc > cc_max) {
                cc = cc_max;
            }
            mutate_leds(cc);
            clockwise = false;
        } else if(counterclockwise) {
            if(cc < cc_step) {
                cc = 0;
            } else {
                cc -= cc_step;
            }
            mutate_leds(cc);
            counterclockwise = false;
        }
    }
    return 0;
}

void init_gpio()
{
    gpio_init(ROT_SW);
    gpio_set_dir(ROT_SW, GPIO_IN);
    gpio_pull_up(ROT_SW);

    gpio_init(ROT_A);
    gpio_set_dir(ROT_A, GPIO_IN);

    gpio_init(ROT_B);
    gpio_set_dir(ROT_B, GPIO_IN);
}

void init_pwm()
{
    uint slice_num_1 = pwm_gpio_to_slice_num(LED_0);
    uint slice_num_2 = pwm_gpio_to_slice_num(LED_1);
    uint slice_num_3 = pwm_gpio_to_slice_num(LED_2);

    pwm_set_enabled(slice_num_1, false);
    pwm_set_enabled(slice_num_2, false);
    pwm_set_enabled(slice_num_3, false);

    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv_int(&config, 125);
    pwm_config_set_wrap(&config, 1000);

    pwm_init(slice_num_1, &config, false);
    pwm_init(slice_num_2, &config, false);
    pwm_init(slice_num_3, &config, false);

    gpio_set_function(LED_0, GPIO_FUNC_PWM);
    gpio_set_function(LED_1, GPIO_FUNC_PWM);
    gpio_set_function(LED_2, GPIO_FUNC_PWM);

    pwm_set_enabled(slice_num_1, true);
    pwm_set_enabled(slice_num_2, true);
    pwm_set_enabled(slice_num_3, true);
}

void toggle_leds(bool *leds_on, uint16_t *cc) {
    uint slice_num_1 = pwm_gpio_to_slice_num(LED_0);
    uint slice_num_2 = pwm_gpio_to_slice_num(LED_1);
    uint slice_num_3 = pwm_gpio_to_slice_num(LED_2);

    uint channel_num_1 = pwm_gpio_to_channel(LED_0);
    uint channel_num_2 = pwm_gpio_to_channel(LED_1);
    uint channel_num_3 = pwm_gpio_to_channel(LED_2);

    if(!(*leds_on)) {
        pwm_set_chan_level(slice_num_1, channel_num_1, *cc);
        pwm_set_chan_level(slice_num_2, channel_num_2, *cc);
        pwm_set_chan_level(slice_num_3, channel_num_3, *cc);
        *leds_on = true;
    } else {
        if(*cc == 0) {
            *cc = 500;
            pwm_set_chan_level(slice_num_1, channel_num_1, *cc);
            pwm_set_chan_level(slice_num_2, channel_num_2, *cc);
            pwm_set_chan_level(slice_num_3, channel_num_3, *cc);
        } else {
            pwm_set_chan_level(slice_num_1, channel_num_1, 0);
            pwm_set_chan_level(slice_num_2, channel_num_2, 0);
            pwm_set_chan_level(slice_num_3, channel_num_3, 0);
            *leds_on = false;
        }
    }
}

void mutate_leds(uint16_t cc)
{
    uint slice_num_1 = pwm_gpio_to_slice_num(LED_0);
    uint slice_num_2 = pwm_gpio_to_slice_num(LED_1);
    uint slice_num_3 = pwm_gpio_to_slice_num(LED_2);

    uint channel_num_1 = pwm_gpio_to_channel(LED_0);
    uint channel_num_2 = pwm_gpio_to_channel(LED_1);
    uint channel_num_3 = pwm_gpio_to_channel(LED_2);

    pwm_set_chan_level(slice_num_1, channel_num_1, cc);
    pwm_set_chan_level(slice_num_2, channel_num_2, cc);
    pwm_set_chan_level(slice_num_3, channel_num_3, cc);
}

void isr_rotate()
{
    static uint8_t prev_state_a = 0;
    static uint8_t prev_state_b = 0;

    uint8_t state_a = gpio_get(ROT_B);
    uint8_t state_b = gpio_get(ROT_A);

    if(leds_on) {
        if (state_a != prev_state_a) {
            if (state_a == state_b) {
                clockwise = true;
            } else {
                counterclockwise = true;
            }
        }
    }

    prev_state_a = state_a;
    prev_state_b = state_b;
}