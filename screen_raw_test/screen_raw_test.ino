#include <Arduino.h>
#include <SPI.h>

#define TFT_MOSI 23
#define TFT_MISO 19
#define TFT_SCLK 18
#define TFT_CS   5
#define TFT_DC   2
#define TFT_RST  4
#define TFT_BL   17

SPIClass tftSpi(VSPI);

void csLow() {
  digitalWrite(TFT_CS, LOW);
}

void csHigh() {
  digitalWrite(TFT_CS, HIGH);
}

void writeCommand(uint8_t cmd) {
  digitalWrite(TFT_DC, LOW);
  csLow();
  tftSpi.transfer(cmd);
  csHigh();
}

void writeData(uint8_t data) {
  digitalWrite(TFT_DC, HIGH);
  csLow();
  tftSpi.transfer(data);
  csHigh();
}

void writeData16(uint16_t data) {
  digitalWrite(TFT_DC, HIGH);
  csLow();
  tftSpi.transfer(data >> 8);
  tftSpi.transfer(data & 0xFF);
  csHigh();
}

void setWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
  writeCommand(0x2A);
  writeData(x0 >> 8);
  writeData(x0 & 0xFF);
  writeData(x1 >> 8);
  writeData(x1 & 0xFF);

  writeCommand(0x2B);
  writeData(y0 >> 8);
  writeData(y0 & 0xFF);
  writeData(y1 >> 8);
  writeData(y1 & 0xFF);

  writeCommand(0x2C);
}

void fillScreen(uint16_t color) {
  setWindow(0, 0, 239, 319);
  digitalWrite(TFT_DC, HIGH);
  csLow();
  for (uint32_t i = 0; i < 240UL * 320UL; i++) {
    tftSpi.transfer(color >> 8);
    tftSpi.transfer(color & 0xFF);
  }
  csHigh();
}

void initSt7789() {
  digitalWrite(TFT_RST, HIGH);
  delay(1);
  digitalWrite(TFT_RST, LOW);
  delay(10);
  digitalWrite(TFT_RST, HIGH);
  delay(120);

  writeCommand(0x11);
  delay(120);

  writeCommand(0x36);
  writeData(0x00);

  writeCommand(0x3A);
  writeData(0x55);

  writeCommand(0xB2);
  writeData(0x0C);
  writeData(0x0C);
  writeData(0x00);
  writeData(0x33);
  writeData(0x33);

  writeCommand(0xB7);
  writeData(0x46);

  writeCommand(0xBB);
  writeData(0x1B);

  writeCommand(0xC0);
  writeData(0x2C);

  writeCommand(0xC2);
  writeData(0x01);

  writeCommand(0xC3);
  writeData(0x0F);

  writeCommand(0xC4);
  writeData(0x20);

  writeCommand(0xC6);
  writeData(0x0F);

  writeCommand(0xD0);
  writeData(0xA4);
  writeData(0xA1);

  writeCommand(0xD6);
  writeData(0xA1);

  writeCommand(0xE0);
  const uint8_t gammaPos[] = {0xF0, 0x00, 0x06, 0x04, 0x05, 0x05, 0x31, 0x44, 0x48, 0x36, 0x12, 0x12, 0x2B, 0x34};
  for (uint8_t i = 0; i < sizeof(gammaPos); i++) writeData(gammaPos[i]);

  writeCommand(0xE1);
  const uint8_t gammaNeg[] = {0xF0, 0x0B, 0x0F, 0x0F, 0x0D, 0x26, 0x31, 0x43, 0x47, 0x38, 0x14, 0x14, 0x2C, 0x32};
  for (uint8_t i = 0; i < sizeof(gammaNeg); i++) writeData(gammaNeg[i]);

  writeCommand(0x21);
  writeCommand(0x29);
  delay(100);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println("=== Raw ST7789 SPI test ===");

  pinMode(TFT_CS, OUTPUT);
  pinMode(TFT_DC, OUTPUT);
  pinMode(TFT_RST, OUTPUT);
  pinMode(TFT_BL, OUTPUT);

  digitalWrite(TFT_CS, HIGH);
  digitalWrite(TFT_DC, HIGH);
  digitalWrite(TFT_BL, HIGH);

  tftSpi.begin(TFT_SCLK, TFT_MISO, TFT_MOSI, TFT_CS);
  tftSpi.setFrequency(1000000);
  tftSpi.setDataMode(SPI_MODE0);
  tftSpi.setBitOrder(MSBFIRST);

  initSt7789();
}

void loop() {
  Serial.println("raw fill red");
  fillScreen(0xF800);
  delay(1000);
  Serial.println("raw fill green");
  fillScreen(0x07E0);
  delay(1000);
  Serial.println("raw fill blue");
  fillScreen(0x001F);
  delay(1000);
  Serial.println("raw fill white");
  fillScreen(0xFFFF);
  delay(1000);
}
