/*
 * The Spinning of Buttons
 * WHowe <github.com/whowechina>
 * 
 */

#include "spin.h"

#include <stdint.h>
#include <stdbool.h>

#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/pwm.h"

#include "config.h"
#include "board_defs.h"

#include "tmag5273.h"

const uint16_t FULL_SCALE = 360 * 16;

void spin_init()
{
    gpio_init(BUS_I2C_SDA);
    gpio_set_function(BUS_I2C_SDA, GPIO_FUNC_I2C);
    gpio_pull_up(BUS_I2C_SDA);
    gpio_init(BUS_I2C_SCL);
    gpio_set_function(BUS_I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(BUS_I2C_SCL);
    i2c_init(BUS_I2C, BUS_I2C_FREQ);

    uint8_t ce = SPIN_VCC_DEF;
    gpio_init(ce);
    gpio_set_dir(ce, GPIO_OUT);
    gpio_put(ce, 1);
    sleep_us(1000);
    tmag5273_init(0, BUS_I2C);
    tmag5273_init_sensor();
    if (bishi_cfg->spin.units_per_turn == 0) {
        bishi_cfg->spin.units_per_turn = 80;
        config_changed();
    }
}

bool spin_present()
{
    return tmag5273_is_present(0);
}

static uint16_t spin_reading;

void spin_update()
{
    spin_reading = tmag5273_read_angle();
    if (bishi_cfg->spin.reversed) {
        spin_reading = FULL_SCALE - spin_reading;
    }
}

uint16_t spin_read()
{
    return spin_reading;
}

uint16_t spin_units()
{
    static uint16_t last = 0;
    static int counter = 0;

    int delta = spin_reading - last;
    if (delta > FULL_SCALE / 2) {
        delta -= FULL_SCALE;
    } else if (delta < -FULL_SCALE / 2) {
        delta += FULL_SCALE;
    }

    int resolution = FULL_SCALE / bishi_cfg->spin.units_per_turn;
    if ((delta <= -resolution) || (delta >= resolution)) {
        last = spin_reading;
        counter += delta;
    }

    return counter * bishi_cfg->spin.units_per_turn / FULL_SCALE;
}
