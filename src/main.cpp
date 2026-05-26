#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

void setup() {

  Serial.begin(115200);

  tft.init();

  tft.setRotation(1);

  tft.fillScreen(TFT_BLACK);

  // Text
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);

  tft.setCursor(40, 40);
  tft.println("ESP32");

  tft.setTextColor(TFT_GREEN, TFT_BLACK);

  tft.setCursor(20, 90);
  tft.println("ST7789 TFT");

  // Shapes
  tft.drawRect(10, 10, 220, 220, TFT_RED);

  tft.fillCircle(120, 180, 30, TFT_BLUE);
}

void loop() {

}
