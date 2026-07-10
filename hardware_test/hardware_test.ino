#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <FastLED.h>

// Keep this file independent from the main project so it compiles fast.
#define PIN_I2C_SDA 21
#define PIN_I2C_SCL 22

#define PIN_JQ_RX_FROM_MODULE 13
#define PIN_JQ_TX_TO_MODULE   16

#define PIN_MQ2 34
#define PIN_IR  35

#define PIN_WS2812 33
#define WS2812_LED_COUNT 16

HardwareSerial jq(2);
CRGB leds[WS2812_LED_COUNT];

void jqSendCommand(uint8_t command, const uint8_t *data, uint8_t dataLength) {
  uint8_t checksum = 0xAA + command + dataLength;
  jq.write((uint8_t)0xAA);
  jq.write(command);
  jq.write(dataLength);
  for (uint8_t i = 0; i < dataLength; i++) {
    jq.write(data[i]);
    checksum += data[i];
  }
  jq.write(checksum);
}

void jqPlayTrack(uint16_t track) {
  uint8_t data[] = { (uint8_t)(track >> 8), (uint8_t)(track & 0xFF) };
  jqSendCommand(0x07, data, 2);
}

void jqSetVolume(uint8_t volume) {
  uint8_t data[] = { (uint8_t)constrain(volume, 0, 30) };
  jqSendCommand(0x13, data, 1);
}

void scanI2C() {
  Serial.println();
  Serial.println("I2C scan start");
  uint8_t found = 0;
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.print("Found I2C: 0x");
      if (addr < 16) Serial.print("0");
      Serial.println(addr, HEX);
      found++;
    }
  }
  if (found == 0) {
    Serial.println("No I2C device found");
  }
  Serial.println("Expected: SHT30=0x44, DS3231=0x68, BH1750=0x23");
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println();
  Serial.println("=== CCAirDetector hardware test ===");
  Serial.println("ESP32 boot ok");

  pinMode(PIN_MQ2, INPUT);
  pinMode(PIN_IR, INPUT);

  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  scanI2C();

  FastLED.addLeds<WS2812B, PIN_WS2812, GRB>(leds, WS2812_LED_COUNT);
  FastLED.setBrightness(64);
  FastLED.clear(true);

  jq.begin(9600, SERIAL_8N1, PIN_JQ_RX_FROM_MODULE, PIN_JQ_TX_TO_MODULE);
  jqSetVolume(8);
  delay(100);
  jqPlayTrack(1);
  Serial.println("Sent JQ8900 play track 1 command");

  WiFi.mode(WIFI_STA);
  Serial.println("WiFi scan start");
  int n = WiFi.scanNetworks();
  Serial.print("WiFi networks: ");
  Serial.println(n);
}

void loop() {
  static uint32_t lastPrint = 0;
  static uint8_t colorIndex = 0;

  if (millis() - lastPrint >= 1000) {
    lastPrint = millis();

    int mq2 = digitalRead(PIN_MQ2);
    int ir = digitalRead(PIN_IR);
    Serial.print("MQ2 DO=");
    Serial.print(mq2);
    Serial.print("  IR=");
    Serial.print(ir);
    Serial.println("  LOW means triggered for your modules");

    if (colorIndex == 0) {
      fill_solid(leds, WS2812_LED_COUNT, CRGB::Red);
    } else if (colorIndex == 1) {
      fill_solid(leds, WS2812_LED_COUNT, CRGB::Green);
    } else {
      fill_solid(leds, WS2812_LED_COUNT, CRGB::Blue);
    }
    FastLED.show();
    colorIndex = (colorIndex + 1) % 3;
  }
}
