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

#define PWM_TOP (1000)
#define CC_STEP 50
#define DEBOUNCE_DELAY_MS 100

void init_gpio()
{
    gpio_init(BUTTON_TOGGLE);
    gpio_set_dir(BUTTON_TOGGLE, GPIO_IN);
    gpio_pull_up(BUTTON_TOGGLE);

    gpio_init(BUTTON_BRIGHTNESS_INC);
    gpio_set_dir(BUTTON_BRIGHTNESS_INC, GPIO_IN);
    gpio_pull_up(BUTTON_BRIGHTNESS_INC);

    gpio_init(BUTTON_BRIGHTNESS_DEC);
    gpio_set_dir(BUTTON_BRIGHTNESS_DEC, GPIO_IN);
    gpio_pull_up(BUTTON_BRIGHTNESS_DEC);
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
    pwm_config_set_wrap(&config, PWM_TOP - 1);

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

void led_brightness_change(uint16_t cc)
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

int main(void)
{
    stdio_init_all();

    printf("Booting...\n");

    init_gpio();
    init_pwm();

    uint16_t cc = PWM_TOP / 2;

    bool leds_on = false;

    bool last_toggle_button_state = gpio_get(BUTTON_TOGGLE);
    bool last_inc_button_state = gpio_get(BUTTON_BRIGHTNESS_INC);
    bool last_dec_button_state = gpio_get(BUTTON_BRIGHTNESS_DEC);

    while(true) {
        bool toggle_button_state = !gpio_get(BUTTON_TOGGLE);
        bool inc_button_state = !gpio_get(BUTTON_BRIGHTNESS_INC);
        bool dec_button_state = !gpio_get(BUTTON_BRIGHTNESS_DEC);

        if(toggle_button_state != last_toggle_button_state) {
            busy_wait_ms(DEBOUNCE_DELAY_MS);
            last_toggle_button_state = toggle_button_state;

            if(toggle_button_state) {
                if(!leds_on) {
                    led_brightness_change(cc);
                    leds_on = true;
                } else {
                    if(cc == 0) {
                        cc = PWM_TOP / 2;
                        led_brightness_change(cc);
                    } else {
                        led_brightness_change(0);
                        leds_on = false;
                    }
                }
            }
        }

        if(inc_button_state != last_inc_button_state && leds_on) {
            busy_wait_ms(DEBOUNCE_DELAY_MS);
            last_inc_button_state = inc_button_state;

            if(inc_button_state) {
                while(!gpio_get(BUTTON_BRIGHTNESS_INC)) {
                    cc += CC_STEP;
                    if(cc > PWM_TOP) {
                        cc = PWM_TOP;
                        break;
                    }
                    led_brightness_change(cc);
                    busy_wait_ms(DEBOUNCE_DELAY_MS);
                }
            }
        }

        if(dec_button_state != last_dec_button_state && leds_on) {
            busy_wait_ms(DEBOUNCE_DELAY_MS);
            last_dec_button_state = dec_button_state;

            if(dec_button_state) {
                while(!gpio_get(BUTTON_BRIGHTNESS_DEC)) {
                    if(cc < CC_STEP) {
                        cc = 0;
                    } else {
                        cc -= CC_STEP;
                    }
                    led_brightness_change(cc);
                    busy_wait_ms(DEBOUNCE_DELAY_MS);
                }
            }
        }
    }

    return 0;
}