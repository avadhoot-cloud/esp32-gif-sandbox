# TFT minimal test (your exact Arduino sketch)

Your sketch is in `src/main.cpp`. Sandbox firmware is in `src/main_sandbox.cpp`.

## Flash (default = standard ESP32 ST7789 pins)

```powershell
pio run -e esp32_tft_test -t upload
```

**Expected wiring (must match):**

| Display | ESP32 |
|---------|-------|
| SDA (MOSI) | **D23** |
| SCL (SCK) | **D18** |
| DC | **D2** |
| RES (RST) | **D4** |
| BLK | **D13** (or tie BLK to 3V3) |
| GND | GND |
| VCC | 3V3 |

No CS pin on your module (`TFT_CS = -1`).

## If still blank — try breadboard photo pins

```powershell
pio run -e esp32_tft_photo -t upload
```

| Display | ESP32 |
|---------|-------|
| SDA | **D25** |
| SCL | **D14** |
| DC | **D27** |
| RES | **D12** |
| BLK | **D13** |

## Restore sandbox firmware

```powershell
pio run -e esp32 -t upload
```

## Match Arduino IDE exactly

Copy your `User_Setup.h` from:

`Arduino/libraries/TFT_eSPI/User_Setup.h`

into `include/User_Setup.h`, then run `pio run -e esp32_tft_test -t upload`.
