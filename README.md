# ESP32 + ST7789 GIF / Serial Sandbox

Isolated test project (not part of StatusBar main app). Use this to find reliable **USB serial baud**, **chunk size**, **TFT rotation**, and **SPI clock** before merging changes into production firmware.

## Hardware reference (this project)

| Item | Value |
|------|--------|
| MCU | ESP32 (ESP32-D0WDQ6 class devkit, 4 MB flash, no PSRAM) |
| USB bridge | CP210x (COM9 on your PC) |
| Display driver | **ST7789** |
| Resolution | **240 × 240** |
| Interface | SPI (VSPI) |
| MOSI | GPIO 23 |
| SCLK | GPIO 18 |
| CS | GPIO 5 |
| DC | GPIO 2 |
| RST | GPIO 4 |
| Backlight | GPIO 15 (PWM, active high) |
| Default SPI (main StatusBar) | 27 MHz |
| Your working Arduino sketch | `Serial 115200`, `tft.init()`, **`setRotation(1)`** |

## Quick start

1. Close PlatformIO Serial Monitor and browser Web Serial so the CP210x port is free.
2. If upload cannot find your board, copy `platformio_local.ini.example` → `platformio_local.ini` and set `upload_port` (e.g. `COM9`).
3. Flash sandbox firmware (default env only — does **not** build all SPI/baud variants):
   ```powershell
   cd c:\Users\LENOVO\OneDrive\Desktop\esp32-gif-sandbox
   pio run -t upload
   ```
   Or explicitly: `pio run -e esp32 -t upload`
3. Run automated serial matrix (needs Python + pyserial):
   ```powershell
   pip install pyserial pillow
   python tools/run_upload_matrix.py --port COM9 --gif assets/sample.gif
   ```
4. Manual TFT baseline (115200 serial monitor):
   ```json
   {"cmd":"TFT_BASELINE"}
   ```
5. Wipe GIF storage:
   ```json
   {"cmd":"FORMAT_FS"}
   ```

## Sandbox firmware commands

| Command | Purpose |
|---------|---------|
| `PING` | Alive check |
| `TFT_BASELINE` | Your known-good text + shapes (`init`, rotation 1) |
| `TFT_ROT` + `"value":0-3` | Try rotations |
| `FORMAT_FS` | Erase all LittleFS GIFs |
| `LIST_GIFS` | List `.gif` files |
| `START_UPLOAD` / `CHUNK` / `END_UPLOAD` | Binary upload (production-like) |
| `START_UPLOAD_B64` / `CHUNK_B64` / `END_UPLOAD` | Same flow, no raw binary on wire |
| `PLAY_GIF` + `"file":"sample.gif"` | Play from LittleFS |

## SPI frequency builds

```powershell
pio run -e esp32_spi20 -t upload   # 20 MHz
pio run -e esp32_spi27 -t upload   # 27 MHz (StatusBar default)
pio run -e esp32_spi40 -t upload   # 40 MHz stress test
```

After each flash, run `PLAY_GIF` on an uploaded file and note visual quality.

## Expected outcome

`tools/results/upload_matrix.json` lists which **baud + chunk size** combinations complete without corrupted JSON (e.g. `send chun#` errors). Prefer the **slowest baud that passes 100%** for the main app Web Serial setting.
