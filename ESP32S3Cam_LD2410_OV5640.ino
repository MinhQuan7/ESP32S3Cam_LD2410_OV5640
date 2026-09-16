/**
 * ESP32S3 + OV5640 + LD2410 + ST7789 240x240
 *
 * Presence live view: ld2410 UART presenceDetected() wakes camera+panel,
 * streams RGB565 240x240 to ST7789 while present. On absence: camera deinit,
 * panel sleep, backlight off.
 *
 * Wiring (ESP32-S3-EYE / N16R8 — free GPIOs):
 *   ST7789 SCL=14  SDA=21  DC=48  RST=47  CS=38  BL=39  VCC=3V3  GND=GND
 *   LD2410 TX->41  RX->42  VCC=5V/3V3  GND=GND
 *
 * Arduino IDE:
 *   - Tools → PSRAM → OPI PSRAM, Flash Size → 16MB
 *   - Libraries: GFX Library for Arduino, ld2410
 *
 * Bring-up:
 *   1) Serial should print LD2410 firmware version. If "not connected", check TX/RX.
 *   2) Splash on boot, snapshot on presence, black+BL off on absence.
 *   3) If colors look swapped, set TFT_RGB565_BIG_ENDIAN 0 in config.h
 */

#include <ld2410.h>
#include "config.h"
#include "camera_init.h"
#include "display_st7789.h"

enum AppState : uint8_t {
  STATE_ASLEEP = 0,
  STATE_WAKING,
  STATE_AWAKE,
  STATE_SLEEPING,
};

static ld2410 radar;
static AppState state = STATE_ASLEEP;
static uint32_t lastCaptureMs = 0;
static uint32_t nextWakeAttemptMs = 0;
static uint32_t lastStatusMs = 0;
static uint32_t wakeCycleCount = 0;
static uint32_t captureOkCount = 0;
static uint32_t fpsWindowCount = 0;
static uint32_t fpsWindowStartMs = 0;
static uint32_t lastFps = 0;

static bool s_present = false;
static bool s_pending = false;
static uint32_t s_pendingSinceMs = 0;

static const char *stateName(AppState s) {
  switch (s) {
    case STATE_ASLEEP:
      return "ASLEEP";
    case STATE_WAKING:
      return "WAKING";
    case STATE_AWAKE:
      return "AWAKE";
    case STATE_SLEEPING:
      return "SLEEPING";
    default:
      return "?";
  }
}

static void serialBanner(const char *msg) {
  Serial.println();
  Serial.println(msg);
  Serial.flush();
}

static bool radarRawPresent() {
  if (!radar.isConnected()) {
    return false;
  }
  const int detected = radar.presenceDetected() ? 1 : 0;
  return detected == LD2410_PRESENT_LEVEL;
}

static void presenceInit() {
  Serial1.begin(LD2410_BAUD, SERIAL_8N1, LD2410_RX_PIN, LD2410_TX_PIN);
  delay(500);
  Serial.printf("[RADAR] UART Serial1 RX=%d TX=%d baud=%u presentLevel=%d (1=person)\n",
                LD2410_RX_PIN, LD2410_TX_PIN, (unsigned)LD2410_BAUD,
                (int)LD2410_PRESENT_LEVEL);

  if (radar.begin(Serial1)) {
    Serial.printf("[RADAR] OK firmware %u.%u.%x\n",
                  (unsigned)radar.firmware_major_version,
                  (unsigned)radar.firmware_minor_version,
                  (unsigned)radar.firmware_bugfix_version);
  } else {
    Serial.println("[RADAR] not connected — check LD2410 TX->ESP RX, RX->ESP TX, 5V/GND");
  }

  const uint32_t t0 = millis();
  while (millis() - t0 < 200) {
    radar.read();
    delay(5);
  }

  s_present = radarRawPresent();
  s_pending = s_present;
  s_pendingSinceMs = millis();
  Serial.printf("[RADAR] initial present=%d connected=%d\n",
                (int)s_present, (int)radar.isConnected());
}

static void presenceUpdate() {
  radar.read();
  const bool wantPresent = radarRawPresent();
  const uint32_t now = millis();

  if (wantPresent != s_pending) {
    s_pending = wantPresent;
    s_pendingSinceMs = now;
    return;
  }

  const uint32_t needMs = s_pending ? (uint32_t)PRESENCE_CONFIRM_MS
                                    : (uint32_t)ABSENCE_CONFIRM_MS;
  if ((now - s_pendingSinceMs) < needMs) {
    return;
  }

  if (s_pending == s_present) {
    return;
  }

  s_present = s_pending;
  if (s_present) {
    Serial.println("[RADAR] CONFIRMED present");
  } else {
    Serial.println("[RADAR] CONFIRMED absent");
  }
}

static bool presenceIsPresent() {
  return s_present;
}

static void printStatus() {
  Serial.println("-------- STATUS --------");
  Serial.printf("state=%s present=%d connected=%d cam=%d tft=%d cycles=%lu fps=%lu frames=%lu\n",
                stateName(state),
                (int)presenceIsPresent(),
                (int)radar.isConnected(),
                (int)cameraIsReady(),
                (int)displayIsAwake(),
                (unsigned long)wakeCycleCount,
                (unsigned long)lastFps,
                (unsigned long)captureOkCount);
  if (radar.isConnected() && radar.presenceDetected()) {
    if (radar.stationaryTargetDetected()) {
      Serial.printf("stat %ucm e=%u  ",
                    (unsigned)radar.stationaryTargetDistance(),
                    (unsigned)radar.stationaryTargetEnergy());
    }
    if (radar.movingTargetDetected()) {
      Serial.printf("move %ucm e=%u",
                    (unsigned)radar.movingTargetDistance(),
                    (unsigned)radar.movingTargetEnergy());
    }
    Serial.println();
  }
  Serial.printf("heap=%u freePsram=%u uptime=%lu ms\n",
                ESP.getFreeHeap(),
                ESP.getFreePsram(),
                (unsigned long)millis());
  Serial.println("------------------------");
  Serial.flush();
}

