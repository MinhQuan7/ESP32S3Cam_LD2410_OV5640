#include "net_wifi.h"

#include <Arduino.h>
#include <WiFi.h>
#include <time.h>

#include "config.h"

#if UPLOAD_ENABLED
#include "secrets.h"
#endif

void netBegin(void) {
#if !UPLOAD_ENABLED
  return;
#else
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(false);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  configTime(0, 0, "pool.ntp.org", "time.google.com");
  Serial.printf("[NET] WiFi begin ssid=%s (non-blocking)\n", WIFI_SSID);
#endif
}

bool netIsUp(void) {
#if !UPLOAD_ENABLED
  return false;
#else
  return WiFi.status() == WL_CONNECTED;
#endif
}

bool netTimeSynced(void) {
#if !UPLOAD_ENABLED
  return false;
#else
  time_t now = time(nullptr);
  // 1700000000 ~= 2023-11-14 UTC — anything before that is unset/boot epoch.
  return now > (time_t)1700000000;
#endif
}

int netRssi(void) {
#if !UPLOAD_ENABLED
  return 0;
#else
  if (WiFi.status() != WL_CONNECTED) {
    return 0;
  }
  return WiFi.RSSI();
#endif
}
