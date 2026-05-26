# Wiring — 7-pin ST7789 240×240 (your breadboard)

Your module header (left → right): **GND · VCC · SCL · SDA · RES · DC · BLK**  
There is **no CS pin** — firmware uses `TFT_CS = -1`.

## ESP32 GPIO map (from your photos)

| Display pin | Wire color (typical) | ESP32 pin |
|-------------|----------------------|-----------|
| GND | Black | **GND** |
| VCC | Red | **3V3** (use 3.3 V, not 5 V VIN) |
| SCL | Orange | **D14** (GPIO 14) |
| SDA | Green | **D25** (GPIO 25) |
| RES | Yellow | **D12** (GPIO 12) |
| DC | Black | **D27** (GPIO 27) |
| BLK | Brown | **D13** (GPIO 13) |

PlatformIO `platformio.ini` is set to these pins.

## Common mistakes (check these)

1. **Every display pin must have a wire** — on the breadboard, **VCC must not sit on an empty row** (one photo showed row 25 empty = no power to the chip).
2. **Shared GND** — ESP32 GND and display GND must be connected.
3. **DC wire** — the second black wire goes to **D27**, not GND.
4. **BLK** — if you skip GPIO 13, tie BLK to **3V3** for always-on backlight.

## StatusBar / old docs (different harness)

If you later rewire to the StatusBar harness: MOSI **23**, SCK **18**, CS **5**, DC **2**, RST **4**, BL **15**.
