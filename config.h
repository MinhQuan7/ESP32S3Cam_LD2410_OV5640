#pragma once

// =============================================================================
// ESP32S3 + OV5640 + LD2410 + ST7789 — presence snapshot
// Pins are the free set on ESP32-S3-EYE / N16R8 (camera + OPI PSRAM reserved).
// LD2410 uses UART (library ld2410) on Serial1 @ 256000. Wiring:
//   LD2410 TX -> LD2410_RX_PIN, LD2410 RX -> LD2410_TX_PIN
// =============================================================================

// --- ST7789 240x240 SPI ------------------------------------------------------
#ifndef TFT_SCLK
#define TFT_SCLK 14
#endif

#ifndef TFT_MOSI
#define TFT_MOSI 21
#endif

#ifndef TFT_DC
#define TFT_DC 48
#endif

#ifndef TFT_RST
#define TFT_RST 47
#endif

#ifndef TFT_CS
#define TFT_CS 38
#endif

#ifndef TFT_BL
#define TFT_BL 39
#endif

#ifndef TFT_BL_ACTIVE_LEVEL
#define TFT_BL_ACTIVE_LEVEL HIGH
#endif

#ifndef TFT_WIDTH
#define TFT_WIDTH 240
#endif

#ifndef TFT_HEIGHT
#define TFT_HEIGHT 240
#endif

#ifndef TFT_ROTATION
#define TFT_ROTATION 0
#endif

#ifndef TFT_IPS
#define TFT_IPS true
#endif

#ifndef TFT_SPI_FREQ_HZ
#define TFT_SPI_FREQ_HZ 40000000
#endif

// true: camera RGB565 is big-endian — use draw16bitBeRGBBitmap
// Set false if colors look swapped / inverted after first flash
#ifndef TFT_RGB565_BIG_ENDIAN
#define TFT_RGB565_BIG_ENDIAN 1
#endif

// --- LD2410 UART (Serial1, 256000 8N1) ---------------------------------------
// Free GPIOs after camera + ST7789. ESP RX <- radar TX, ESP TX -> radar RX.
#ifndef LD2410_RX_PIN
#define LD2410_RX_PIN 41
#endif

#ifndef LD2410_TX_PIN
#define LD2410_TX_PIN 42
#endif

#ifndef LD2410_BAUD
#define LD2410_BAUD 256000
#endif

// 1 = có người (wake/capture), 0 = không có người (sleep).
// Chỉ đổi thành 0 nếu Serial cho thấy presence bị đảo.
#ifndef LD2410_PRESENT_LEVEL
#define LD2410_PRESENT_LEVEL 1
#endif

#ifndef PRESENCE_CONFIRM_MS
#define PRESENCE_CONFIRM_MS 80
#endif

#ifndef ABSENCE_CONFIRM_MS
#define ABSENCE_CONFIRM_MS 15000
#endif

#ifndef PRESENCE_POLL_MS
#define PRESENCE_POLL_MS 10
#endif

// Min interval between frames while present. 0 = as fast as the camera+SPI allow.
#ifndef CAPTURE_REFRESH_MS
#define CAPTURE_REFRESH_MS 0
#endif

#ifndef CAMERA_INIT_RETRY_MS
#define CAMERA_INIT_RETRY_MS 2000
#endif

#ifndef CAMERA_WARMUP_FRAMES
#define CAMERA_WARMUP_FRAMES 2
#endif

#ifndef STATUS_LOG_INTERVAL_MS
#define STATUS_LOG_INTERVAL_MS 5000
#endif

// --- Camera ------------------------------------------------------------------
#ifndef XCLK_FREQ_HZ
#define XCLK_FREQ_HZ 20000000
#endif

#ifndef FB_COUNT
#define FB_COUNT 2
#endif

#ifndef CAMERA_VFLIP
#define CAMERA_VFLIP 1
#endif

#ifndef CAMERA_HMIRROR
#define CAMERA_HMIRROR 0
#endif
