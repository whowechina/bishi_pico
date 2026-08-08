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

#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/sync.h"
#include "hardware/structs/ioqspi.h"
#include "hardware/structs/sio.h"

#include "tusb.h"
#include "usb_descriptors.h"

#include "board_defs.h"

#include "savedata.h"
#include "config.h"
#include "cli.h"
#include "commands.h"

#include "light.h"
#include "button.h"
#include "spin.h"
#include "chain.h"

typedef struct {
    uint8_t h;
    uint8_t s;
    uint8_t v;
} hsv_t;

typedef struct {
    hsv_t off[5];  /* CAB, SPIN, A, B, C */
    hsv_t on[5];
} theme_pattern_t;

static struct {
    const char *name;
    uint8_t keybind;
    theme_pattern_t lights[6];
} light_theme[] = {
    {
        "Bishi Bashi Channel",
        0x01,
        {
            { },
            {
                { { RED, 255, 255 }, { GREEN, 255, 80 }, { RED, 255, 128 }, { 0 }, { BLUE, 255, 128 }, },
                { { RED, 255, 255 }, { GREEN, 255, 255 }, { RED, 255, 255 }, { 0 }, { BLUE, 255, 255 }, },
            },
            {
                { { GREEN, 255, 255 }, { GREEN, 255, 80 }, { RED, 255, 128 }, { 0 }, { BLUE, 255, 128 }, },
                { { GREEN, 255, 255 }, { GREEN, 255, 255 }, { RED, 255, 255 }, { 0 }, { BLUE, 255, 255 }, },
            },
            {
                { { BLUE, 255, 255 }, { GREEN, 255, 80 }, { RED, 255, 128 }, { 0 }, { BLUE, 255, 128 }, },
                { { BLUE, 255, 255 }, { GREEN, 255, 255 }, { RED, 255, 255 }, { 0 }, { BLUE, 255, 255 }, },
            },
            {
                { { YELLOW, 255, 255 }, { GREEN, 255, 80 }, { RED, 255, 128 }, { 0 }, { BLUE, 255, 128 }, },
                { { YELLOW, 255, 255 }, { GREEN, 255, 255 }, { RED, 255, 255 }, { 0 }, { BLUE, 255, 255 }, },
            },
        },
    },
    {
        "Old Bishi Bashi",
        0x04,
        {
            { },
            {
                { { RED, 255, 255 }, { YELLOW, 255, 128 }, { RED, 255, 128 }, { GREEN, 255, 128 }, { BLUE, 255, 128 }, },
                { { RED, 255, 255 }, { YELLOW, 255, 255 }, { RED, 255, 255 }, { GREEN, 255, 255 }, { BLUE, 255, 255 }, },
            },
            {
                { { GREEN, 255, 255 }, { YELLOW, 255, 128 }, { RED, 255, 128 }, { GREEN, 255, 128 }, { BLUE, 255, 128 }, },
                { { GREEN, 255, 255 }, { YELLOW, 255, 255 }, { RED, 255, 255 }, { GREEN, 255, 255 }, { BLUE, 255, 255 }, },
            },
            {
                { { BLUE, 255, 255 }, { YELLOW, 255, 128 }, { RED, 255, 128 }, { GREEN, 255, 128 }, { BLUE, 255, 128 }, },
                { { BLUE, 255, 255 }, { YELLOW, 255, 255 }, { RED, 255, 255 }, { GREEN, 255, 255 }, { BLUE, 255, 255 }, },
            },
            {
                { { 0 }, { 0 }, { 0 }, { 0 }, { 0 }, },
                { { 0, 0, 10 }, { 0, 0, 10 }, { 0, 0, 10 }, { 0, 0, 10 }, { 0, 0, 10 }, },
            },
        },
    },
    {
        "Love Live",
        0x0a,
        {
            { },
            {
                { { 0 }, { RED, 255, 200 }, { 0, 0, 128 }, { 0, 0, 128 }, { 0, 0, 128 }, },
                { { 0 }, { RED, 255, 255 }, { 0, 0, 255 }, { 0, 0, 255 }, { 0, 0, 255 }, },
            },
            {
                { { 0 }, { 0 }, { 0, 0, 128 }, { 0, 0, 128 }, { 0, 0, 128 }, },
                { { 0 }, { 0 }, { 0, 0, 255 }, { 0, 0, 255 }, { 0, 0, 255 }, },
            },
            {
                { { 0 }, { BLUE, 255, 200 }, { 0, 0, 128 }, { 0, 0, 128 }, { 0, 0, 128 }, },
                { { 0 }, { BLUE, 255, 255 }, { 0, 0, 255 }, { 0, 0, 255 }, { 0, 0, 255 }, },
            },
            {
                { { 0 }, { 0 }, { 0 }, { 0 }, { 0 }, },
                { { 0, 0, 10 }, { 0, 0, 10 }, { 0, 0, 10 }, { 0, 0, 10 }, { 0, 0, 10 }, },
            },
        },
    },
    {
        "Live Sports",
        0x08,
        {
            { },
            {
                { { RED, 255, 255 }, { GREEN, 255, 128 }, { BLUE, 255, 128 }, { 0 }, { RED, 255, 128 }, },
                { { RED, 255, 255 }, { GREEN, 0, 255 }, { BLUE, 0, 255 }, { 0, 0, 10 }, { RED, 0, 255 }, },
            },
            {
                { { BLUE, 255, 255 }, { GREEN, 255, 128 }, { BLUE, 255, 128 }, { 0 }, { RED, 255, 128 }, },
                { { BLUE, 255, 255 }, { GREEN, 0, 255 }, { BLUE, 0, 255 }, { 0, 0, 10 }, { RED, 0, 255 }, },
            },
            {
                { { 0 }, { 0 }, { 0 }, { 0 }, { 0 }, },
                { { 0, 0, 10 }, { 0, 0, 10 }, { 0, 0, 10 }, { 0, 0, 10 }, { 0, 0, 10 }, },
            },
            {
                { { 0 }, { 0 }, { 0 }, { 0 }, { 0 }, },
                { { 0, 0, 10 }, { 0, 0, 10 }, { 0, 0, 10 }, { 0, 0, 10 }, { 0, 0, 10 }, },
            },
        },
    },
    {
        "Sega Naomi Mini Game",
        0x02,
        {
            { },
            {
                { { RED, 255, 255 }, { 0 }, { BLUE, 255, 128 }, { YELLOW, 255, 128 }, { RED, 255, 128 }, },
                { { RED, 255, 255 }, { 0 }, { BLUE, 255, 255 }, { YELLOW, 255, 255 }, { RED, 255, 255 }, },
            },
            {
                { { GREEN, 255, 255 }, { 0 }, { BLUE, 255, 128 }, { YELLOW, 255, 128 }, { RED, 255, 128 }, },
                { { GREEN, 255, 255 }, { 0 }, { BLUE, 255, 255 }, { YELLOW, 255, 255 }, { RED, 255, 255 }, },
            },
            {
                { { BLUE, 255, 255 }, { 0 }, { BLUE, 255, 128 }, { YELLOW, 255, 128 }, { RED, 255, 128 }, },
                { { BLUE, 255, 255 }, { 0 }, { BLUE, 255, 255 }, { YELLOW, 255, 255 }, { RED, 255, 255 }, },
            },
            {
                { { 0 }, { 0 }, { 0 }, { 0 }, { 0 }, },
                { { 0 }, { 0 }, { 0 }, { 0 }, { 0 }, },
            },
        },
    },
    {
        "Come on Baby",
        0x06,
        {
            { },
            {
                { { YELLOW, 255, 255 }, { RED, 255, 128 }, { GREEN, 255, 128 }, { 0 }, { BLUE, 255, 128 }, },
                { { YELLOW, 255, 255 }, { RED, 255, 255 }, { GREEN, 255, 255 }, { 0 }, { BLUE, 255, 255 }, },
            },
            {
                { { CYAN, 255, 255 }, { RED, 255, 128 }, { GREEN, 255, 128 }, { 0 }, { BLUE, 255, 128 }, },
                { { CYAN, 255, 255 }, { RED, 255, 255 }, { GREEN, 255, 255 }, { 0 }, { BLUE, 255, 255 }, },
            },
            {
                { { 0 }, { 0 }, { 0 }, { 0 }, { 0 }, },
                { { 0 }, { 0 }, { 0 }, { 0 }, { 0 }, },
            },
            {
                { { 0 }, { 0 }, { 0 }, { 0 }, { 0 }, },
                { { 0 }, { 0 }, { 0 }, { 0 }, { 0 }, },
            },
        },
    },
};