static bool captureAndShow() {
  camera_fb_t *fb = nullptr;
  if (!cameraCaptureRGB565(&fb) || !fb) {
    Serial.println("[APP] capture failed");
    return false;
  }

  displayShowRGB565(reinterpret_cast<const uint16_t *>(fb->buf),
                    static_cast<int16_t>(fb->width),
                    static_cast<int16_t>(fb->height));
  cameraFbReturn(fb);

  lastCaptureMs = millis();
  captureOkCount++;
  fpsWindowCount++;
  const uint32_t elapsed = lastCaptureMs - fpsWindowStartMs;
  if (elapsed >= 1000) {
    lastFps = (fpsWindowCount * 1000UL) / elapsed;
    fpsWindowCount = 0;
    fpsWindowStartMs = lastCaptureMs;
  }
  return true;
}

static void enterAsleep(uint32_t retryDelayMs) {
  state = STATE_ASLEEP;
  nextWakeAttemptMs = millis() + retryDelayMs;
}

void setup() {
  Serial.begin(115200);
#if defined(ARDUINO_USB_CDC_ON_BOOT) && (ARDUINO_USB_CDC_ON_BOOT == 1)
  const uint32_t serialWaitStart = millis();
  while (!Serial && (millis() - serialWaitStart) < 5000) {
    delay(10);
  }
  delay(500);
#else
  delay(300);
#endif

  serialBanner("=== ESP32S3 presence snapshot ===");
#if defined(ARDUINO_USB_CDC_ON_BOOT) && (ARDUINO_USB_CDC_ON_BOOT == 1)
  Serial.println("[BOOT] Serial mode: USB CDC (use OTG port)");
#else
  Serial.println("[BOOT] Serial mode: UART0/CH340 (use TTL port)");
#endif
  Serial.printf("[BOOT] TFT %dx%d SPI %uHz\n",
                TFT_WIDTH, TFT_HEIGHT, (unsigned)TFT_SPI_FREQ_HZ);
  Serial.flush();

  if (!displayInit()) {
    Serial.println("[BOOT] FATAL: display init failed — halt");
    Serial.flush();
    while (true) {
      delay(2000);
    }
  }
  displaySplash("waiting...");
  presenceInit();

  if (presenceIsPresent()) {
    Serial.println("[BOOT] already present — wake on first loop");
    state = STATE_WAKING;
  } else {
    delay(600);
    displaySleep();
    state = STATE_ASLEEP;
  }

  printStatus();
  Serial.println("[BOOT] setup() done");
  Serial.flush();
}

void loop() {
  presenceUpdate();

  switch (state) {
    case STATE_ASLEEP:
      if (presenceIsPresent() && (int32_t)(millis() - nextWakeAttemptMs) >= 0) {
        state = STATE_WAKING;
        Serial.println("[APP] ASLEEP -> WAKING");
      }
      break;

    case STATE_WAKING:
      displayWake();
      displaySplash("capturing...");
      if (!cameraInit()) {
        Serial.println("[APP] camera init failed — retry later");
        displaySplash("cam fail");
        delay(400);
        displaySleep();
        enterAsleep(CAMERA_INIT_RETRY_MS);
        break;
      }
      if (!captureAndShow()) {
        Serial.println("[APP] first capture failed — retry later");
        cameraDeinit();
        displaySleep();
        enterAsleep(CAMERA_INIT_RETRY_MS);
        break;
      }
      wakeCycleCount++;
      fpsWindowCount = 0;
      fpsWindowStartMs = millis();
      lastFps = 0;
      state = STATE_AWAKE;
      Serial.printf("[APP] WAKING -> AWAKE live view cycle=%lu heap=%u psram=%u\n",
                    (unsigned long)wakeCycleCount,
                    ESP.getFreeHeap(), ESP.getFreePsram());
      break;

    case STATE_AWAKE:
      if (!presenceIsPresent()) {
        state = STATE_SLEEPING;
        Serial.printf("[APP] AWAKE -> SLEEPING frames=%lu fps=%lu\n",
                      (unsigned long)captureOkCount, (unsigned long)lastFps);
        break;
      }
      if (CAPTURE_REFRESH_MS == 0 ||
          (millis() - lastCaptureMs) >= (uint32_t)CAPTURE_REFRESH_MS) {
        captureAndShow();
      }
      break;

    case STATE_SLEEPING:
      cameraDeinit();
      displaySleep();
      enterAsleep(0);
      Serial.printf("[APP] SLEEPING -> ASLEEP heap=%u psram=%u\n",
                    ESP.getFreeHeap(), ESP.getFreePsram());
      break;
  }

  const uint32_t now = millis();
  if (now - lastStatusMs >= STATUS_LOG_INTERVAL_MS) {
    lastStatusMs = now;
    printStatus();
  }

  if (state != STATE_AWAKE) {
    delay(PRESENCE_POLL_MS);
  }
}
