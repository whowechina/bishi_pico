/*
 * Daisy Chain Ops
 * WHowe <github.com/whowechina>
 */

#ifndef CHAIN_H
#define CHAIN_H

#include <stdint.h>

void chain_init();
void chain_update();
void chain_usb_connected();
void chain_report_self(uint8_t button, uint8_t spin);

const uint8_t *chain_report();

#endif
