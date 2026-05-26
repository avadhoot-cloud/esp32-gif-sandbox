# ESP32 + ST7789 GIF / Serial Sandbox

Isolated PlatformIO test project for showing GIFs on an ESP32-driven ST7789 TFT.

## Hardware reference

| Item | Value |
|------|-------|
| MCU | ESP32 devkit, 4 MB flash, no PSRAM |
| USB bridge | CP210x, usually COM9 |
| Display driver | ST7789 |
| Resolution | 240 x 240 |
| Interface | SPI |
| SCL / SCLK | GPIO 18 |
| SDA / MOSI | GPIO 23 |
| CS | none, `TFT_CS = -1` |
| RES / RST | GPIO 2 |
| DC | GPIO 4 |
| BLK | 3.3 V always-on |
| Working baseline | `Serial 115200`, `tft.init()`, `setRotation(1)` |

## Quick start

1. Close PlatformIO Serial Monitor so COM9 is free.
2. If upload cannot find your board, edit `platformio_local.ini` and set `upload_port = COM9`.
3. Prepare a panel-sized GIF:
   ```powershell
   python tools/prepare_gif.py
   ```
4. Flash the sandbox firmware from VS Code PlatformIO or terminal:
   ```powershell
   pio run -t upload
   ```
   If `pio` is not on PATH, use:
   ```powershell
   & "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -t upload
   ```
5. Upload and play the GIF, with transfer-rate results:
   ```powershell
   pip install pyserial pillow
   python tools/run_upload_matrix.py --port COM9
   ```

The upload script uses `assets/sample_240.gif`, reports bytes/sec and kilobits/sec, then sends `PLAY_GIF` after each successful upload.

## Manual serial commands

Send these at 115200 baud:

| Command | Purpose |
|---------|---------|
| `{"cmd":"PING"}` | Check firmware is alive |
| `{"cmd":"TFT_BASELINE"}` | Draw the known-good ESP32/ST7789 text and shapes |
| `{"cmd":"TFT_ROT","value":1}` | Set rotation |
| `{"cmd":"FORMAT_FS"}` | Erase LittleFS GIF storage |
| `{"cmd":"LIST_GIFS"}` | List uploaded GIF files |
| `{"cmd":"PLAY_GIF","file":"sample_240.gif"}` | Play an uploaded GIF |

## SPI frequency builds

```powershell
pio run -e esp32_spi20 -t upload
pio run -e esp32_spi27 -t upload
pio run -e esp32_spi40 -t upload
```

Use 27 MHz first. If the image flickers or corrupts, try 20 MHz.
