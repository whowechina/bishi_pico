/*
 * Controller Main
 * WHowe <github.com/whowechina>
 */

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "pico/stdio.h"
#include "pico/stdlib.h"
#include "bsp/board.h"
#include "pico/multicore.h"
#include "pico/bootrom.h"

#include "hardware/gpio.h"
#include "hardware/sync.h"
#include "hardware/structs/ioqspi.h"
#include "hardware/structs/sio.h"

#include "tusb.h"
#include "usb_descriptors.h"

#include "board_defs.h"

#include "save.h"
#include "config.h"
#include "cli.h"
#include "commands.h"

#include "light.h"
#include "button.h"
#include "spin.h"

#define WHITE 0xffffff

static void run_lights()
{
    uint16_t button = button_read();

    uint32_t phase = time_us_32() >> 16;
    uint32_t color = rgb32_from_hsv(phase + 51, 255, 128);
    if (button & 0x01) {
        color = WHITE;
    }
    light_set_spinner(color, false);

    for (int i = 0; i < 3; i++) {
        color = rgb32_from_hsv(phase + (i + 1) * 51, 255, 128);
        if (button & (1 << (3 - i))) {
            color = WHITE;
        }
        light_set_button(i, color, false);
    }

    color = rgb32_from_hsv(phase + 51 * 4, 255, 128);
    light_set_cab(color, false);
}

static void runtime_setup()
{
    uint16_t button = button_read();

    bool aux_down = button & 0x40;
    bool int_pedal = button & 0x80;

    if (aux_down && int_pedal) {
        bishi_runtime.ext_pedal_invert = button & 0x100;
    }
}

static mutex_t core1_io_lock;
static void core1_loop()
{
    while (1) {
        if (mutex_try_enter(&core1_io_lock, NULL)) {
            run_lights();
            light_update();
            mutex_exit(&core1_io_lock);
        }
        cli_fps_count(1);
        sleep_us(700);
    }
}

struct __attribute__((packed)) {
    uint16_t buttons;
    uint8_t joy[4];
} hid_report = {0};

static void hid_update()
{
    hid_report.buttons = button_read();
    hid_report.joy[0] = spin_units(0);
    hid_report.joy[1] = spin_units(0);
    hid_report.joy[2] = spin_units(0);
    hid_report.joy[3] = spin_units(0);

    if (tud_hid_ready()) {
        tud_hid_n_report(0, REPORT_ID_JOYSTICK, &hid_report, sizeof(hid_report));
    }
}

static void core0_loop()
{
    while(1) {
        tud_task();

        cli_run();

        save_loop();
        cli_fps_count(0);

        button_update();
        spin_update();
        hid_update();
        runtime_setup();

        sleep_us(900);
    }
}

/* if certain key pressed when booting, enter update mode */
static void update_check()
{
    const uint8_t pins[] = BUTTON_DEF;
    int pressed = 0;
    for (int i = 0; i < count_of(pins); i++) {
        uint8_t gpio = pins[i];
        gpio_init(gpio);
        gpio_set_function(gpio, GPIO_FUNC_SIO);
        gpio_set_dir(gpio, GPIO_IN);
        gpio_pull_up(gpio);
        sleep_ms(1);
        if (!gpio_get(gpio)) {
            pressed++;
        }
    }

    if (pressed >= 4) {
        sleep_ms(100);
        reset_usb_boot(0, 2);
        return;
    }
}

static void detect_pedal_polarity()
{
    uint16_t buttons = button_read();
    bishi_runtime.ext_pedal_invert = buttons & 0x100;
}

void init()
{
    sleep_ms(50);
    board_init();

    update_check();

    tusb_init();
    stdio_init_all();

    config_init();
    mutex_init(&core1_io_lock);
    save_init(0xca44caac, &core1_io_lock);

    light_init();
    button_init();
    spin_init();

    detect_pedal_polarity();

    cli_init("bishi_pico>", "\n   << Bishi Pico Controller >>\n"
                            " https://github.com/whowechina\n\n");
    
    commands_init();
}

int main(void)
{
    init();
    multicore_launch_core1(core1_loop);
    core0_loop();
    return 0;
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id,
                               hid_report_type_t report_type, uint8_t *buffer,
                               uint16_t reqlen)
{
    printf("Get from USB %d-%d\n", report_id, report_type);
    return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id,
                           hid_report_type_t report_type, uint8_t const *buffer,
                           uint16_t bufsize)
{
    if (report_id == REPORT_ID_LIGHTS) {
    }
}
