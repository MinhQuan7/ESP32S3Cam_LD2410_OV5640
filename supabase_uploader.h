#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_camera.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Alloc PSRAM snapshot buffer + create uploader task on UPLOAD_TASK_CORE. */
bool uploaderInit(void);

/**
 * True when IDLE + WiFi up + schedule due.
 * Does NOT check UPLOAD_SETTLE_MS — caller gates settle separately.
 */
bool uploaderIsDue(uint32_t nowMs);

/**
 * Copy RGB565 frame into owned PSRAM buffer and notify uploader task.
 * Returns false if BUSY, not ready, or unexpected frame size/format.
 * Does not take ownership of fb (caller still returns it to the camera driver).
 */
bool uploaderSubmitRGB565(const camera_fb_t *fb);

/** Serial status line: ok/fail/http/jpg/lastMs/state. */
void uploaderPrintStatus(void);

#ifdef __cplusplus
}
#endif
