# ESP32-S3 CAM + OV5640 + LD2410 + ST7789

Firmware **presence live view**: radar LD2410 phát hiện người thì bật camera + màn hình, stream RGB565 240×240 lên ST7789. Khi không còn người: tắt camera, sleep panel, tắt đèn nền.

Tài liệu này hướng dẫn **cài Arduino IDE, board ESP32, thư viện bắt buộc và nạp code** trên Windows.

---

## 1. Phần cứng

| Thành phần | Ghi chú |
|---|---|
| Board | ESP32-S3-EYE / N16R8 (Flash 16MB + OPI PSRAM 8MB) + camera OV5640 |
| Màn hình | ST7789 IPS 240×240, SPI |
| Radar | HLK-LD2410 (UART 256000 8N1) |
| Cáp | USB (cổng OTG hoặc cổng UART/CH340 tùy board) |

### 1.1. Đấu dây (GPIO trống trên ESP32-S3-EYE / N16R8)

**ST7789**

| ST7789 | ESP32-S3 |
|---|---|
| SCL (SCLK) | GPIO 14 |
| SDA (MOSI) | GPIO 21 |
| DC | GPIO 48 |
| RST | GPIO 47 |
| CS | GPIO 38 |
| BL | GPIO 39 |
| VCC | 3V3 |
| GND | GND |

**LD2410** (chéo TX/RX)

| LD2410 | ESP32-S3 |
|---|---|
| TX | GPIO 41 (`LD2410_RX_PIN` — ESP nhận) |
| RX | GPIO 42 (`LD2410_TX_PIN` — ESP gửi) |
| VCC | 5V (ưu tiên) hoặc 3V3 |
| GND | GND |

Pin camera OV5640 đã cố định theo board ESP32-S3-EYE trong `camera_pins.h`. Không đổi trừ khi dùng board khác.

---

## 2. Cài Arduino IDE

