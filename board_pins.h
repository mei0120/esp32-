#pragma once

// I2C
#define PIN_I2C_SDA 21
#define PIN_I2C_SCL 22

// TFT SPI
#define PIN_TFT_MOSI 23
#define PIN_TFT_MISO 19
#define PIN_TFT_SCLK 18
#define PIN_TFT_CS   5
#define PIN_TFT_DC   2
#define PIN_TFT_RST  4
#define PIN_TFT_BL   17

// JQ8900
#define PIN_JQ_RX_FROM_MODULE 13
#define PIN_JQ_TX_TO_MODULE   16

// Digital inputs
#define PIN_MQ2 34
#define PIN_IR  35

// Optional battery ADC. Keep disabled because GPIO34 is used by MQ2 on this board.
#define PIN_BAT_ADC -1

// RGB LED
#define PIN_WS2812 33
#define WS2812_LED_COUNT 16
#define WS2812_COLOR_ORDER RGB

// Trigger levels. Change these after real board testing if the modules are active-high.
#define MQ2_TRIGGER_LEVEL LOW
#define IR_TRIGGER_LEVEL  LOW

// Buttons
#define PIN_BTN1 25
#define PIN_BTN2 26
#define PIN_BTN3 27
