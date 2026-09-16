#pragma once

#include <stdbool.h>
#include <stdint.h>

bool displayInit();
void displayShowRGB565(const uint16_t *buf, int16_t w, int16_t h);
void displaySleep();
void displayWake();
void displayBacklight(bool on);
void displaySplash(const char *msg);
bool displayIsAwake();