#define THEME_NUM count_of(light_theme)

static void print_keybind(uint8_t keybind)
{
    static const struct {
        uint8_t mask;
        const char *name;
    } keys[] = {
        { 0x01, "SPIN" },
        { 0x02, "A" },
        { 0x04, "B" },
        { 0x08, "C" },
    };

    bool first = true;
    for (uint8_t i = 0; i < count_of(keys); i++) {
        if (keybind & keys[i].mask) {
            if (!first) {
                printf("+");
            }
            printf("%s", keys[i].name);
            first = false;
        }
    }

    if (first) {
        printf("NONE");
    }
}

void theme_list_print(void)
{
    printf("[Themes]\n");
    for (uint8_t i = 0; i < THEME_NUM; i++) {
        printf("  %u: %s (boot key: ", i, light_theme[i].name);
        print_keybind(light_theme[i].keybind);
        printf(")\n");
    }
}

static uint32_t hsv2rgb(hsv_t hsv)
{
    return rgb32_from_hsv(hsv.h, hsv.s, hsv.v);
}

static void light_render()
{
    uint32_t phase = time_us_32() >> 16;
    uint8_t theme = bishi_cfg->theme % THEME_NUM;
    uint8_t id = bishi_runtime.chain_id % 5;

    for (int i = 0; i < 5; i++) {
        light_theme[theme].lights[0].off[i].h = 51 * i + phase;
        light_theme[theme].lights[0].on[i].h = 51 * i + phase;
        light_theme[theme].lights[0].off[i].s = 255;
        light_theme[theme].lights[0].on[i].s = 255;
        light_theme[theme].lights[0].off[i].v = 180;
        light_theme[theme].lights[0].on[i].v = 255;
    }

    theme_pattern_t *my_theme = &light_theme[theme].lights[id];

    light_set_cab(hsv2rgb(my_theme->off[0]), false);
    light_set_spinner(hsv2rgb(my_theme->off[1]), false);
    light_set_button(2, hsv2rgb(my_theme->off[2]), false);
    light_set_button(1, hsv2rgb(my_theme->off[3]), false);
    light_set_button(0, hsv2rgb(my_theme->off[4]), false);

    uint16_t button = button_read();
    if (button & 0x01) {
        light_set_spinner(hsv2rgb(my_theme->on[1]), false);
    }
    if (button & 0x02) {
        light_set_button(2, hsv2rgb(my_theme->on[2]), false);
    }
    if (button & 0x04) {
        light_set_button(1, hsv2rgb(my_theme->on[3]), false);
    }
    if (button & 0x08) {
        light_set_button(0, hsv2rgb(my_theme->on[4]), false);
    }
}

