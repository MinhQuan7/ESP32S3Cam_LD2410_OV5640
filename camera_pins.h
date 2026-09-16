#pragma once

// =============================================================================
// Camera pinout for ESP32S3 Camera boards
// Default: CAMERA_MODEL_ESP32S3_EYE (Freenove / common ESP32-S3 Cam + OV5640)
// Uncomment exactly ONE model below if your board differs.
// =============================================================================

#define CAMERA_MODEL_ESP32S3_EYE
// #define CAMERA_MODEL_XIAO_ESP32S3
// #define CAMERA_MODEL_WAVESHARE_ESP32S3_CAM  // placeholder — fill pins from your schematic

#if defined(CAMERA_MODEL_ESP32S3_EYE)
#define PWDN_GPIO_NUM  -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM  15
#define SIOD_GPIO_NUM  4
#define SIOC_GPIO_NUM  5

#define Y2_GPIO_NUM 11
#define Y3_GPIO_NUM 9
#define Y4_GPIO_NUM 8
#define Y5_GPIO_NUM 10
#define Y6_GPIO_NUM 12
#define Y7_GPIO_NUM 18
#define Y8_GPIO_NUM 17
#define Y9_GPIO_NUM 16

#define VSYNC_GPIO_NUM 6
#define HREF_GPIO_NUM  7
#define PCLK_GPIO_NUM  13

#elif defined(CAMERA_MODEL_XIAO_ESP32S3)
#define PWDN_GPIO_NUM  -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM  10
#define SIOD_GPIO_NUM  40
#define SIOC_GPIO_NUM  39

#define Y9_GPIO_NUM 48
#define Y8_GPIO_NUM 11
#define Y7_GPIO_NUM 12
#define Y6_GPIO_NUM 14
#define Y5_GPIO_NUM 16
#define Y4_GPIO_NUM 18
#define Y3_GPIO_NUM 17
#define Y2_GPIO_NUM 15

#define VSYNC_GPIO_NUM 38
#define HREF_GPIO_NUM  47
#define PCLK_GPIO_NUM  13

#elif defined(CAMERA_MODEL_WAVESHARE_ESP32S3_CAM)
// TODO: replace with official Waveshare schematic pins before use
#error "CAMERA_MODEL_WAVESHARE_ESP32S3_CAM pins not set — use schematic or ESP32S3_EYE if compatible"

#else
#error "Camera model not selected in camera_pins.h"
#endif
