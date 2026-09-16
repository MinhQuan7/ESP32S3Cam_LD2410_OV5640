#include "camera_init.h"

#include <Arduino.h>
#include "config.h"
#include "camera_pins.h"

#ifndef BOARD_HAS_PSRAM
#error "PSRAM disabled in board options. Arduino IDE: Tools → PSRAM → OPI PSRAM (N16R8), Flash Size 16MB, then Upload again."
#endif

static bool s_ready = false;

static void printPsramDiag() {
  Serial.printf("[CAM] heap=%u freePsram=%u psramFound=%d\n",
                ESP.getFreeHeap(),
                ESP.getFreePsram(),
                psramFound() ? 1 : 0);
#if defined(CONFIG_SPIRAM_MODE_OCT)
  Serial.println("[CAM] build: CONFIG_SPIRAM_MODE_OCT (OPI) enabled");
#elif defined(CONFIG_SPIRAM_MODE_QUAD)
  Serial.println("[CAM] build: CONFIG_SPIRAM_MODE_QUAD — N16R8 needs OPI, not QSPI");
#else
  Serial.println("[CAM] build: SPIRAM mode macro not visible");
#endif
}

static void discardWarmupFrames() {
  for (int i = 0; i < CAMERA_WARMUP_FRAMES; ++i) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (fb) {
      esp_camera_fb_return(fb);
    }
  }
}

bool cameraInit() {
  if (s_ready) {
    return true;
  }

  printPsramDiag();
  if (!psramFound()) {
    Serial.println("[CAM] ERROR: PSRAM not found at runtime.");
    Serial.println("[CAM] Fix: Arduino IDE → Tools → PSRAM → OPI PSRAM, Flash Size → 16MB");
    return false;
  }

  camera_config_t config = {};
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;

  config.xclk_freq_hz = XCLK_FREQ_HZ;
  config.pixel_format = PIXFORMAT_RGB565;
  config.frame_size = FRAMESIZE_240X240;
  config.jpeg_quality = 12;
  config.fb_count = FB_COUNT;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.grab_mode = CAMERA_GRAB_LATEST;

  Serial.println("[CAM] calling esp_camera_init() RGB565 240x240...");
  Serial.flush();
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("[CAM] esp_camera_init failed: 0x%x\n", err);
    s_ready = false;
    return false;
  }

  sensor_t *s = esp_camera_sensor_get();
  if (!s) {
    Serial.println("[CAM] ERROR: sensor_t is null");
    esp_camera_deinit();
    return false;
  }

  Serial.printf("[CAM] sensor PID=0x%04x (OV5640 expect 0x5640)\n", s->id.PID);

  s->set_framesize(s, FRAMESIZE_240X240);
  s->set_vflip(s, CAMERA_VFLIP);
  s->set_hmirror(s, CAMERA_HMIRROR);
  s->set_brightness(s, 0);
  s->set_contrast(s, 0);
  s->set_saturation(s, 0);
  s->set_whitebal(s, 1);
  s->set_awb_gain(s, 1);
  s->set_exposure_ctrl(s, 1);
  s->set_aec2(s, 1);
  s->set_gain_ctrl(s, 1);

  discardWarmupFrames();

  s_ready = true;
  Serial.printf("[CAM] ready: RGB565 240x240 xclk=%uHz fb_count=%d freePsram=%u\n",
                (unsigned)XCLK_FREQ_HZ, FB_COUNT, ESP.getFreePsram());
  return true;
}

void cameraDeinit() {
  if (!s_ready) {
    return;
  }
  esp_err_t err = esp_camera_deinit();
  s_ready = false;
  if (err != ESP_OK) {
    Serial.printf("[CAM] deinit warn: 0x%x\n", err);
  } else {
    Serial.printf("[CAM] deinit OK heap=%u freePsram=%u\n",
                  ESP.getFreeHeap(), ESP.getFreePsram());
  }
}

bool cameraIsReady() {
  return s_ready;
}

bool cameraCaptureRGB565(camera_fb_t **outFb) {
  if (!outFb || !s_ready) {
    return false;
  }
  *outFb = nullptr;

  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("[CAM] fb_get failed");
    return false;
  }

  if (fb->format != PIXFORMAT_RGB565 || fb->width <= 0 || fb->height <= 0) {
    Serial.printf("[CAM] unexpected fb format=%d %dx%d len=%u\n",
                  (int)fb->format, fb->width, fb->height, (unsigned)fb->len);
    esp_camera_fb_return(fb);
    return false;
  }

  *outFb = fb;
  return true;
}

void cameraFbReturn(camera_fb_t *fb) {
  if (fb) {
    esp_camera_fb_return(fb);
  }
}
