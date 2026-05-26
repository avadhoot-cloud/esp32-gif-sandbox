# Hardware profile — ESP32 + ST7789 (your bench)

## MCU

| Property | Value |
|----------|--------|
| Chip | ESP32-D0WDQ6 (rev v1.1 observed on your board) |
| Cores | 2 × Xtensa LX6 @ 240 MHz |
| RAM | 320 KB SRAM |
| Flash | 4 MB |
| PSRAM | None on standard DevKit |
| USB–UART | Silicon Labs CP210x → **COM9** |

## Display

| Property | Value |
|----------|--------|
| Controller | **ST7789** |
| Resolution | **240 × 240** pixels |
| Interface | 4-wire SPI (VSPI) |
| Color | RGB565 (16-bit) |

### Wiring (your 7-pin breadboard module — see `WIRING.md`)

| Signal | GPIO |
|--------|------|
| MOSI (SDA) | **25** |
| SCLK (SCL) | **14** |
| CS | *(none — module has no CS)* |
| DC | **27** |
| RST (RES) | **12** |
| BL (BLK) | **13** |

StatusBar production harness uses MOSI 23, SCK 18, CS 5, DC 2, RST 4, BL 15.

## Your working Arduino sketch vs StatusBar

| Setting | Your sketch | StatusBar (before sandbox) |
|---------|-------------|----------------------------|
| `Serial` baud | **115200** | 921600 |
| TFT init | **`tft.init()`** | `tft.begin()` |
| Rotation | **`setRotation(1)`** | `setRotation(0)` |
| GIF palette | — | was LE, now BE in fixes |

Sandbox defaults: **115200**, **`init()`**, **rotation 1**, **BIG_ENDIAN** GIF palette.

## SPI clock test matrix

| PlatformIO env | SPI_FREQUENCY |
|----------------|---------------|
| `esp32_spi20` | 20 MHz |
| `esp32_spi27` | 27 MHz (StatusBar default) |
| `esp32_spi40` | 40 MHz (stress) |

## USB serial corruption (`send chun#…`)

At **921600 baud**, raw binary chunks can **overlap** JSON lines on the USB CDC link → host sees corrupted ACKs and stalls ~15–25%.

Mitigations tested in sandbox:

1. **115200** USB baud (reliable, slower)
2. **512–1024** byte chunks + `Serial.flush()` on device
3. **No** `Serial.printf` during upload
4. **16 KB** RX buffer on ESP32

Run `tools/run_upload_matrix.py` after flashing to record which combinations pass.
