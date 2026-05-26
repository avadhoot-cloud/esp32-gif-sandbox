// Matches TFT_eSPI "Generic ESP32" ST7789 240x240 (Setup24 commented block).
// Used when Arduino IDE has this in libraries/TFT_eSPI/User_Setup.h

#define ST7789_DRIVER

#define TFT_WIDTH  240
#define TFT_HEIGHT 240

#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS   -1
#define TFT_DC   2
#define TFT_RST  4
#define TFT_BL   13
#define TFT_BACKLIGHT_ON HIGH

#define LOAD_GLCD
#define LOAD_FONT2

#define SPI_FREQUENCY       27000000
#define SPI_READ_FREQUENCY  16000000