static void run_lights()
{
    static uint64_t next_frame = 0;
    uint64_t now = time_us_64();
    if (now < next_frame) {
        return;
    }
    next_frame = now + 1000000 / 120; // 120Hz

    light_render();
}

static void core1_init()
{
    flash_safe_execute_core_init();
}

static void core1_loop()
{
    core1_init();
    while (1) {
        run_lights();
        light_update();
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
    static uint64_t last_report = 0;
    uint64_t now = time_us_64();
    if (now - last_report < 1000) {
        return;
    }
    last_report = now;

    const uint8_t *report = chain_report();
    hid_report.buttons = ((report[6] & 0x0f) << 12) | ((report[4] & 0x0f) << 8) |
                         ((report[2] & 0xff) << 4) | (report[0] & 0x0f);
    hid_report.joy[0] = report[1];
    hid_report.joy[1] = report[3];
    hid_report.joy[2] = report[5];
    hid_report.joy[3] = report[7];

    if (tud_hid_ready()) {
        tud_hid_n_report(0, REPORT_ID_JOYSTICK, &hid_report, sizeof(hid_report));
    }
}

#define LOOP_HZ 4000

static void core0_loop()
{
    uint64_t next_frame = 0;

    while(1) {
        tud_task();

        cli_run();

        savedata_loop();
        cli_fps_count(0);

        button_update();
        spin_update();
        chain_report_self(button_read(), spin_units());

        chain_update();
        hid_update();

        sleep_until(next_frame);
        next_frame += 1001;
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

static void theme_check()
{
    uint16_t button = button_read();
    for (uint8_t i = 0; i < THEME_NUM; i++) {
        if (light_theme[i].keybind == button) {
            bishi_cfg->theme = i;
            config_changed();
            return;
        }
    }

    bishi_cfg->theme = 0;
    config_changed();
}

void init()
{
    sleep_ms(50);
    set_sys_clock_khz(150000, true);

    board_init();

    update_check();

    tusb_init();
    stdio_init_all();

    config_init();
    savedata_init(0xca44caac);

    usb_descriptors_set_sn(bishi_cfg->sn);

    light_init();
    button_init();
    spin_init();
    chain_init();

    cli_init("bishi_pico>", "\n   << Bishi Pico Controller >>\n"
                            " https://github.com/whowechina\n\n");
    
    theme_check();
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
