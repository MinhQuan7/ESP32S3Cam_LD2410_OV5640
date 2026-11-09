#include "supabase_uploader.h"

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <atomic>
#include <cstring>
#include <time.h>

#include "esp_heap_caps.h"
#include "img_converters.h"

#include "config.h"
#include "net_wifi.h"
#include "upload_schedule.h"

#if UPLOAD_ENABLED
#include "secrets.h"
#include "supabase_ca.h"
#endif

enum : uint8_t {
  UPL_IDLE = 0,
  UPL_BUSY = 1,
};

#if UPLOAD_ENABLED

static constexpr size_t kSnapPixels = (size_t)TFT_WIDTH * (size_t)TFT_HEIGHT;
static constexpr size_t kSnapBytes = kSnapPixels * sizeof(uint16_t);

static std::atomic<uint8_t> s_state{UPL_IDLE};
static uint8_t *s_rgb = nullptr;
static uint16_t s_w = 0;
static uint16_t s_h = 0;
static TaskHandle_t s_task = nullptr;
static portMUX_TYPE s_schedMux = portMUX_INITIALIZER_UNLOCKED;
static UploadSchedule s_sched;
static uint32_t s_bootId = 0;
static uint32_t s_seq = 0;

static uint32_t s_okCount = 0;
static uint32_t s_failCount = 0;
static int s_lastHttp = 0;
static size_t s_lastJpgBytes = 0;
static uint32_t s_lastMs = 0;
static bool s_ready = false;

// Espressif default Mozilla-derived CRT bundle (Arduino-ESP32 / IDF).
extern const uint8_t rootca_crt_bundle_start[] asm("_binary_x509_crt_bundle_start");
extern const uint8_t rootca_crt_bundle_end[] asm("_binary_x509_crt_bundle_end");

static bool keyLooksLikeJwt(const char *key) {
  return key && key[0] == 'e' && key[1] == 'y' && key[2] == 'J';
}

static void configureTls(WiFiClientSecure &client) {
  // Prefer Espressif CRT bundle (many roots). Fall back to ISRG Root X1.
#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
  const size_t bundleLen =
      (size_t)(rootca_crt_bundle_end - rootca_crt_bundle_start);
  if (bundleLen > 0) {
    client.setCACertBundle(rootca_crt_bundle_start, bundleLen);
  } else {
    client.setCACert(SUPABASE_CA_ISRG_ROOT_X1);
  }
#else
  (void)rootca_crt_bundle_start;
  (void)rootca_crt_bundle_end;
  client.setCACert(SUPABASE_CA_ISRG_ROOT_X1);
#endif
  // WiFiClientSecure::setTimeout is seconds on Arduino-ESP32.
  client.setTimeout((UPLOAD_HTTP_TIMEOUT_MS + 999) / 1000);
}

static void buildObjectPath(char *out, size_t outLen) {
  ++s_seq;
  if (netTimeSynced()) {
    time_t now = time(nullptr);
    struct tm tmUtc;
    gmtime_r(&now, &tmUtc);
    snprintf(out, outLen, "%s/%04d%02d%02d/%02d%02d%02d_%lu.jpg", DEVICE_ID,
             tmUtc.tm_year + 1900, tmUtc.tm_mon + 1, tmUtc.tm_mday,
             tmUtc.tm_hour, tmUtc.tm_min, tmUtc.tm_sec,
             (unsigned long)s_seq);
  } else {
    snprintf(out, outLen, "%s/nosync/%08lx_%lu.jpg", DEVICE_ID,
             (unsigned long)s_bootId, (unsigned long)millis());
  }
}

#if UPLOAD_SWAP_BYTES
static void swapRgb565Bytes(uint8_t *buf, size_t nbytes) {
  for (size_t i = 0; i + 1 < nbytes; i += 2) {
    uint8_t tmp = buf[i];
    buf[i] = buf[i + 1];
    buf[i + 1] = tmp;
  }
}
#endif

/** Accept "host", "https://host", "https://host/" from secrets.h. */
static const char *normalizeSupabaseHost(void) {
  const char *host = SUPABASE_HOST;
  if (strncmp(host, "https://", 8) == 0) {
    host += 8;
  } else if (strncmp(host, "http://", 7) == 0) {
    host += 7;
  }
  return host;
}

