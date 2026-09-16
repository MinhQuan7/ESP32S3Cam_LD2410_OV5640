#include "display_st7789.h"

#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include "config.h"

static Arduino_ESP32SPI s_bus(
    TFT_DC,
    TFT_CS,
    TFT_SCLK,
    TFT_MOSI,
    GFX_NOT_DEFINED /* MISO */);

static Arduino_ST7789 s_gfx(
    &s_bus,
    TFT_RST,
    TFT_ROTATION,
    TFT_IPS,
    TFT_WIDTH,
    TFT_HEIGHT);

static bool s_inited = false;
static bool s_awake = false;

void displayBacklight(bool on) {
  const int level = on ? TFT_BL_ACTIVE_LEVEL : (TFT_BL_ACTIVE_LEVEL == HIGH ? LOW : HIGH);
  digitalWrite(TFT_BL, level);
}

bool displayInit() {
  if (s_inited) {
    return true;
  }

  pinMode(TFT_BL, OUTPUT);
  displayBacklight(true);

  s_gfx.begin(TFT_SPI_FREQ_HZ);

  s_gfx.setRotation(TFT_ROTATION);
  s_inited = true;
  s_awake = true;
  Serial.printf("[TFT] ST7789 ready %dx%d spi=%uHz pins SCL=%d SDA=%d DC=%d RST=%d CS=%d BL=%d\n",
                TFT_WIDTH, TFT_HEIGHT, (unsigned)TFT_SPI_FREQ_HZ,
                TFT_SCLK, TFT_MOSI, TFT_DC, TFT_RST, TFT_CS, TFT_BL);
  return true;
}

void displayShowRGB565(const uint16_t *buf, int16_t w, int16_t h) {
  if (!s_inited || !buf || w <= 0 || h <= 0) {
    return;
  }
  if (!s_awake) {
    displayWake();
  }

  const int16_t x = (TFT_WIDTH - w) / 2;
  const int16_t y = (TFT_HEIGHT - h) / 2;

#if TFT_RGB565_BIG_ENDIAN
  s_gfx.draw16bitBeRGBBitmap(x, y, const_cast<uint16_t *>(buf), w, h);
#else
  s_gfx.draw16bitRGBBitmap(x, y, const_cast<uint16_t *>(buf), w, h);
#endif
}

void displaySleep() {
  if (!s_inited || !s_awake) {
    displayBacklight(false);
    s_awake = false;
    return;
  }
  s_gfx.displayOff();
  displayBacklight(false);
  s_awake = false;
  Serial.println("[TFT] sleep (SLPIN + BL off)");
}

void displayWake() {
  if (!s_inited) {
    return;
  }
  displayBacklight(true);
  if (!s_awake) {
    s_gfx.displayOn();
    s_awake = true;
    Serial.println("[TFT] wake (SLPOUT + BL on)");
  }
}

void displaySplash(const char *msg) {
  if (!s_inited) {
    return;
  }
  if (!s_awake) {
    displayWake();
  }
  s_gfx.fillScreen(RGB565_BLACK);
  s_gfx.setTextColor(RGB565_WHITE);
  s_gfx.setTextSize(2);
  s_gfx.setCursor(16, 88);
  s_gfx.println("DTG CAM");
  s_gfx.setTextSize(1);
  s_gfx.setCursor(16, 120);
  s_gfx.println(msg ? msg : "waiting...");
}

bool displayIsAwake() {
  return s_awake;
}
