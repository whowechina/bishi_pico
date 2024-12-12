/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "usb_descriptors.h"
#include "pico/unique_id.h"
#include "tusb.h"

tusb_desc_device_t desc_device_joy = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = 0x00,
    .bDeviceSubClass = 0x00,
    .bDeviceProtocol = 0x00,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,

    .idVendor = 0xcaee,
    .idProduct = 0x0021,
    .bcdDevice = 0x0100,

    .iManufacturer = 1,
    .iProduct = 2,
    .iSerialNumber = 3,

    .bNumConfigurations = 1
};

// Invoked when received GET DEVICE DESCRIPTOR
// Application return pointer to descriptor
uint8_t const* tud_descriptor_device_cb(void) {
    return (uint8_t const*)&desc_device_joy;
}

//--------------------------------------------------------------------+
// HID Report Descriptor
//--------------------------------------------------------------------+

uint8_t const desc_hid_report_joy[] = {
    BISHI_PICO_REPORT_DESC_JOYSTICK,
    BISHI_PICO_REPORT_DESC_LIGHTS,
};

// Invoked when received GET HID REPORT DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
uint8_t const* tud_hid_descriptor_report_cb(uint8_t itf)
{
    switch (itf) {
        case 0:
            return desc_hid_report_joy;
        default:
            return NULL;
    }
}
//--------------------------------------------------------------------+
// Configuration Descriptor
//--------------------------------------------------------------------+

enum { ITF_NUM_JOY, ITF_NUM_CLI, ITF_NUM_CLI_DATA, ITF_NUM_TOTAL };

#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + \
                          TUD_HID_DESC_LEN * 1 + \
                          TUD_CDC_DESC_LEN * 1)

#define EPNUM_JOY 0x81

#define EPNUM_CLI_NOTIF 0x89
#define EPNUM_CLI_OUT   0x0a
#define EPNUM_CLI_IN    0x8a

uint8_t const desc_configuration_joy[] = {
    // Config number, interface count, string index, total length, attribute,
    // power in mA
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN,
                          TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 200),

    // Interface number, string index, protocol, report descriptor len, EP In
    // address, size & polling interval
    TUD_HID_DESCRIPTOR(ITF_NUM_JOY, 4, HID_ITF_PROTOCOL_NONE,
                       sizeof(desc_hid_report_joy), EPNUM_JOY,
                       CFG_TUD_HID_EP_BUFSIZE, 1),
 
    TUD_CDC_DESCRIPTOR(ITF_NUM_CLI, 5, EPNUM_CLI_NOTIF,
                       8, EPNUM_CLI_OUT, EPNUM_CLI_IN, 64),
};

// Invoked when received GET CONFIGURATION DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
uint8_t const* tud_descriptor_configuration_cb(uint8_t index) {
    return desc_configuration_joy;
}

//--------------------------------------------------------------------+
// String Descriptors
//--------------------------------------------------------------------+
static char serial_number_str[24] = "\0";

void usb_descriptors_set_sn(uint64_t sn)
{
    sprintf(serial_number_str, "%16llx", sn);
}

static const char *string_desc_arr[] = {
    (const char[]){0x09, 0x04},  // 0: is supported language is English (0x0409)
    "WHowe",                     // 1: Manufacturer
    "Bishi Pico",                // 2: Product
    serial_number_str,           // 3: Serials, use chip ID
    "Bishi Pico",
    "Bishi Pico CLI Port",
    "Bishi A 1 R",
    "Bishi A 1 G",
    "Bishi A 1 B",
    "Bishi A 2 R",
    "Bishi A 2 G",
    "Bishi A 2 B",
    "Bishi A 3 R",
    "Bishi A 3 G",
    "Bishi A 3 B",
    "Bishi A 4 R",
    "Bishi A 4 G",
    "Bishi A 4 B",
    "Bishi A Cab R",
    "Bishi A Cab G",
    "Bishi A Cab B",
    "Bishi B 1 R",
    "Bishi B 1 G",
    "Bishi B 1 B",
    "Bishi B 2 R",
    "Bishi B 2 G",
    "Bishi B 2 B",
    "Bishi B 3 R",
    "Bishi B 3 G",
    "Bishi B 3 B",
    "Bishi B 4 R",
    "Bishi B 4 G",
    "Bishi B 4 B",
    "Bishi B Cab R",
    "Bishi B Cab G",
    "Bishi B Cab B",
    "Bishi C 1 R",
    "Bishi C 1 G",
    "Bishi C 1 B",
    "Bishi C 2 R",
    "Bishi C 2 G",
    "Bishi C 2 B",
    "Bishi C 3 R",
    "Bishi C 3 G",
    "Bishi C 3 B",
    "Bishi C 4 R",
    "Bishi C 4 G",
    "Bishi C 4 B",
    "Bishi C Cab R",
    "Bishi C Cab G",
    "Bishi C Cab B",
    "Bishi D 1 R",
    "Bishi D 1 G",
    "Bishi D 1 B",
    "Bishi D 2 R",
    "Bishi D 2 G",
    "Bishi D 2 B",
    "Bishi D 3 R",
    "Bishi D 3 G",
    "Bishi D 3 B",
    "Bishi D 4 R",
    "Bishi D 4 G",
    "Bishi D 4 B",
    "Bishi D Cab R",
    "Bishi D Cab G",
    "Bishi D Cab B",
};

// Invoked when received GET STRING DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long
// enough for transfer to complete
uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    static uint16_t _desc_str[128];

    if (index == 0) {
        memcpy(&_desc_str[1], string_desc_arr[0], 2);
        _desc_str[0] = (TUSB_DESC_STRING << 8) | (2 + 2);
        return _desc_str;
    }

    if ((index == 3) && (serial_number_str[0] == '\0')) {
        pico_unique_board_id_t board_id;
        pico_get_unique_board_id(&board_id);
        sprintf(serial_number_str, "%16llx", *(uint64_t *)&board_id);
    }

    const char *str = string_desc_arr[index];
    uint8_t chr_count = strlen(str);
    if (chr_count > count_of(_desc_str)) {
        chr_count = count_of(_desc_str);
    }

    // Convert ASCII string into UTF-16
    for (uint8_t i = 0; i < chr_count; i++) {
        _desc_str[1 + i] = str[i];
    }

    // first byte is length (including header), second byte is string type
    _desc_str[0] = (TUSB_DESC_STRING << 8) | (2 * chr_count + 2);

    return _desc_str;
}
