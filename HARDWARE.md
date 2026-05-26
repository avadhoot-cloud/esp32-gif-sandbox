# Hardware Profile

## Display

| Property | Value |
|----------|-------|
| Controller | ST7789 |
| Resolution | 240 x 240 pixels |
| Interface | 4-wire SPI without CS |
| Color format | RGB565 |

## Wiring

| TFT pin | ESP32 |
|---------|-------|
| VCC | 3.3V |
| GND | GND |
| SCL | GPIO18 |
| SDA | GPIO23 |
| RES | GPIO2 |
| DC | GPIO4 |
| BLK | 3.3V |

These are the active PlatformIO defaults in `platformio.ini` and `include/User_Setup.h`.

## Firmware Defaults

| Setting | Value |
|---------|-------|
| Serial baud | 115200 |
| TFT init | `tft.init()` |
| Rotation | `setRotation(1)` |
| GIF palette | `BIG_ENDIAN_PIXELS` |
| Default environment | `esp32` |

The old photo-test wiring is still available as `esp32_tft_photo`, but it is not the default.
