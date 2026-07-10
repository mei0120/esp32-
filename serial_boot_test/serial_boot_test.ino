#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println("=== ESP32 serial boot test ===");
  Serial.println("If you can read this, upload and UART are OK.");
}

void loop() {
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint >= 1000) {
    lastPrint = millis();
    Serial.print("millis=");
    Serial.println(millis());
  }
}
