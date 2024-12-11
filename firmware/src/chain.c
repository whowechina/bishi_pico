/*
 * Daisy Chain Ops
 * WHowe <github.com/whowechina>
 * 
 */

#include "chain.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "pico/stdio.h"
#include "pico/stdlib.h"

#include "hardware/gpio.h" 
#include "hardware/uart.h" 

#include "tusb.h"

#include "config.h"
#include "board_defs.h"

#define SYNC_INTERVAL_US 200000
#define SYNC_EXPIRE_TIME_US 1000000

/*
    Bishi Pico Chain Protocol
    1. Frame
        [FS] [CMD] [LEN] [DATA] [CS]
        (1B)  (1B)  (1B)  (nB)  (1B)
    
    FS (Frame Start): 0xFF
    CMD (Command): 0xF0..0xFE
    LEN (Data Length): Depends on CMD
    CS (Check Sum): FS ^ LEN ^ CMD ^ DATA

    2. CMD (Command) [0xF0..0xFE]
    2.1 0xF0: Report upward
        0xFF 0xF0 0x06 [BUTTON] [SPIN] [BUTTON] [SPIN] [BUTTON] [SPIN] [CS]
                         (1B)    (1B)    (1B)    (1B)    (1B)   (1B)   (1B)
        The first BUTTON/SPIN pair is "my" report, the latter are from "my children".

    * 0xF1: Sync downward
        0xFF 0xF1 0x03 [Your ID] [Theme] [Spin Rate] [CS]
                         (1B)      (1B)     (1B)     (1B)
        Master's ID is always 1, the children are 2, 3, 4.
        Each forwarding increases the ID by 1, and stops at 3.

    * 0xF2: HID light downard.
        0xFF 0xF2 0x10 [ID] [LED RGBs] [CS]
                       (1B)   (15B)    (1B)
        If ID matches, frame is consumed, otherwise forwarded.
*/

#define CHAIN_FS 0xFF
#define CHAIN_CMD_REPORT 0xF0
#define CHAIN_CMD_SYNC 0xF1
#define CHAIN_CMD_LIGHT 0xF2
#define CHAIN_CMD_CONFIG 0xF3

static uint8_t report_cache[8] = { 0 };

const uint8_t *chain_report()
{
    return report_cache;
}

void chain_report_self(uint8_t button, uint8_t spin)
{
    report_cache[0] = button;
    report_cache[1] = spin;
}

