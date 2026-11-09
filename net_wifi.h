#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Start STA + SNTP non-blocking. Safe to call once from setup(). */
void netBegin(void);

/** True when WiFi.status() == WL_CONNECTED. */
bool netIsUp(void);

/** True when SNTP has set a plausible UTC epoch (> 2023-11). */
bool netTimeSynced(void);

/** RSSI in dBm, or 0 if not connected. */
int netRssi(void);

#ifdef __cplusplus
}
#endif
