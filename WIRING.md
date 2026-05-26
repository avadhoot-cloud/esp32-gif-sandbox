# Wiring - 7-pin ST7789 240x240

Your module header is:

`GND VCC SCL SDA RES DC BLK`

There is no CS pin, so firmware uses `TFT_CS = -1`.

| Display pin | ESP32 pin |
|-------------|-----------|
| GND | GND |
| VCC | 3.3V |
| SCL | GPIO18 |
| SDA | GPIO23 |
| RES | GPIO2 |
| DC | GPIO4 |
| BLK | 3.3V |

## Checks

1. Use 3.3 V for VCC, not 5 V VIN.
2. ESP32 GND and display GND must be connected.
3. RES must be GPIO2 and DC must be GPIO4.
4. BLK must be tied to 3.3 V for an always-on backlight.

If you test the old alternate wiring, flash `pio run -e esp32_tft_photo -t upload`.