static void inline chain_uart_init(uart_inst_t *uart, uint baudrate, uint8_t tx, uint8_t rx)
{
    uart_init(uart, baudrate);
    gpio_set_function(tx, GPIO_FUNC_UART);
    gpio_set_function(rx, GPIO_FUNC_UART);
    uart_set_hw_flow(uart, false, false);
    uart_set_format(uart, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(uart, true);
}

void chain_init()
{
    chain_uart_init(UART_UP, UART_BAUDRATE, UART_UP_TX, UART_UP_RX);
    chain_uart_init(UART_DOWN, UART_BAUDRATE, UART_DOWN_TX, UART_DOWN_RX);
}

typedef struct {
    union {
        uint8_t buf[64];
        struct {
            uint8_t fs;
            uint8_t cmd;
            uint8_t datalen;
            uint8_t data[];
        };
    };
    int len;
} rx_buf;

static rx_buf rx_up;
static rx_buf rx_down;

static bool poll_data(rx_buf *buf, uart_inst_t *uart)
{
    bool new_data = false;
    while (uart_is_readable(uart)) {
        int c = uart_getc(uart);
        if (buf->len == sizeof(buf->buf)) {
            buf->len--;
            memmove(buf->buf, buf->buf + 1, buf->len);
        }
        buf->buf[buf->len] = c;
        buf->len++;
        new_data = true;
    }
    return new_data;
}

static inline void buf_consume(rx_buf *buf, int n)
{
    if (n >= buf->len) {
        buf->len = 0;
        return;
    }

    buf->len -= n;
    memmove(buf->buf, buf->buf + n, buf->len);
}

static inline void buf_consume_cmd(rx_buf *buf)
{
    buf_consume(buf, 4 + buf->datalen);
}

static void align_fs(rx_buf *buf)
{
    while ((buf->len > 0) && (buf->fs != CHAIN_FS)) {
        buf_consume(buf, 1);
    }
}

static bool locate_frame(rx_buf *buf)
{
    align_fs(buf);

    if (buf->len < 3) {
        return false;
    }

    if (buf->fs != CHAIN_FS) {
        return false;
    }

    if (buf->cmd < 0xF0) {
        return false;
    }

    if (buf->len < buf->datalen + 4) {
        return false;
    }

    uint8_t cs = buf->fs ^ buf->cmd ^ buf->datalen;
    for (int i = 0; i < buf->datalen; i++) {
        cs ^= buf->data[i];
    }
    if (cs != buf->data[buf->datalen]) {
        buf_consume(buf, 4 + buf->datalen);
        return false;
    }

    return true;
}

static void send_frame(uart_inst_t *uart, uint8_t cmd, uint8_t *data, int len)
{
    uint8_t cs = CHAIN_FS ^ cmd ^ len;
    for (int i = 0; i < len; i++) {
        cs ^= data[i];
    }

    uint8_t header[] = { CHAIN_FS, cmd, len };
    uart_write_blocking(uart, header, sizeof(header));
    uart_write_blocking(uart, data, len);
    uart_write_blocking(uart, &cs, 1);
}

static void proc_sync(uint8_t *data)
{
    uint8_t id = data[0];
    uint8_t theme = data[1];
    uint8_t rate = data[2];

    if ((id < 2) || (id > 4)) {
        return;
    }

    if (rate < 20) {
        rate = 80;
    }

    bishi_runtime.chain_id = id;
    bishi_runtime.sync_expire = time_us_64() + SYNC_EXPIRE_TIME_US;
    if (bishi_cfg->theme != theme) {
        bishi_cfg->theme = theme;
        config_changed();
    }
    if (bishi_cfg->spin.units_per_turn != rate) {
        bishi_cfg->spin.units_per_turn = rate;
        config_changed();
    }

    if (id < 4) {
        data[0] = id + 1;
        send_frame(UART_DOWN, CHAIN_CMD_SYNC, data, 2);
    }
}

static void proc_from_upstream()
{
    if (poll_data(&rx_up, UART_UP) > 0) {
    //    printf("Got data %d:", rx_up.len);
        for (int i = 0; i < rx_up.len; i++) {
    //        printf(" %02x", rx_up.buf[i]);
        }
    //    printf("\n");
    }

    if (!locate_frame(&rx_up)) {
        return;
    }

    switch (rx_up.cmd) {
       case CHAIN_CMD_SYNC:
            proc_sync(rx_up.data);
            break;
        case CHAIN_CMD_LIGHT:
            break;
        default:
            break;
    }
    buf_consume_cmd(&rx_up);
}

static void proc_report(uint8_t *data)
{
    memcpy(report_cache + 2, data, 6);
}

static void proc_from_downstream()
{
    poll_data(&rx_down, UART_DOWN);

    if (!locate_frame(&rx_down)) {
        return;
    }

    switch (rx_down.cmd) {
       case CHAIN_CMD_REPORT:
            proc_report(rx_down.data);
            break;
        default:
            break;
    }
    buf_consume_cmd(&rx_down);
}

static void proc_usb()
{
    if (time_us_64() < bishi_runtime.sync_expire) {
        return;
    }

    bishi_runtime.chain_id = tud_mounted() ? 1 : 0;
}

static void being_node()
{
    send_frame(UART_UP, CHAIN_CMD_REPORT, report_cache, 8);
}

static void being_master()
{
    if (bishi_runtime.chain_id != 1) {
        return;
    }

    /* Send sync to downstream */
    static uint64_t next_sync = 0;
    uint64_t now = time_us_64();
    if (now >= next_sync) {
        uint8_t data[] = { 2, bishi_cfg->theme, bishi_cfg->spin.units_per_turn };
        send_frame(UART_DOWN, CHAIN_CMD_SYNC, data, sizeof(data));
        next_sync = now + SYNC_INTERVAL_US;
    }
}

void chain_update()
{
    proc_from_upstream();
    proc_from_downstream();
    proc_usb();
    being_node();
    being_master();
}