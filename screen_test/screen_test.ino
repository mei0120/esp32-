#include <Arduino.h>
#include <TFT_eSPI.h>

#define PIN_TFT_BL 17
#define PIN_TFT_RST 4

TFT_eSPI tft = TFT_eSPI();

void lcdCmd(uint8_t cmd) {
  tft.writecommand(cmd);
}

void lcdData(uint8_t data) {
  tft.writedata(data);
}

void initOfficialSt7789v2() {
  digitalWrite(PIN_TFT_RST, HIGH);
  delay(1);
  digitalWrite(PIN_TFT_RST, LOW);
  delay(10);
  digitalWrite(PIN_TFT_RST, HIGH);
  delay(120);

  lcdCmd(0x11);
  delay(120);

  lcdCmd(0x36);
  lcdData(0x00);

  lcdCmd(0x3A);
  lcdData(0x55);

  lcdCmd(0xB2);
  lcdData(0x0C);
  lcdData(0x0C);
  lcdData(0x00);
  lcdData(0x33);
  lcdData(0x33);

  lcdCmd(0xB7);
  lcdData(0x46);

  lcdCmd(0xBB);
  lcdData(0x1B);

  lcdCmd(0xC0);
  lcdData(0x2C);

  lcdCmd(0xC2);
  lcdData(0x01);

  lcdCmd(0xC3);
  lcdData(0x0F);

  lcdCmd(0xC4);
  lcdData(0x20);

  lcdCmd(0xC6);
  lcdData(0x0F);

  lcdCmd(0xD0);
  lcdData(0xA4);
  lcdData(0xA1);

  lcdCmd(0xD6);
  lcdData(0xA1);

  lcdCmd(0xE0);
  const uint8_t gammaPos[] = {0xF0, 0x00, 0x06, 0x04, 0x05, 0x05, 0x31, 0x44, 0x48, 0x36, 0x12, 0x12, 0x2B, 0x34};
  for (uint8_t value : gammaPos) lcdData(value);

  lcdCmd(0xE1);
  const uint8_t gammaNeg[] = {0xF0, 0x0B, 0x0F, 0x0F, 0x0D, 0x26, 0x31, 0x43, 0x47, 0x38, 0x14, 0x14, 0x2C, 0x32};
  for (uint8_t value : gammaNeg) lcdData(value);

  lcdCmd(0x21);
  lcdCmd(0x29);
  delay(20);
}

void setBacklight(uint8_t value) {
  pinMode(PIN_TFT_BL, OUTPUT);
  digitalWrite(PIN_TFT_BL, value > 0 ? HIGH : LOW);
}

void fillAndLabel(uint16_t color, const char *label, uint16_t textColor) {
  tft.fillScreen(color);
  tft.setTextColor(textColor, color);
  tft.setTextDatum(MC_DATUM);
  tft.drawString(label, tft.width() / 2, tft.height() / 2, 4);
  delay(900);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println("=== TFT screen test ===");

  setBacklight(180);
  pinMode(PIN_TFT_RST, OUTPUT);
  digitalWrite(PIN_TFT_RST, LOW);
  delay(80);
  digitalWrite(PIN_TFT_RST, HIGH);
  delay(200);

  tft.init();
  initOfficialSt7789v2();
  tft.setRotation(1);
  tft.setSwapBytes(true);
  Serial.print("TFT size: ");
  Serial.print(tft.width());
  Serial.print(" x ");
  Serial.println(tft.height());

  fillAndLabel(TFT_RED, "RED", TFT_WHITE);
  fillAndLabel(TFT_GREEN, "GREEN", TFT_BLACK);
  fillAndLabel(TFT_BLUE, "BLUE", TFT_WHITE);
  fillAndLabel(TFT_WHITE, "WHITE", TFT_BLACK);
  fillAndLabel(TFT_BLACK, "BLACK", TFT_WHITE);

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(TL_DATUM);
  tft.drawString("CCAirDetector", 20, 20, 4);
  tft.drawString("TFT OK", 20, 60, 4);
  tft.drawString("BL GPIO17", 20, 100, 2);
}

void loop() {
  static uint32_t last = 0;
  static uint32_t count = 0;
  if (millis() - last >= 1000) {
    last = millis();
    tft.fillRect(20, 135, 220, 30, TFT_BLACK);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawString("seconds: " + String(count++), 20, 135, 4);
    Serial.println("TFT test running");
  }
}