static int postJpeg(const uint8_t *jpg, size_t jpgLen, const char *objectPath) {
  const char *host = normalizeSupabaseHost();
  // Drop trailing slash on host if present.
  char hostBuf[96];
  size_t hostLen = strnlen(host, sizeof(hostBuf) - 1);
  while (hostLen > 0 && host[hostLen - 1] == '/') {
    --hostLen;
  }
  if (hostLen == 0 || hostLen >= sizeof(hostBuf)) {
    Serial.println("[UPL] bad SUPABASE_HOST");
    return -1;
  }
  memcpy(hostBuf, host, hostLen);
  hostBuf[hostLen] = '\0';

  char uri[160];
  snprintf(uri, sizeof(uri), "/storage/v1/object/%s/%s", SUPABASE_BUCKET,
           objectPath);

  WiFiClientSecure client;
  configureTls(client);

  HTTPClient http;
  http.setTimeout(UPLOAD_HTTP_TIMEOUT_MS);
  http.setReuse(false);

  // Explicit host/port/uri avoids URL-parse failures (e.g. doubled https://).
  if (!http.begin(client, hostBuf, (uint16_t)443, uri, true)) {
    Serial.printf("[UPL] http.begin failed host=%s uri=%s\n", hostBuf, uri);
    return -1;
  }

  http.addHeader("Content-Type", "image/jpeg");
  http.addHeader("apikey", SUPABASE_KEY);
  http.addHeader("x-upsert", "false");
  if (keyLooksLikeJwt(SUPABASE_KEY)) {
    char auth[256];
    snprintf(auth, sizeof(auth), "Bearer %s", SUPABASE_KEY);
    http.addHeader("Authorization", auth);
  }

  Serial.printf("[UPL] POST %u bytes -> %s/%s\n", (unsigned)jpgLen,
                SUPABASE_BUCKET, objectPath);

  const int code = http.sendRequest("POST", const_cast<uint8_t *>(jpg), jpgLen);

  if (code <= 0) {
    Serial.printf("[UPL] transport error code=%d\n", code);
  } else if (code != 200) {
    String body = http.getString();
    if (body.length() > 200) {
      body = body.substring(0, 200);
    }
    Serial.printf("[UPL] HTTP %d body=%s\n", code, body.c_str());
  } else {
    Serial.printf("[UPL] OK HTTP 200 jpg=%u\n", (unsigned)jpgLen);
  }

  http.end();
  client.stop();
  return code;
}

static void applyResult(bool ok, uint32_t nowMs) {
  portENTER_CRITICAL(&s_schedMux);
  uploadScheduleOnResult(&s_sched, ok, nowMs, (uint32_t)UPLOAD_RETRY_MIN_MS,
                         (uint32_t)UPLOAD_PERIOD_MS);
  portEXIT_CRITICAL(&s_schedMux);
}

static void uploaderTask(void *arg) {
  (void)arg;
  for (;;) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    const uint32_t t0 = millis();
    uint8_t *jpg = nullptr;
    size_t jpgLen = 0;
    bool ok = false;
    int httpCode = 0;

    if (!s_rgb || s_w == 0 || s_h == 0) {
      Serial.println("[UPL] empty snapshot");
      applyResult(false, millis());
      s_failCount++;
      s_lastHttp = -2;
      s_lastMs = millis() - t0;
      s_state.store(UPL_IDLE, std::memory_order_release);
      continue;
    }

    if (!fmt2jpg(s_rgb, kSnapBytes, s_w, s_h, PIXFORMAT_RGB565,
                 (uint8_t)UPLOAD_JPEG_QUALITY, &jpg, &jpgLen) ||
        !jpg || jpgLen == 0) {
      Serial.println("[UPL] fmt2jpg failed");
      if (jpg) {
        free(jpg);
        jpg = nullptr;
      }
      applyResult(false, millis());
      s_failCount++;
      s_lastHttp = -3;
      s_lastJpgBytes = 0;
      s_lastMs = millis() - t0;
      s_state.store(UPL_IDLE, std::memory_order_release);
      continue;
    }

    s_lastJpgBytes = jpgLen;

    if (!netIsUp()) {
      Serial.println("[UPL] WiFi down before POST");
      free(jpg);
      applyResult(false, millis());
      s_failCount++;
      s_lastHttp = -4;
      s_lastMs = millis() - t0;
      s_state.store(UPL_IDLE, std::memory_order_release);
      continue;
    }

    char path[96];
    buildObjectPath(path, sizeof(path));
    httpCode = postJpeg(jpg, jpgLen, path);
    free(jpg);
    jpg = nullptr;

    ok = (httpCode == 200);
    applyResult(ok, millis());
    if (ok) {
      s_okCount++;
    } else {
      s_failCount++;
    }
    s_lastHttp = httpCode;
    s_lastMs = millis() - t0;
    s_state.store(UPL_IDLE, std::memory_order_release);
  }
}

