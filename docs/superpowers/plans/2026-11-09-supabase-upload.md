# Supabase Snapshot Upload Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Sau khi chụp, đẩy ảnh lên Supabase Storage, tối đa 1 ảnh mỗi 5 phút mỗi thiết bị và không làm giảm FPS của live view.

**Architecture:** Main loop (core 1) giữ nguyên pipeline camera -> TFT. Khi bộ lập lịch báo "đến hạn", main loop chỉ `memcpy` frame RGB565 vào buffer snapshot PSRAM riêng, rồi notify task uploader (core 0). Task uploader nén JPEG và POST HTTPS lên Storage REST.

**Tech Stack:** Arduino-ESP32 3.x, `esp_camera` + `img_converters.h` (`fmt2jpg`), `WiFi`, `NetworkClientSecure` + `HTTPClient`, FreeRTOS task notification, SNTP.

**Spec:** User request — period upload 5 phút, Storage only, cooldown, 240x240 JPEG, anon key + RLS.

## Global Constraints

- Không `delay()` / mạng trong nhánh `STATE_AWAKE`.
- Secrets chỉ trong `secrets.h` (gitignored). Không log key.
- Không dùng `service_role`.
- `UPLOAD_ENABLED 0` tắt toàn bộ tính năng.

## Review Focus

1. WiFi mất: không submit, không block, auto-reconnect.
2. Người rời đi khi đang upload: camera deinit OK, upload từ buffer riêng.
3. Key/RLS sai (401/403): backoff nhân đôi, không spam.
4. `millis()` wraparound: lịch vẫn đúng.
5. Màu JPEG đảo byte: cờ `UPLOAD_SWAP_BYTES`.

See also the Cursor plan attachment for full task breakdown.
