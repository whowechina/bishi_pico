/*
 * The Spinning of Buttons
 * WHowe <github.com/whowechina>
 */

#ifndef SPIN_H
#define SPIN_H

#include <stdint.h>
#include <stdbool.h>
#include "hardware/flash.h"

void spin_init();
void spin_update();
uint16_t spin_read();
uint16_t spin_units();
bool spin_present();

#endif
