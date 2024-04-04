#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/util/queue.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/irq.h"

#define LED_0 20
#define LED_1 21
#define LED_2 22

#define ROT_A 10
#define ROT_B 11
#define ROT_SW 12

#define PWM_TOP (1000)
#define CC_STEP 25
#define DEBOUNCE_DELAY_MS 100

static queue_t events_clockwise;
static queue_t events_counterclockwise;

volatile bool clockwise = false;
volatile bool counterclockwise = false;

bool leds_on = false;

void init_gpio(void)
{
    gpio_init(ROT_SW);
    gpio_set_dir(ROT_SW, GPIO_IN);
    gpio_pull_up(ROT_SW);

    gpio_init(ROT_A);
    gpio_set_dir(ROT_A, GPIO_IN);

    gpio_init(ROT_B);
    gpio_set_dir(ROT_B, GPIO_IN);
}

void init_pwm(void)
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

void led_brightness_control(uint16_t cc)
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

void toggle_leds(bool *leds_on, uint16_t *cc) {
    if(!(*leds_on)) {
        led_brightness_control(*cc);
        *leds_on = true;
    } else {
        if(*cc == 0) {
            *cc = 500;
            led_brightness_control(*cc);
        } else {
            led_brightness_control(0);
            *leds_on = false;
        }
    }
}

void gpio_handler(void)
{
    static bool prev_state_a = false;
    static bool prev_state_b = false;

    bool state_a = gpio_get(ROT_B);
    bool state_b = gpio_get(ROT_A);

    if(leds_on) {
        if(state_a != prev_state_a) {
            if(state_a == state_b) {
                clockwise = true;
                queue_try_add(&events_clockwise, &clockwise);
            } else {
                queue_try_add(&events_counterclockwise, &counterclockwise);
                counterclockwise = true;
            }
        }
    }

    prev_state_a = state_a;
    prev_state_b = state_b;
}

int main(void)
{
    stdio_init_all();

    printf("Booting...\n");

    init_gpio();
    init_pwm();

    queue_init(&events_clockwise, sizeof(bool), 10);
    queue_init(&events_counterclockwise, sizeof(bool), 10);

    uint16_t cc = PWM_TOP / 2;

    gpio_set_irq_enabled_with_callback(ROT_A, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &gpio_handler);
    gpio_set_irq_enabled_with_callback(ROT_B, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &gpio_handler);

    bool prev_rot_sw_state = !gpio_get(ROT_SW);

    while(true) {
        bool rot_sw_state = !gpio_get(ROT_SW);

        if(rot_sw_state != prev_rot_sw_state) {
            busy_wait_ms(DEBOUNCE_DELAY_MS);
            prev_rot_sw_state = rot_sw_state;
            if(rot_sw_state) {
                toggle_leds(&leds_on, &cc);
            }
        }

        while(queue_try_remove(&events_clockwise, &clockwise)) {
            cc += CC_STEP;
            if(cc > PWM_TOP) {
                cc = PWM_TOP;
            }
            led_brightness_control(cc);
        }

        while(queue_try_remove(&events_counterclockwise, &counterclockwise)) {
            if(cc < CC_STEP) {
                cc = 0;
            } else {
                cc -= CC_STEP;
            }
            led_brightness_control(cc);
        }
    }

    queue_free(&events_clockwise);
    queue_free(&events_counterclockwise);

    return 0;
}