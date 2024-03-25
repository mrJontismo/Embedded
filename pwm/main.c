#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"

#define BUTTON_TOGGLE 8
#define BUTTON_BRIGHTNESS_INC 9
#define BUTTON_BRIGHTNESS_DEC 7

#define LED_0 20
#define LED_1 21
#define LED_2 22

#define PWM_FREQ 1000
#define PWM_CLK_DIVIDER 1000

int main(void)
{
    stdio_init_all();

    printf("Booting...\n");

    gpio_init(BUTTON_TOGGLE);
    gpio_set_dir(BUTTON_TOGGLE, GPIO_IN);
    gpio_pull_up(BUTTON_TOGGLE);

    gpio_init(BUTTON_BRIGHTNESS_INC);
    gpio_set_dir(BUTTON_BRIGHTNESS_INC, GPIO_IN);
    gpio_pull_up(BUTTON_BRIGHTNESS_INC);

    gpio_init(BUTTON_BRIGHTNESS_DEC);
    gpio_set_dir(BUTTON_BRIGHTNESS_DEC, GPIO_IN);
    gpio_pull_up(BUTTON_BRIGHTNESS_DEC);

    uint slice_num_1 = pwm_gpio_to_slice_num(LED_0);
    uint channel_num_1 = pwm_gpio_to_channel(LED_0);

    uint slice_num_2 = pwm_gpio_to_slice_num(LED_1);
    uint channel_num_2 = pwm_gpio_to_channel(LED_1);

    uint slice_num_3 = pwm_gpio_to_slice_num(LED_2);
    uint channel_num_3 = pwm_gpio_to_channel(LED_2);

    pwm_set_enabled(slice_num_1, false);
    pwm_set_enabled(slice_num_2, false);
    pwm_set_enabled(slice_num_3, false);

    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv_int(&config, PWM_CLK_DIVIDER);
    pwm_config_set_wrap(&config, PWM_FREQ);

    pwm_init(slice_num_1, &config, false);
    pwm_init(slice_num_2, &config, false);
    pwm_init(slice_num_3, &config, false);

    gpio_set_function(LED_0, GPIO_FUNC_PWM);
    gpio_set_function(LED_1, GPIO_FUNC_PWM);
    gpio_set_function(LED_2, GPIO_FUNC_PWM);

    pwm_set_enabled(slice_num_1, true);
    pwm_set_enabled(slice_num_2, true);
    pwm_set_enabled(slice_num_3, true);

    uint16_t cc = 500;
    uint16_t cc_max = 1000;
    uint16_t cc_min = 0;
    uint8_t cc_step = 100;

    bool leds_on = false;
    bool button_pressed = false;

    uint32_t last_button_press_time = 0;

    while(true) {
        bool toggle_button_state = !gpio_get(BUTTON_TOGGLE);
        bool brightness_inc_button_state = !gpio_get(BUTTON_BRIGHTNESS_INC);
        bool brightness_dec_button_state = !gpio_get(BUTTON_BRIGHTNESS_DEC);

        if(toggle_button_state != button_pressed) {
            button_pressed = toggle_button_state;

            if(button_pressed) {
                if(time_us_32() - last_button_press_time > 100000) {
                    last_button_press_time = time_us_32();
                    if(!leds_on) {
                        pwm_set_chan_level(slice_num_1, channel_num_1, cc);
                        pwm_set_chan_level(slice_num_2, channel_num_2, cc);
                        pwm_set_chan_level(slice_num_3, channel_num_3, cc);
                        leds_on = true;
                    } else {
                        if (cc == cc_min) {
                            cc = 500;
                            pwm_set_chan_level(slice_num_1, channel_num_1, cc);
                            pwm_set_chan_level(slice_num_2, channel_num_2, cc);
                            pwm_set_chan_level(slice_num_3, channel_num_3, cc);
                        } else {
                            pwm_set_chan_level(slice_num_1, channel_num_1, cc_min);
                            pwm_set_chan_level(slice_num_2, channel_num_2, cc_min);
                            pwm_set_chan_level(slice_num_3, channel_num_3, cc_min);
                            leds_on = false;
                        }
                    }
                }
            }
        }


        if(brightness_inc_button_state && leds_on) {
            if(time_us_32() - last_button_press_time > 100000) {
                last_button_press_time = time_us_32();
                cc += cc_step;

                if (cc > cc_max) {
                    cc = cc_max;
                }

                printf("Increased CC: %d\n", cc);
                pwm_set_chan_level(slice_num_1, channel_num_1, cc);
                pwm_set_chan_level(slice_num_2, channel_num_2, cc);
                pwm_set_chan_level(slice_num_3, channel_num_3, cc);
            }
        }

        if(brightness_dec_button_state && leds_on) {
            if(time_us_32() - last_button_press_time > 100000) {
                last_button_press_time = time_us_32();

                if(cc < cc_step) {
                    cc = cc_min;
                } else {
                    cc -= cc_step;
                }

                printf("Decreased CC: %d\n", cc);
                pwm_set_chan_level(slice_num_1, channel_num_1, cc);
                pwm_set_chan_level(slice_num_2, channel_num_2, cc);
                pwm_set_chan_level(slice_num_3, channel_num_3, cc);
            }
        }

        sleep_ms(10);
    }

    return 0;
}