bool uploaderInit(void) {
  if (s_ready) {
    return true;
  }

  uploadScheduleInit(&s_sched);
  s_bootId = (uint32_t)esp_random();

  s_rgb = (uint8_t *)heap_caps_malloc(kSnapBytes, MALLOC_CAP_SPIRAM);
  if (!s_rgb) {
    Serial.println("[UPL] PSRAM alloc failed for snapshot buffer");
    return false;
  }
  memset(s_rgb, 0, kSnapBytes);

  BaseType_t created =
      xTaskCreatePinnedToCore(uploaderTask, "upl", UPLOAD_TASK_STACK, nullptr,
                              UPLOAD_TASK_PRIO, &s_task, UPLOAD_TASK_CORE);
  if (created != pdPASS || !s_task) {
    Serial.println("[UPL] task create failed");
    heap_caps_free(s_rgb);
    s_rgb = nullptr;
    return false;
  }

  s_ready = true;
  Serial.printf(
      "[UPL] ready period=%ums settle=%ums jpeg_q=%d core=%d stack=%d\n",
      (unsigned)UPLOAD_PERIOD_MS, (unsigned)UPLOAD_SETTLE_MS,
      (int)UPLOAD_JPEG_QUALITY, (int)UPLOAD_TASK_CORE, (int)UPLOAD_TASK_STACK);
  return true;
}

bool uploaderIsDue(uint32_t nowMs) {
  if (!s_ready) {
    return false;
  }
  if (s_state.load(std::memory_order_acquire) != UPL_IDLE) {
    return false;
  }
  if (!netIsUp()) {
    return false;
  }

  UploadSchedule snap;
  portENTER_CRITICAL(&s_schedMux);
  snap = s_sched;
  portEXIT_CRITICAL(&s_schedMux);
  return uploadScheduleDue(&snap, nowMs, (uint32_t)UPLOAD_PERIOD_MS);
}

bool uploaderSubmitRGB565(const camera_fb_t *fb) {
  if (!s_ready || !fb || !fb->buf || !s_rgb) {
    return false;
  }
  if (fb->format != PIXFORMAT_RGB565) {
    return false;
  }
  if (fb->width != TFT_WIDTH || fb->height != TFT_HEIGHT) {
    Serial.printf("[UPL] unexpected size %dx%d\n", fb->width, fb->height);
    return false;
  }
  if (fb->len < kSnapBytes) {
    Serial.printf("[UPL] fb too small len=%u need=%u\n", (unsigned)fb->len,
                  (unsigned)kSnapBytes);
    return false;
  }

  uint8_t expected = UPL_IDLE;
  if (!s_state.compare_exchange_strong(expected, UPL_BUSY,
                                       std::memory_order_acq_rel)) {
    return false;
  }

  memcpy(s_rgb, fb->buf, kSnapBytes);
#if UPLOAD_SWAP_BYTES
  swapRgb565Bytes(s_rgb, kSnapBytes);
#endif
  s_w = (uint16_t)fb->width;
  s_h = (uint16_t)fb->height;

  xTaskNotifyGive(s_task);
  return true;
}

void uploaderPrintStatus(void) {
  UploadSchedule snap;
  portENTER_CRITICAL(&s_schedMux);
  snap = s_sched;
  portEXIT_CRITICAL(&s_schedMux);

  Serial.printf(
      "[UPL] state=%s ok=%lu fail=%lu http=%d jpg=%u lastMs=%lu "
      "everOk=%d backoff=%lu wifi=%d rssi=%d ntp=%d\n",
      s_state.load(std::memory_order_relaxed) == UPL_BUSY ? "BUSY" : "IDLE",
      (unsigned long)s_okCount, (unsigned long)s_failCount, s_lastHttp,
      (unsigned)s_lastJpgBytes, (unsigned long)s_lastMs, (int)snap.everOk,
      (unsigned long)snap.backoffMs, (int)netIsUp(), netRssi(),
      (int)netTimeSynced());
}

#else // !UPLOAD_ENABLED

bool uploaderInit(void) { return true; }
bool uploaderIsDue(uint32_t) { return false; }
bool uploaderSubmitRGB565(const camera_fb_t *) { return false; }
void uploaderPrintStatus(void) {
  Serial.println("[UPL] disabled (UPLOAD_ENABLED=0)");
}

#endif