1. Tải **Arduino IDE 2.x** (khuyến nghị 2.3 trở lên):  
   [https://www.arduino.cc/en/software](https://www.arduino.cc/en/software)
2. Cài đặt, mở IDE.
3. (Windows) Nếu máy không nhận cổng COM:
   - Board cổng USB-C native / OTG: driver WinUSB thường có sẵn.
   - Board có chip CH340: cài driver CH340.
   - Board có CP2102: cài [Silicon Labs CP210x](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers).

Kiểm tra cổng: `Device Manager` → `Ports (COM & LPT)` → ghi nhớ số COM (ví dụ `COM7`).

---

## 3. Cài board ESP32 (Arduino-ESP32)

Camera (`esp_camera.h`) nằm **trong core ESP32**, không cài riêng từ Library Manager.

### 3.1. Thêm URL Board Manager

1. `File` → `Preferences` (hoặc `Arduino IDE` → `Settings` trên một số bản).
2. Ô **Additional boards manager URLs**, dán:

```
https://espressif.github.io/arduino-esp32/package_esp32_index.json
```

3. Bấm `OK`.

### 3.2. Cài package `esp32`

1. `Tools` → `Board` → `Boards Manager…`
2. Ô tìm kiếm: `esp32`
3. Chọn **esp32 by Espressif Systems**
4. Bấm `Install` (hoặc `Update`)

**Phiên bản khuyến nghị:** `3.0.x` hoặc mới hơn (có `FRAMESIZE_240X240`, ESP32-S3, OPI PSRAM).  
Nếu compile lỗi lạ với GFX, thử `2.0.17` — không khuyến nghị trừ khi đã xác nhận lỗi trên 3.x.

Sau khi cài xong, đóng/mở lại Arduino IDE.

---

## 4. Cài thư viện bắt buộc

Mở `Sketch` → `Include Library` → `Manage Libraries…` (Library Manager).

Cài **đúng 2 thư viện** sau. Không cần TFT_eSPI, Adafruit_ST7789, hay thư viện camera ngoài.

### 4.1. GFX Library for Arduino

| Mục | Giá trị |
|---|---|
| Tên tìm | `GFX Library for Arduino` |
| Tác giả | **Moononournation** |
| Header trong code | `#include <Arduino_GFX_Library.h>` |
| Dùng cho | ST7789 SPI 240×240 |

Bấm `Install`. Nếu IDE hỏi cài dependency, chọn **Install all**.

GitHub: [moononournation/Arduino_GFX](https://github.com/moononournation/Arduino_GFX)

### 4.2. ld2410

| Mục | Giá trị |
|---|---|
| Tên tìm | `ld2410` |
| Tác giả | **Nick Reynolds** (`ncmreynolds`) |
| Header trong code | `#include <ld2410.h>` |
| Dùng cho | HLK-LD2410 UART 256000 |

Bấm `Install`.

GitHub: [ncmreynolds/ld2410](https://github.com/ncmreynolds/ld2410)

### 4.3. Checklist thư viện

Trong `Sketch` → `Include Library`, phải thấy:

- [x] GFX Library for Arduino
- [x] ld2410
- [x] (có sẵn từ core) ESP32 / `esp_camera` — **không** cài package tên “ESP32 Camera” bên thứ ba

Có thể kiểm tra nhanh bằng cách mở file `.ino` rồi `Sketch` → `Verify/Compile`. Lỗi `ld2410.h: No such file` hoặc `Arduino_GFX_Library.h: No such file` = chưa cài đúng thư viện.

---

## 5. Mở project

1. `File` → `Open…`
2. Chọn file:

```
ESP32S3Cam_LD2410_OV5640.ino
```

Arduino IDE phải mở **cả thư mục** (các file `.h` / `.cpp` hiện thành tab). Nếu chỉ thấy 1 tab `.ino`, hãy mở đúng file trong thư mục project, không copy lẻ file `.ino` ra chỗ khác.

Cấu trúc bắt buộc:

```
ESP32S3Cam_LD2410_OV5640/
├── ESP32S3Cam_LD2410_OV5640.ino
├── config.h
├── secrets.h.example          # copy → secrets.h (gitignored)
├── camera_pins.h
├── camera_init.h / .cpp
├── display_st7789.h / .cpp
├── net_wifi.h / .cpp
├── supabase_uploader.h / .cpp
├── supabase_ca.h
├── upload_schedule.h
├── docs/supabase_setup.sql
└── tests/test_upload_schedule.cpp
```

---

## 6. Cấu hình Tools (bắt buộc đúng)

`Tools` → `Board` → `esp32` → **ESP32S3 Dev Module**

Sau đó chỉnh lần lượt:

| Menu Tools | Giá trị bắt buộc | Lý do |
|---|---|---|
| **Board** | `ESP32S3 Dev Module` | Chip ESP32-S3 |
| **USB CDC On Boot** | `Enabled` nếu nạp/log qua cổng USB OTG | Code chờ Serial CDC tối đa 5 giây |
| **USB CDC On Boot** | `Disabled` nếu nạp/log qua UART0 / CH340 | Dùng cổng TTL |
| **CPU Frequency** | `240MHz (WiFi)` | Camera + SPI cần xung cao |
| **Core Debug Level** | `None` (hoặc `Info` khi debug) | — |
| **USB DFU On Boot** | `Disabled` | — |
| **Flash Mode** | `QIO 80MHz` | Flash ngoài N16R8 |
| **Flash Size** | **`16MB (128Mb)`** | Sai size → PSRAM/partition lỗi |
| **Partition Scheme** | `16M Flash (3MB APP/9.9MB FATFS)` | Đủ chỗ app + camera |
| **PSRAM** | **`OPI PSRAM`** | **Bắt buộc.** QSPI/Disabled sẽ `#error` lúc compile hoặc camera fail lúc chạy |
| **Arduino Runs On** | `Core 1` | Mặc định ổn định |
| **Events Run On** | `Core 1` | — |
| **Upload Mode** | `UART0 / Hardware CDC` (theo cổng đang cắm) | Khớp cổng USB đang dùng |
| **Upload Speed** | `921600` (nếu fail thì `115200`) | — |
| **USB Mode** | `Hardware CDC and JTAG` khi dùng OTG | — |
| **Port** | COM của board (ví dụ `COM7`) | — |
| **Erase All Flash Before Sketch Upload** | `Disabled` (bật 1 lần nếu firmware cũ kẹt) | — |

### 6.1. Chọn đúng cổng USB

Firmware in ra lúc boot:

- `[BOOT] Serial mode: USB CDC (use OTG port)` → cắm **cổng USB OTG / native USB** của S3.
- `[BOOT] Serial mode: UART0/CH340 (use TTL port)` → cắm **cổng UART/CH340**.

Nếu mở Serial Monitor không thấy log: sai cổng hoặc sai `USB CDC On Boot`.

---

## 7. Compile và nạp code

### 7.1. Compile (kiểm tra trước)

1. `Sketch` → `Verify/Compile` (hoặc nút ✓).
2. Chờ hết `Done compiling`.

Lỗi thường gặp lúc compile:

| Thông báo | Cách xử lý |
|---|---|
| `PSRAM disabled in board options` | `Tools` → `PSRAM` → **OPI PSRAM**, compile lại |
| `ld2410.h: No such file or directory` | Cài thư viện **ld2410** (Nick Reynolds) |
| `Arduino_GFX_Library.h: No such file` | Cài **GFX Library for Arduino** (Moononournation) |
| `esp_camera.h: No such file` | Chưa chọn board `ESP32S3 Dev Module` / chưa cài package `esp32` |
| `FRAMESIZE_240X240` undeclared | Core ESP32 quá cũ — cập nhật Boards Manager |

### 7.2. Nạp (Upload)

1. Cắm USB, chọn đúng `Port`.
2. Đóng Serial Monitor / tool khác đang giữ COM (Putty, PlatformIO monitor…).
3. `Sketch` → `Upload` (hoặc nút →).
4. Đợi `Connecting...` rồi `Hard resetting via RTS pin...` / `Done uploading`.

Nếu kẹt ở `Connecting........_____`:

1. Giữ nút **BOOT** (IO0) trên board.
2. Bấm **RESET**, thả RESET, **vẫn giữ BOOT**.
3. Khi IDE bắt đầu ghi flash thì thả BOOT.
4. Hoặc hạ `Upload Speed` xuống `115200`.

Sau khi nạp xong, board tự reset. Mở Serial Monitor ngay.

---

## 8. Serial Monitor — kiểm tra bring-up

1. `Tools` → `Serial Monitor`
2. Baud: **115200**
3. Góc phải: `Both NL & CR` (không bắt buộc với firmware này)

Log boot đúng sẽ gần như:

```
=== ESP32S3 presence snapshot ===
[BOOT] Serial mode: ...
[BOOT] TFT 240x240 SPI 40000000Hz
[TFT] ST7789 ready 240x240 ...
[RADAR] UART Serial1 RX=41 TX=42 baud=256000 presentLevel=1
[RADAR] OK firmware x.y.z
[CAM] ... (khi có người)
```

### 8.1. Ý nghĩa log

| Log | Ý nghĩa | Việc cần làm |
|---|---|---|
| `[RADAR] OK firmware ...` | LD2410 OK | Không cần |
| `[RADAR] not connected` | Sai TX/RX, thiếu GND chung, thiếu nguồn | Đảo TX↔RX, đo 5V |
| `[RADAR] CONFIRMED present` | Có người — camera + TFT bật | Đứng trước radar |
| `[RADAR] CONFIRMED absent` | Hết người ~15s — sleep | — |
| `[CAM] ERROR: PSRAM not found` | Sai Tools PSRAM | Chọn **OPI PSRAM**, Flash **16MB**, nạp lại |
| `[CAM] sensor PID=0x5640` | OV5640 nhận đúng | Không cần |
| `[CAM] esp_camera_init failed` | Pin camera / nguồn / PSRAM | Kiểm tra FPC camera, 3V3 |
| Màu trên TFT đảo / sai | Byte order RGB565 | Đổi `TFT_RGB565_BIG_ENDIAN` trong `config.h` (1 ↔ 0), nạp lại |
| Presence bị đảo (có người lại sleep) | Ngưỡng logic | Đổi `LD2410_PRESENT_LEVEL` (1 ↔ 0) trong `config.h` |

Màn hình lúc boot: splash `DTG CAM` / `waiting...`. Có người: live view. Vắng người: màn đen + tắt backlight.

---

## 9. Tùy chỉnh nhanh (`config.h`)

Chỉ sửa khi đã nạp chạy được bản mặc định.

| Macro | Mặc định | Ghi chú |
|---|---|---|
| `TFT_RGB565_BIG_ENDIAN` | `1` | `0` nếu màu sai |
| `LD2410_PRESENT_LEVEL` | `1` | `0` nếu presence đảo |
| `PRESENCE_CONFIRM_MS` | `80` | Debounce có người (ms) |
| `ABSENCE_CONFIRM_MS` | `15000` | Debounce hết người (ms) |
| `CAPTURE_REFRESH_MS` | `0` | `0` = frame nhanh nhất |
| `CAMERA_VFLIP` / `CAMERA_HMIRROR` | `1` / `0` | Lật ảnh |
| `UPLOAD_ENABLED` | `1` | `0` = tắt WiFi/upload, chỉ live view |
| `UPLOAD_PERIOD_MS` | `300000` | Cooldown giữa 2 lần upload thành công (5 phút) |
| `UPLOAD_SETTLE_MS` | `1500` | Chờ AEC/AWB sau khi wake trước khi up |
| `UPLOAD_JPEG_QUALITY` | `80` | Chất lượng JPEG (1–100) |
| `UPLOAD_SWAP_BYTES` | `0` | `1` nếu màu ảnh trên Supabase bị đảo |

Board camera khác ESP32-S3-EYE: sửa `camera_pins.h`, chỉ bật **một** `#define CAMERA_MODEL_...`.

---

## 10. Supabase snapshot upload

Khi `UPLOAD_ENABLED=1`, firmware đẩy **JPEG 240×240** lên Supabase Storage tối đa **1 ảnh / 5 phút** mỗi thiết bị. Nén JPEG + HTTPS chạy trên FreeRTOS task **core 0** — live view (core 1) không bị block.

### 10.1. Tạo secrets

```bat
copy secrets.h.example secrets.h
```

Điền trong `secrets.h`:

| Macro | Ví dụ |
|---|---|
| `WIFI_SSID` / `WIFI_PASS` | WiFi 2.4 GHz |
| `SUPABASE_HOST` | `abcdefgh.supabase.co` (không có `https://`) |
| `SUPABASE_KEY` | **anon / publishable** key (không dùng `service_role`) |
| `SUPABASE_BUCKET` | `camera-snapshots` |
| `DEVICE_ID` | `cam-01` (khớp RLS policy) |

`secrets.h` đã có trong `.gitignore` — không commit.

### 10.2. SQL setup (chạy 1 lần)

Supabase Dashboard → **SQL Editor** → chạy [`docs/supabase_setup.sql`](docs/supabase_setup.sql).

Policy: bucket private, anon chỉ **INSERT**, path prefix = `DEVICE_ID`, mime `image/jpeg`, max 200 KB.

### 10.3. Object path

```
<DEVICE_ID>/<YYYYMMDD>/<HHMMSS>_<seq>.jpg
```

Nếu NTP chưa sync: `<DEVICE_ID>/nosync/<bootId>_<millis>.jpg`.

### 10.4. Log Serial

| Log | Ý nghĩa |
|---|---|
| `[NET] WiFi begin ssid=...` | Bắt đầu kết nối (non-blocking) |
| `[UPL] ready period=300000ms ...` | Task uploader OK |
| `[UPL] POST ... bytes -> camera-snapshots/...` | Đang gửi |
| `[UPL] OK HTTP 200` | Upload thành công — chờ thêm 5 phút |
| `[UPL] HTTP 401/403` | Key/RLS sai — backoff 60→120→240→300 s |
| `[UPL] state=BUSY/IDLE ok=... fail=...` | Trong STATUS định kỳ |

### 10.5. HIL checklist (sau khi nạp)

| # | Kiểm tra | Kỳ vọng | Kết quả |
|---|---|---|---|
| 1 | Có người sau boot | ~1.5 s sau AWAKE có object trong bucket, màu đúng | NOT_RUN |
| 2 | Còn người | Lần up tiếp theo cách ~5 phút | NOT_RUN |
| 3 | FPS trong STATUS | Trong lúc upload, FPS chênh ≤ 1 so với trước | NOT_RUN |
| 4 | Tắt router | Không treo; reconnect xong up lại | NOT_RUN |
| 5 | Key sai | Log 401, backoff 60/120/240/300 s | NOT_RUN |
| 6 | Người rời giữa upload | Không crash; heap ổn sau nhiều chu kỳ | NOT_RUN |

Host unit test (scheduler): `g++ -std=c++17 tests/test_upload_schedule.cpp -o tests/t.exe && tests\t.exe` — đã PASS trên máy phát triển.

---

## 11. Quy trình tóm tắt (in ra bàn nạp)

1. Cài Arduino IDE 2.x + driver COM.
2. Preferences → dán URL `package_esp32_index.json`.
3. Boards Manager → cài **esp32 by Espressif Systems**.
4. Library Manager → cài **GFX Library for Arduino** + **ld2410**.
5. Mở `ESP32S3Cam_LD2410_OV5640.ino` trong đúng thư mục project.
6. Board = **ESP32S3 Dev Module**, PSRAM = **OPI PSRAM**, Flash = **16MB**.
7. Chọn Port → Verify → Upload.
8. Serial 115200: phải thấy firmware LD2410; đứng trước radar để kiểm tra ảnh.

---

## 12. Yêu cầu kỹ thuật (giữ realtime)

- PSRAM **OPI** bắt buộc: framebuffer RGB565 240×240 nằm PSRAM (`CAMERA_FB_IN_PSRAM`), `fb_count = 2`, `CAMERA_GRAB_LATEST`.
- SPI TFT 40 MHz — không hạ trừ khi màn hình nhiễu.
- `CAPTURE_REFRESH_MS = 0` để pipeline camera → SPI không bị delay nhân tạo.
- Không thêm `delay()` / HTTP trong nhánh `STATE_AWAKE` — upload chỉ `memcpy` + notify.
- Snapshot upload buffer riêng trong PSRAM: camera deinit khi người rời đi không ảnh hưởng upload đang chạy.
- Không thay GFX bằng thư viện chậm hơn (TFT_eSPI software SPI, fill từng pixel) nếu muốn giữ live view mượt.
