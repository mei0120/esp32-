#line 1 "D:\\scheduleTIME\\CCAirDetector\\tftUtil.cpp"
#include <TFT_eSPI.h>
#include "common.h"
#include "PreferencesUtil.h"
#include "tftUtil.h"
#include "Task.h"
#include "net.h"

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite clk = TFT_eSprite(&tft);
int backColor = BACK_BLACK;
int bright = BRIGHT;
uint16_t backFillColor;
uint16_t penColor;
// setting椤甸潰鐢?
bool getDataFailed = false;
// config椤甸潰鐢?
int configChoosedIndex = 0; // 璁板綍褰撳墠閫変腑鐨勬槸绗嚑涓€夐」
bool modalLeftChoosed = false; // 妯℃€佹宸﹂€夐」琚€変腑

static void writeLcdCmd(uint8_t cmd){
  tft.writecommand(cmd);
}

static void writeLcdData(uint8_t data){
  tft.writedata(data);
}

static void initOfficialSt7789v2(){
  pinMode(PIN_TFT_RST, OUTPUT);
  digitalWrite(PIN_TFT_RST, HIGH);
  delay(1);
  digitalWrite(PIN_TFT_RST, LOW);
  delay(10);
  digitalWrite(PIN_TFT_RST, HIGH);
  delay(120);

  writeLcdCmd(0x11);
  delay(120);

  writeLcdCmd(0x36);
  writeLcdData(0x00);

  writeLcdCmd(0x3A);
  writeLcdData(0x55);

  writeLcdCmd(0xB2);
  writeLcdData(0x0C);
  writeLcdData(0x0C);
  writeLcdData(0x00);
  writeLcdData(0x33);
  writeLcdData(0x33);

  writeLcdCmd(0xB7);
  writeLcdData(0x46);

  writeLcdCmd(0xBB);
  writeLcdData(0x1B);

  writeLcdCmd(0xC0);
  writeLcdData(0x2C);

  writeLcdCmd(0xC2);
  writeLcdData(0x01);

  writeLcdCmd(0xC3);
  writeLcdData(0x0F);

  writeLcdCmd(0xC4);
  writeLcdData(0x20);

  writeLcdCmd(0xC6);
  writeLcdData(0x0F);

  writeLcdCmd(0xD0);
  writeLcdData(0xA4);
  writeLcdData(0xA1);

  writeLcdCmd(0xD6);
  writeLcdData(0xA1);

  writeLcdCmd(0xE0);
  const uint8_t gammaPos[] = {0xF0, 0x00, 0x06, 0x04, 0x05, 0x05, 0x31, 0x44, 0x48, 0x36, 0x12, 0x12, 0x2B, 0x34};
  for (uint8_t i = 0; i < sizeof(gammaPos); i++) writeLcdData(gammaPos[i]);

  writeLcdCmd(0xE1);
  const uint8_t gammaNeg[] = {0xF0, 0x0B, 0x0F, 0x0F, 0x0D, 0x26, 0x31, 0x43, 0x47, 0x38, 0x14, 0x14, 0x2C, 0x32};
  for (uint8_t i = 0; i < sizeof(gammaNeg); i++) writeLcdData(gammaNeg[i]);

  writeLcdCmd(0x21);
  writeLcdCmd(0x29);
  delay(20);
}

int displayMinute = -1; // 姝ｅ湪鏄剧ず鐨勫垎閽熸暟
int displayHour = -1; // 姝ｅ湪鏄剧ず鐨勫皬鏃舵暟
int displaySecond = -1;
// calendar椤甸潰鐢?
int year, month, wday, mday, lines, totalDays, lineHeight;
int monthOffset = 0;
int offsetDerection = CALENDAR_RIGHT_OFFSET;
int firstWday,lastWday; // 璁板綍褰撳墠鏄剧ず鐨勬棩鍘嗛〉鐨勫ご涓€澶╁拰鏈竴澶╂槸鍛ㄥ嚑
const int CONFIG_ROW_TOP = 50;
const int CONFIG_ROW_HEIGHT = 21;

static void asciiGlyph(char c, uint8_t glyph[5]){
  c = toupper((unsigned char)c);
  const uint8_t blank[5] = {0,0,0,0,0};
  const uint8_t unknown[5] = {0x7F,0x41,0x5D,0x41,0x7F};
  const uint8_t *src = blank;
  static const uint8_t digits[][5] = {
    {0x3E,0x51,0x49,0x45,0x3E},{0x00,0x42,0x7F,0x40,0x00},
    {0x42,0x61,0x51,0x49,0x46},{0x21,0x41,0x45,0x4B,0x31},
    {0x18,0x14,0x12,0x7F,0x10},{0x27,0x45,0x45,0x45,0x39},
    {0x3C,0x4A,0x49,0x49,0x30},{0x01,0x71,0x09,0x05,0x03},
    {0x36,0x49,0x49,0x49,0x36},{0x06,0x49,0x49,0x29,0x1E}
  };
  static const uint8_t letters[][5] = {
    {0x7E,0x11,0x11,0x11,0x7E},{0x7F,0x49,0x49,0x49,0x36},
    {0x3E,0x41,0x41,0x41,0x22},{0x7F,0x41,0x41,0x22,0x1C},
    {0x7F,0x49,0x49,0x49,0x41},{0x7F,0x09,0x09,0x09,0x01},
    {0x3E,0x41,0x49,0x49,0x7A},{0x7F,0x08,0x08,0x08,0x7F},
    {0x00,0x41,0x7F,0x41,0x00},{0x20,0x40,0x41,0x3F,0x01},
    {0x7F,0x08,0x14,0x22,0x41},{0x7F,0x40,0x40,0x40,0x40},
    {0x7F,0x02,0x0C,0x02,0x7F},{0x7F,0x04,0x08,0x10,0x7F},
    {0x3E,0x41,0x41,0x41,0x3E},{0x7F,0x09,0x09,0x09,0x06},
    {0x3E,0x41,0x51,0x21,0x5E},{0x7F,0x09,0x19,0x29,0x46},
    {0x46,0x49,0x49,0x49,0x31},{0x01,0x01,0x7F,0x01,0x01},
    {0x3F,0x40,0x40,0x40,0x3F},{0x1F,0x20,0x40,0x20,0x1F},
    {0x7F,0x20,0x18,0x20,0x7F},{0x63,0x14,0x08,0x14,0x63},
    {0x07,0x08,0x70,0x08,0x07},{0x61,0x51,0x49,0x45,0x43}
  };
  if(c >= '0' && c <= '9'){
    src = digits[c - '0'];
  }else if(c >= 'A' && c <= 'Z'){
    src = letters[c - 'A'];
  }else{
    switch(c){
      case ' ': src = blank; break;
      case ':': { static const uint8_t g[5] = {0x00,0x36,0x36,0x00,0x00}; src = g; break; }
      case '/': { static const uint8_t g[5] = {0x20,0x10,0x08,0x04,0x02}; src = g; break; }
      case '-': { static const uint8_t g[5] = {0x08,0x08,0x08,0x08,0x08}; src = g; break; }
      case '.': { static const uint8_t g[5] = {0x00,0x60,0x60,0x00,0x00}; src = g; break; }
      case '%': { static const uint8_t g[5] = {0x63,0x13,0x08,0x64,0x63}; src = g; break; }
      case '+': { static const uint8_t g[5] = {0x08,0x08,0x3E,0x08,0x08}; src = g; break; }
      case '!': { static const uint8_t g[5] = {0x00,0x00,0x5F,0x00,0x00}; src = g; break; }
      case '?': src = unknown; break;
      default: src = unknown; break;
    }
  }
  memcpy(glyph, src, 5);
}

static void drawAsciiTextOnBg(const String &text, int x, int y, uint8_t font, uint16_t color, uint16_t bg, uint8_t datum = TL_DATUM);

static void drawAsciiText(const String &text, int x, int y, uint8_t font, uint16_t color, uint8_t datum = TL_DATUM){
  drawAsciiTextOnBg(text, x, y, font, color, backFillColor, datum);
}

static void drawAsciiTextOnBg(const String &text, int x, int y, uint8_t font, uint16_t color, uint16_t bg, uint8_t datum){
  uint8_t scale = font >= 4 ? 3 : 2;
  int width = text.length() == 0 ? 0 : (int)text.length() * 6 * scale - scale;
  if(width > 310 && scale > 1){
    scale = 1;
    width = text.length() == 0 ? 0 : (int)text.length() * 6 * scale - scale;
  }
  int height = 7 * scale;
  if(datum == TC_DATUM || datum == MC_DATUM || datum == BC_DATUM){
    x -= width / 2;
  }else if(datum == TR_DATUM || datum == MR_DATUM || datum == BR_DATUM){
    x -= width;
  }
  if(datum == ML_DATUM || datum == MC_DATUM || datum == MR_DATUM){
    y -= height / 2;
  }else if(datum == BL_DATUM || datum == BC_DATUM || datum == BR_DATUM){
    y -= height;
  }

  tft.fillRect(x, y, width + scale, height, bg);
  for(uint16_t i = 0; i < text.length(); i++){
    uint8_t glyph[5];
    asciiGlyph(text[i], glyph);
    int charX = x + i * 6 * scale;
    for(uint8_t col = 0; col < 5; col++){
      for(uint8_t row = 0; row < 7; row++){
        if(glyph[col] & (1 << row)){
          tft.fillRect(charX + col * scale, y + row * scale, scale, scale, color);
        }
      }
    }
  }
}

static String screenSafeText(const String &text, const String &fallback){
  String out = "";
  bool dropped = false;
  for(uint16_t i = 0; i < text.length(); i++){
    unsigned char c = (unsigned char)text[i];
    if(c >= 32 && c <= 126){
      out += (char)c;
    }else{
      dropped = true;
    }
  }
  out.trim();
  if(out.length() == 0 || (dropped && out.length() < 8)){
    return fallback;
  }
  if(out == "Today:" || out == "Tomorrow:" || out == "Saved:"){
    return fallback;
  }
  return out;
}

static String safeEventLine(const String &text){
  if(text.startsWith("Today:")){
    return screenSafeText(text, "Today: Phone event");
  }
  if(text.startsWith("Tomorrow:")){
    return screenSafeText(text, "Tomorrow: Phone event");
  }
  if(text.startsWith("Saved:")){
    return screenSafeText(text, "Saved on phone");
  }
  return screenSafeText(text, "Phone event");
}

String format2(int value){
  return value < 10 ? "0" + String(value) : String(value);
}

// 鍒濆鍖杢ft
void initBacklight(){
  pinMode(PIN_TFT_BL, OUTPUT);
  digitalWrite(PIN_TFT_BL, HIGH);
}

void setBacklight(int value){
  digitalWrite(PIN_TFT_BL, HIGH);
}

void tftInit(){
  initBacklight();
  tft.init();
  initOfficialSt7789v2();
  tft.initDMA();
  tft.setRotation(1);
  tft.setSwapBytes(true);
  clk.setSwapBytes(true);
  setBacklight(bright); //璋冭妭灞忓箷浜害
  if(backColor == BACK_BLACK){
    backFillColor = 0x0000;
    penColor = 0xFFFF;
  }else{
    backFillColor = 0xFFFF;
    penColor = 0x0000;
  }
  tft.fillScreen(backFillColor);
}
// 鎸夎儗鏅鑹插埛鏂版暣涓睆骞?
void refreshTFT(){
  tft.fillScreen(backFillColor);
}
// 缁樺埗寮€鍦哄姩鐢?
void drawStartLoadingAnim(){
  tft.fillScreen(backFillColor);
  delay(1000);
  // 缁樺埗椤圭洰鍚嶅瓧
  clk.createSprite(240, 50);
  clk.loadFont(projectName_26);
  clk.fillSprite(backFillColor);
  clk.setTextDatum(CC_DATUM);
  clk.setTextColor(penColor);
  clk.drawString("CC Air Detector",120,25);
  for(int i = 0; i < 60; i++){
    clk.pushSprite(40,i);
    delay(3);
  }
  clk.deleteSprite();
  clk.unloadFont();
  // 缁樺埗鎴戠殑鍚嶅瓧
  clk.createSprite(240, 50);
  clk.loadFont(name_24);
  clk.fillSprite(backFillColor);
  clk.setTextDatum(CC_DATUM);
  clk.setTextColor(penColor);
  clk.drawString("Smart Calendar",120,25);
  for(int i = 0; i <= 160; i++){
    clk.pushSprite(i - 120, 100);
    delay(1);
  } 
  clk.deleteSprite();
  clk.unloadFont();
  delay(800);
  // 娓愰殣
  fadeOff();
}
// 缁樺埗閰嶇綉銆佺绾夸娇鐢ㄩ€夋嫨椤甸潰
void drawSettingOrOffline(bool refresh, String text){
  String leftOption = "";
  if(getDataFailed){ // 鏄悓姝ユ暟鎹け璐ヨ繘鍏ョ殑姝ら〉闈?
    leftOption = "Retry";
  }else{
    leftOption = "Config";
  }
  clk.loadFont(settingPage_22);
  if(refresh){
    refreshTFT();
    // 缁樺埗鏂囧瓧
    clk.createSprite(320, 30);
    clk.fillSprite(backFillColor);
    clk.setTextDatum(CC_DATUM);
    clk.setTextColor(penColor);
    clk.drawString(text,160,15);
    clk.pushSprite(0,60);
    clk.deleteSprite();
  }
  // 缁樺埗宸︽寜閽?
  clk.createSprite(120, 50);
  clk.fillSprite(backFillColor);
  if(settingChoosed){
    clk.fillRoundRect(0,0,120,50,7,tft.color565(196,203,207));
  }
  clk.fillRoundRect(5,5,110,40,7,tft.color565(23,114,180));
  clk.setTextDatum(CC_DATUM);
  clk.setTextColor(TFT_WHITE);
  clk.drawString(leftOption,60,25);
  clk.pushSprite(30,140);
  clk.deleteSprite();
  // 缁樺埗鍙虫寜閽?
  clk.createSprite(120, 50);
  clk.fillSprite(backFillColor);
  if(!settingChoosed){
    clk.fillRoundRect(0,0,120,50,7,tft.color565(196,203,207));
  }
  clk.fillRoundRect(5,5,110,40,7,tft.color565(237,51,51));
  clk.setTextDatum(CC_DATUM);
  clk.setTextColor(TFT_WHITE);
  clk.drawString("Offline",60,25);
  clk.pushSprite(170,140);
  clk.deleteSprite();
  clk.unloadFont();
}
// 缁樺埗PAGE1
void drawPage1(){
  refreshTFT();
  drawTop();
  drawAsciiText("Reminder", 160, 26, 4, penColor, TC_DATUM);
  drawReminderStatus();
  drawAsciiText(alarmRinging ? "BN2 Stop" : "BN2 Set", 16, 222, 2, penColor);
  drawAsciiText("BN1 Next", 234, 222, 2, penColor);
  displayMinute = currentMinute();
}

void drawReminderStatus(){
  tft.fillRect(0, 52, 320, 166, backFillColor);
  String nowText = clockReady() ? displayTimeText(false) : "--:--";
  drawAsciiText("Now", 24, 58, 2, penColor);
  drawAsciiText(nowText, 104, 52, 4, penColor);

  uint16_t alarmColor = alarmRinging ? TFT_RED : penColor;
  tft.drawRoundRect(14, 88, 292, 42, 5, alarmColor);
  drawAsciiText(String("Alarm ") + (alarmEnabled ? "ON" : "OFF"), 24, 96, 2, alarmColor);
  drawAsciiText(format2(alarmHour) + ":" + format2(alarmMinute), 132, 92, 4, alarmColor);
  drawAsciiText(String("M") + String(alarmTrack), 252, 100, 2, alarmColor);

  syncEventSummary();
  int nextIndex = nextEventIndex();
  String eventDate = eventDateText(nextIndex);
  String reminder = eventReminderText();
  if(reminder.length() == 0){
    reminder = nextIndex >= 0 ? "Next: " + eventScreenText(nextIndex) : "No event";
  }
  tft.drawRoundRect(14, 136, 292, 46, 5, penColor);
  drawAsciiText("Event " + eventDate, 24, 144, 2, penColor);
  drawAsciiText(safeEventLine(reminder).substring(0, 32), 24, 164, 2, penColor);

  String alarmState = alarmRinging ? "Alarm RINGING" : "Alarm idle";
  drawAsciiText(alarmState, 24, 194, 2, alarmRinging ? TFT_RED : penColor);
  drawAsciiText(eventEnabled ? "Event ON" : "Event OFF", 206, 194, 2, penColor);
  displayMinute = currentMinute();
}

void drawAlarmRingingPage(){
  refreshTFT();
  drawAsciiText("ALARM", 160, 46, 4, TFT_RED, MC_DATUM);
  drawAsciiText(format2(alarmHour) + ":" + format2(alarmMinute) + "  Music M" + String(alarmTrack), 160, 96, 2, penColor, MC_DATUM);
  drawAsciiText(currentFormattedTime(), 160, 126, 2, penColor, MC_DATUM);
  drawAsciiText("Press BN2 to stop", 160, 170, 2, penColor, MC_DATUM);
  tft.drawRoundRect(48, 30, 224, 170, 6, TFT_RED);
}

void drawEventRingingPage(){
  refreshTFT();
  drawAsciiText("EVENT REMINDER", 160, 40, 4, TFT_RED, MC_DATUM);
  int index = activeEventIndex >= 0 ? activeEventIndex : nextEventIndex();
  String date = eventDateText(index);
  drawAsciiText(date, 160, 78, 2, penColor, MC_DATUM);
  drawAsciiText(safeEventLine(eventScreenText(index)).substring(0, 30), 160, 116, 2, penColor, MC_DATUM);
  drawAsciiText("Press BN2 to close", 160, 170, 2, penColor, MC_DATUM);
  tft.drawRoundRect(36, 28, 248, 178, 6, TFT_RED);
}

void drawFireAlarmPage(){
  tft.fillScreen(TFT_BLUE);
  drawAsciiTextOnBg("FIRE ALARM", 160, 42, 4, TFT_WHITE, TFT_BLUE, MC_DATUM);
  drawAsciiTextOnBg("SMOKE / FIRE DETECTED", 160, 88, 2, TFT_WHITE, TFT_BLUE, MC_DATUM);
  drawAsciiTextOnBg("TRACK 3 RED LIGHT EMAIL", 160, 122, 2, TFT_WHITE, TFT_BLUE, MC_DATUM);
  drawAsciiTextOnBg("PRESS BN2 TO STOP", 160, 154, 2, TFT_WHITE, TFT_BLUE, MC_DATUM);
  tft.drawRoundRect(50, 178, 220, 38, 6, TFT_WHITE);
  drawAsciiTextOnBg(simulatedFireAlarm ? "DEMO ALARM ACTIVE" : "MQ2 ALARM ACTIVE", 160, 197, 2, TFT_WHITE, TFT_BLUE, MC_DATUM);
}

void drawSecurityAlarmPage(){
  tft.fillScreen(TFT_BLUE);
  drawAsciiTextOnBg("SECURITY ALARM", 160, 42, 4, TFT_WHITE, TFT_BLUE, MC_DATUM);
  drawAsciiTextOnBg("IR DETECTED", 160, 88, 2, TFT_WHITE, TFT_BLUE, MC_DATUM);
  drawAsciiTextOnBg("TRACK 2 RED LIGHT", 160, 122, 2, TFT_WHITE, TFT_BLUE, MC_DATUM);
  drawAsciiTextOnBg("PRESS BN2 TO STOP", 160, 154, 2, TFT_WHITE, TFT_BLUE, MC_DATUM);
  tft.drawRoundRect(50, 178, 220, 38, 6, TFT_WHITE);
  drawAsciiTextOnBg("ANTI THEFT MODE ACTIVE", 160, 197, 2, TFT_WHITE, TFT_BLUE, MC_DATUM);
}
// 缁樺埗PAGE2
void drawPage2(){
  clk.setTextDatum(CC_DATUM);
  clk.setTextColor(penColor);
  drawTop();

  unsigned long epochTime = currentEpoch();
  struct tm *tm_ptr = nullptr;
  if(epochTime > 0){
    time_t raw = (time_t)epochTime;
    tm_ptr = gmtime(&raw);
  }
  int hourValue = currentHour();
  int hourDisplay = hourValue;
  String ampm = "24H";
  if(!use24HourFormat){
    hourDisplay = hourValue % 12;
    if(hourDisplay == 0){
      hourDisplay = 12;
    }
    ampm = hourValue < 12 ? "AM" : "PM";
  }
  String hourText = hourDisplay < 10 ? "0" + String(hourDisplay) : String(hourDisplay);
  String minuteText = format2(currentMinute());
  String secondText = format2(currentSecond());
  uint16_t datePanel = backColor == BACK_BLACK ? tft.color565(16, 35, 48) : tft.color565(232, 248, 250);
  uint16_t timePanel = backColor == BACK_BLACK ? tft.color565(10, 23, 34) : tft.color565(255, 252, 240);
  uint16_t statusPanel = backColor == BACK_BLACK ? tft.color565(24, 30, 42) : tft.color565(246, 248, 252);
  uint16_t mint = tft.color565(0, 190, 170);
  uint16_t blue = tft.color565(83, 128, 255);
  uint16_t amber = tft.color565(236, 170, 45);
  uint16_t pink = tft.color565(238, 92, 132);

  clk.createSprite(320, 34);
  clk.fillSprite(backFillColor);
  clk.fillRoundRect(8, 2, 304, 30, 7, datePanel);
  clk.loadFont(page3_18);
  clk.setTextColor(penColor);
  if(tm_ptr != nullptr){
    clk.drawString(String(tm_ptr->tm_year + 1900) + "/" + monthDay(tm_ptr->tm_mon, tm_ptr->tm_mday), 80, 15);
    clk.drawString(week(tm_ptr->tm_wday), 175, 15);
  }else{
    clk.drawString("Date --", 95, 15);
  }
  clk.drawString(ampm, 276, 15);
  clk.pushSprite(0, 24);
  clk.unloadFont();
  clk.deleteSprite();

  clk.createSprite(320, 118);
  clk.loadFont(page3Num_90);
  clk.fillSprite(backFillColor);
  clk.fillRoundRect(8, 4, 304, 108, 8, timePanel);
  clk.drawRoundRect(8, 4, 304, 108, 8, mint);
  clk.setTextDatum(CC_DATUM);
  clk.setTextColor(penColor);
  clk.drawString(hourText + ":" + minuteText, 160, 62);
  clk.pushSprite(0, 58);
  clk.unloadFont();
  clk.deleteSprite();

  clk.createSprite(320, 52);
  clk.fillSprite(backFillColor);
  clk.setTextDatum(CC_DATUM);
  clk.loadFont(page2sensor_16);
  clk.fillRoundRect(8, 4, 92, 22, 6, blue);
  clk.fillRoundRect(114, 4, 92, 22, 6, mint);
  clk.fillRoundRect(220, 4, 92, 22, 6, weather.air > 100 ? pink : amber);
  clk.setTextColor(TFT_WHITE);
  clk.drawString("SEC " + secondText, 50, 14);
  clk.drawString("IN " + temperature + " C", 150, 14);
  clk.drawString("AQI " + String(weather.air), 260, 14);
  clk.fillRoundRect(8, 30, 92, 18, 6, statusPanel);
  clk.fillRoundRect(114, 30, 92, 18, 6, infraredDetected ? pink : statusPanel);
  clk.fillRoundRect(220, 30, 92, 18, 6, alarmEnabled ? amber : statusPanel);
  clk.setTextColor(infraredDetected || alarmEnabled ? TFT_WHITE : penColor);
  clk.setTextColor(penColor);
  clk.drawString(String("WiFi ") + (mode == ONLINE_MODE ? "ON" : "OFF"), 52, 39);
  clk.setTextColor(infraredDetected ? TFT_WHITE : penColor);
  clk.drawString(String("IR ") + (infraredDetected ? "ON" : "OFF"), 158, 39);
  clk.setTextColor(alarmEnabled ? TFT_WHITE : penColor);
  clk.drawString(String("ALM ") + (alarmEnabled ? "ON" : "OFF"), 264, 39);
  clk.pushSprite(0, 176);
  clk.unloadFont();
  clk.deleteSprite();

  String eventLine = eventReminderText();
  if(eventLine.length() > 0){
    tft.fillRect(0, 222, 320, 18, TFT_RED);
    drawAsciiText(safeEventLine(eventLine).substring(0, 38), 160, 224, 2, TFT_WHITE, TC_DATUM);
  }else{
    uint16_t idleBar = backColor == BACK_BLACK ? tft.color565(20, 28, 38) : tft.color565(238, 244, 248);
    tft.fillRect(0, 222, 320, 18, idleBar);
    drawAsciiTextOnBg("No event today", 160, 224, 2, penColor, idleBar, TC_DATUM);
  }

  displayMinute = currentMinute();
  displayHour = currentHour();
  displaySecond = currentSecond();
}

void drawPage2Full(){
  refreshTFT();
  drawPage2();
}

void drawClockSecond(){
  clk.createSprite(92, 22);
  clk.fillSprite(tft.color565(83, 128, 255));
  clk.setTextDatum(CC_DATUM);
  clk.setTextColor(TFT_WHITE);
  clk.loadFont(page2sensor_16);
  clk.drawString("SEC " + format2(currentSecond()), 46, 12);
  clk.pushSprite(8, 182);
  clk.unloadFont();
  clk.deleteSprite();
}

void drawWeatherIconAt(int x, int y){
  String s = getWea(weather.icon);
  if(backColor == BACK_BLACK){
    if(s.equals("Snow")){
      tft.pushImage(x,y,50,50,xue_black);
    }else if(s.equals("Thunder")){
      tft.pushImage(x,y,50,50,lei_black);
    }else if(s.equals("Fog")){
      tft.pushImage(x,y,50,50,wu_black);
    }else if(s.equals("Rain")){
      tft.pushImage(x,y,50,50,yu_black);
    }else if(s.equals("Overcast")){
      tft.pushImage(x,y,50,50,yin_black);
    }else if(s.equals("Sunny")){
      tft.pushImage(x,y,50,50,qing_black);
    }else{
      tft.pushImage(x,y,50,50,yun_black);
    }
  }else{
    if(s.equals("Snow")){
      tft.pushImage(x,y,50,50,xue);
    }else if(s.equals("Thunder")){
      tft.pushImage(x,y,50,50,lei);
    }else if(s.equals("Fog")){
      tft.pushImage(x,y,50,50,wu);
    }else if(s.equals("Rain")){
      tft.pushImage(x,y,50,50,yu);
    }else if(s.equals("Overcast")){
      tft.pushImage(x,y,50,50,yin);
    }else if(s.equals("Sunny")){
      tft.pushImage(x,y,50,50,qing);
    }else{
      tft.pushImage(x,y,50,50,yun);
    }
  }
}
// 缁樺埗PAGE3
void drawPage3(bool refresh){
  if(refresh){
    refreshTFT();
  }
  drawTop();
  uint16_t panelColor = backColor == BACK_BLACK ? tft.color565(18, 24, 30) : tft.color565(238, 242, 246);
  uint16_t softColor = backColor == BACK_BLACK ? tft.color565(30, 40, 49) : tft.color565(222, 230, 236);
  uint16_t accentColor = tft.color565(0, 190, 170);

  clk.createSprite(320, 66);
  clk.fillSprite(backFillColor);
  clk.setTextDatum(TL_DATUM);

  clk.fillRoundRect(10, 4, 300, 62, 7, panelColor);
  clk.drawRoundRect(10, 4, 300, 62, 7, accentColor);
  clk.loadFont(page3_18);
  clk.setTextColor(penColor);
  clk.drawString(screenSafeText(city, "City").substring(0, 13), 22, 13);
  clk.drawString(weather.text.substring(0, 12), 22, 38);
  clk.setTextDatum(TR_DATUM);
  clk.drawString(String(weather.temp) + "C", 238, 20);
  clk.setTextDatum(TL_DATUM);
  clk.unloadFont();
  clk.pushSprite(0, 24);
  clk.deleteSprite();
  drawWeatherIconAt(250, 33);

  clk.createSprite(320, 55);
  clk.fillSprite(backFillColor);
  clk.setTextDatum(TL_DATUM);
  clk.loadFont(page2sensor_16);
  const int cardY = 6;
  const int cardW = 70;
  const int cardH = 43;
  const int cardX[] = {10, 86, 162, 238};
  String labels[] = {"AQI", "PM2.5", "OUT H", "IN"};
  String values[] = {
    String(weather.air),
    weather.pm2p5,
    String(weather.humidity) + "%",
    temperature + "C"
  };
  for(uint8_t i = 0; i < 4; i++){
    clk.fillRoundRect(cardX[i], cardY, cardW, cardH, 6, softColor);
    clk.setTextColor(i == 0 && weather.air > 100 ? TFT_RED : penColor);
    clk.drawString(labels[i], cardX[i] + 8, cardY + 7);
    clk.setTextColor(penColor);
    clk.drawString(values[i].substring(0, 7), cardX[i] + 8, cardY + 25);
  }
  clk.unloadFont();
  clk.pushSprite(0, 94);
  clk.deleteSprite();

  clk.createSprite(320, 40);
  clk.fillSprite(backFillColor);
  clk.setTextDatum(TL_DATUM);
  clk.fillRoundRect(10, 3, 300, 34, 6, panelColor);
  clk.loadFont(page2sensor_16);
  clk.setTextColor(penColor);
  clk.drawString("Wind " + screenSafeText(weather.win, "--").substring(0, 15), 22, 13);
  clk.setTextColor(accentColor);
  clk.setTextDatum(TR_DATUM);
  clk.drawString("3D Forecast", 298, 13);
  clk.setTextDatum(TL_DATUM);
  clk.unloadFont();
  clk.pushSprite(0, 149);
  clk.deleteSprite();

  clk.createSprite(320, 43);
  clk.fillSprite(backFillColor);
  clk.setTextDatum(TL_DATUM);
  clk.fillRoundRect(10, 2, 300, 38, 6, panelColor);
  clk.loadFont(page2sensor_16);
  clk.setTextColor(penColor);
  if(weather.forecastReady && weather.forecastCount > 1){
    int row = 0;
    for(int i = 1; i < weather.forecastCount && row < 2; i++){
      String line = weather.forecast[i].date.substring(5) + " " + weather.forecast[i].text + " " +
                    String(weather.forecast[i].tempMin) + "-" + String(weather.forecast[i].tempMax) + "C";
      clk.drawString(line.substring(0, 30), 22, 9 + row * 17);
      row++;
    }
  }else{
    clk.drawString("Forecast waiting", 22, 14);
  }
  clk.unloadFont();
  clk.pushSprite(0, 189);
  clk.deleteSprite();
}

void drawStatusSensorBlock(){
  drawPage3(false);
}

int weekdayOfDate(int y, int m, int d){
  static int offsets[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  if(m < 3){
    y -= 1;
  }
  return (y + y / 4 - y / 100 + y / 400 + offsets[m - 1] + d) % 7;
}

String solarTermForDate(int m, int d){
  const char *terms[] = {
    "Xiao Han", "Da Han", "Li Chun", "Yu Shui", "Jing Zhe", "Chun Fen",
    "Qing Ming", "Gu Yu", "Li Xia", "Xiao Man", "Mang Zhong", "Xia Zhi",
    "Xiao Shu", "Da Shu", "Li Qiu", "Chu Shu", "Bai Lu", "Qiu Fen",
    "Han Lu", "Shuang Jiang", "Li Dong", "Xiao Xue", "Da Xue", "Dong Zhi"
  };
  const uint8_t monthList[] = {1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12};
  const uint8_t dayList[] = {5,20,4,19,6,21,5,20,6,21,6,21,7,23,8,23,8,23,8,23,7,22,7,22};
  for(uint8_t i = 0; i < 24; i++){
    if(m == monthList[i] && d == dayList[i]){
      return String("Sol ") + terms[i];
    }
  }
  for(uint8_t i = 0; i < 24; i++){
    if(m < monthList[i] || (m == monthList[i] && d < dayList[i])){
      return String("N ") + terms[i] + " " + String(monthList[i]) + "/" + String(dayList[i]);
    }
  }
  return "N Xiao Han 1/5";
}

static const uint32_t LUNAR_INFO[] = {
  0x04bd8,0x04ae0,0x0a570,0x054d5,0x0d260,0x0d950,0x16554,0x056a0,0x09ad0,0x055d2,
  0x04ae0,0x0a5b6,0x0a4d0,0x0d250,0x1d255,0x0b540,0x0d6a0,0x0ada2,0x095b0,0x14977,
  0x04970,0x0a4b0,0x0b4b5,0x06a50,0x06d40,0x1ab54,0x02b60,0x09570,0x052f2,0x04970,
  0x06566,0x0d4a0,0x0ea50,0x06e95,0x05ad0,0x02b60,0x186e3,0x092e0,0x1c8d7,0x0c950,
  0x0d4a0,0x1d8a6,0x0b550,0x056a0,0x1a5b4,0x025d0,0x092d0,0x0d2b2,0x0a950,0x0b557,
  0x06ca0,0x0b550,0x15355,0x04da0,0x0a5d0,0x14573,0x052d0,0x0a9a8,0x0e950,0x06aa0,
  0x0aea6,0x0ab50,0x04b60,0x0aae4,0x0a570,0x05260,0x0f263,0x0d950,0x05b57,0x056a0,
  0x096d0,0x04dd5,0x04ad0,0x0a4d0,0x0d4d4,0x0d250,0x0d558,0x0b540,0x0b6a0,0x195a6,
  0x095b0,0x049b0,0x0a974,0x0a4b0,0x0b27a,0x06a50,0x06d40,0x0af46,0x0ab60,0x09570,
  0x04af5,0x04970,0x064b0,0x074a3,0x0ea50,0x06b58,0x055c0,0x0ab60,0x096d5,0x092e0,
  0x0c960,0x0d954,0x0d4a0,0x0da50,0x07552,0x056a0,0x0abb7,0x025d0,0x092d0,0x0cab5,
  0x0a950,0x0b4a0,0x0baa4,0x0ad50,0x055d9,0x04ba0,0x0a5b0,0x15176,0x052b0,0x0a930,
  0x07954,0x06aa0,0x0ad50,0x05b52,0x04b60,0x0a6e6,0x0a4e0,0x0d260,0x0ea65,0x0d530,
  0x05aa0,0x076a3,0x096d0,0x04afb,0x04ad0,0x0a4d0,0x1d0b6,0x0d250,0x0d520,0x0dd45,
  0x0b5a0,0x056d0,0x055b2,0x049b0,0x0a577,0x0a4b0,0x0aa50,0x1b255,0x06d20,0x0ada0,
  0x14b63,0x09370,0x049f8,0x04970,0x064b0,0x168a6,0x0ea50,0x06b20,0x1a6c4,0x0aae0,
  0x0a2e0,0x0d2e3,0x0c960,0x0d557,0x0d4a0,0x0da50,0x05d55,0x056a0,0x0a6d0,0x055d4,
  0x052d0,0x0a9b8,0x0a950,0x0b4a0,0x0b6a6,0x0ad50,0x055a0,0x0aba4,0x0a5b0,0x052b0,
  0x0b273,0x06930,0x07337,0x06aa0,0x0ad50,0x14b55,0x04b60,0x0a570,0x054e4,0x0d160,
  0x0e968,0x0d520,0x0daa0,0x16aa6,0x056d0,0x04ae0,0x0a9d4,0x0a2d0,0x0d150,0x0f252,
  0x0d520
};

static uint32_t lunarInfoForYear(int y){
  if(y < 1900 || y > 2100){
    return 0;
  }
  return LUNAR_INFO[y - 1900];
}

static int leapMonth(int y){
  return lunarInfoForYear(y) & 0x0F;
}

static int leapDays(int y){
  return leapMonth(y) ? ((lunarInfoForYear(y) & 0x10000) ? 30 : 29) : 0;
}

static int lunarMonthDays(int y, int m){
  return (lunarInfoForYear(y) & (0x10000 >> m)) ? 30 : 29;
}

static int lunarYearDays(int y){
  int sum = 348;
  uint32_t info = lunarInfoForYear(y);
  for(uint32_t bit = 0x8000; bit > 0x8; bit >>= 1){
    if(info & bit){
      sum++;
    }
  }
  return sum + leapDays(y);
}

static int daysFromCivil(int y, int m, int d){
  y -= m <= 2;
  const int era = (y >= 0 ? y : y - 399) / 400;
  const unsigned yoe = (unsigned)(y - era * 400);
  const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + (int)doe - 719468;
}

String lunarDateForDate(int y, int m, int d){
  int offset = daysFromCivil(y, m, d) - daysFromCivil(1900, 1, 31);
  if(offset < 0 || y > 2100){
    return "Lunar --";
  }

  int ly = 1900;
  int daysOfYear = 0;
  while(ly <= 2100 && offset >= (daysOfYear = lunarYearDays(ly))){
    offset -= daysOfYear;
    ly++;
  }
  if(ly > 2100){
    return "Lunar --";
  }

  int leap = leapMonth(ly);
  bool isLeap = false;
  int lm = 1;
  int daysOfMonth = 0;
  for(; lm <= 12 && offset >= 0; lm++){
    if(leap > 0 && lm == (leap + 1) && !isLeap){
      lm--;
      isLeap = true;
      daysOfMonth = leapDays(ly);
    }else{
      daysOfMonth = lunarMonthDays(ly, lm);
    }

    if(offset < daysOfMonth){
      break;
    }
    offset -= daysOfMonth;

    if(isLeap && lm == leap){
      isLeap = false;
    }
  }

  String text = "Lunar ";
  if(isLeap){
    text += "L ";
  }
  text += String(lm);
  text += "/";
  text += String(offset + 1);
  return text;
}

// CALENDAR
void drawCalendar(){
  refreshTFT();
  drawTop();
  // 
  unsigned long epochTime = currentEpoch();
  if(epochTime == 0){
    return;
  }
  time_t rawTime = (time_t)epochTime;
  struct tm *tm_ptr = gmtime(&rawTime);
  if(tm_ptr == nullptr){
    return;
  }
  year = tm_ptr->tm_year + 1900;
  month = tm_ptr->tm_mon + 1;
  mday = tm_ptr->tm_mday;
  wday = tm_ptr->tm_wday;

  int showYear = year + monthOffset / 12;
  int showMonth = month + monthOffset % 12;
  if(showMonth > 12){
    showYear++;
    showMonth -= 12;
  }else if(showMonth <= 0){
    showYear--;
    showMonth += 12;
  }

  clk.createSprite(320,35);
  clk.setTextDatum(CC_DATUM);
  clk.fillSprite(backFillColor);
  clk.setTextColor(penColor);
  clk.loadFont(calendar_22);
  clk.drawString(String(showYear) + "/" + String(showMonth), 160, 14);
  clk.drawFastHLine(20,30,280,penColor);
  clk.unloadFont();
  clk.loadFont(page2sensor_16);
  clk.drawString("< BN1L", 55, 14);
  clk.drawString("BN1 >", 265, 14);
  clk.unloadFont();
  clk.pushSprite(0,20);
  clk.deleteSprite();

  clk.createSprite(320,25);
  clk.setTextDatum(CC_DATUM);
  clk.fillSprite(backFillColor);
  clk.setTextColor(penColor);
  clk.loadFont(page2sensor_16);
  clk.drawString("Sun", 40, 12);
  clk.drawString("Mon", 80, 12);
  clk.drawString("Tue", 120, 12);
  clk.drawString("Wed", 160, 12);
  clk.drawString("Thu", 200, 12);
  clk.drawString("Fri", 240, 12);
  clk.drawString("Sat", 280, 12);
  clk.pushSprite(0,55);
  clk.unloadFont();
  clk.deleteSprite();

  totalDays = getTotalDays(showYear, showMonth);
  firstWday = weekdayOfDate(showYear, showMonth, 1);
  lastWday = weekdayOfDate(showYear, showMonth, totalDays);
  lines = (firstWday + totalDays + 6) / 7;
  lineHeight = lines == 6 ? 23 : 28;

  clk.setTextDatum(CC_DATUM); 
  clk.setTextColor(penColor);
  clk.loadFont(calendar_18);
  int index = 1;
  for(int i = 0; i < lines; i++){
    clk.createSprite(320, lineHeight);
    clk.fillSprite(backFillColor);
    for(int j = 0; j < 7; j++){
      if(i == 0 && j < firstWday){
        continue;
      }
      if(index > totalDays){
        break;
      }
      if(index == mday && monthOffset == 0){
        clk.fillCircle(40 + j * 40, lineHeight/2, lineHeight / 2, TFT_BLUE);
        clk.setTextColor(TFT_WHITE);
        clk.drawString(String(index), 40 + j * 40, lineHeight/2 + 4);
        clk.setTextColor(penColor);
      }else{
        clk.drawString(String(index), 40 + j * 40, lineHeight/2 + 4);
      }
      index++;
    }
    clk.pushSprite(0,80 + lineHeight * i);
    clk.deleteSprite();
  }
  clk.unloadFont();
  clk.createSprite(320, 18);
  clk.fillSprite(backFillColor);
  clk.setTextDatum(CC_DATUM);
  clk.setTextColor(penColor);
  clk.loadFont(page2sensor_16);
  int infoDay = monthOffset == 0 ? mday : 1;
  clk.drawString(lunarDateForDate(showYear, showMonth, infoDay), 78, 9);
  clk.drawString(solarTermForDate(showMonth, infoDay), 230, 9);
  clk.pushSprite(0, 222);
  clk.unloadFont();
  clk.deleteSprite();
  displayMinute = currentMinute();
}

void drawConfig(){
  refreshTFT();

  drawTop();

  clk.setTextColor(penColor);
  clk.createSprite(320, 30);
  clk.fillSprite(backFillColor);
  clk.loadFont(configTitle_26);
  clk.setTextDatum(CC_DATUM);
  clk.drawString("Config",160,15);
  clk.pushSprite(0,20);
  clk.deleteSprite();
  clk.unloadFont();
  for(int i = 0; i < OPTION_COUNT; i++){
    drawConfigOption(i);
  }
}

void draw2LineText(String text1, String text2){
  refreshTFT();
  clk.loadFont(settingPage_22);
  clk.createSprite(320, 100);
  clk.fillSprite(backFillColor);
  clk.setTextDatum(CC_DATUM);
  clk.setTextColor(penColor);
  clk.drawString(text1, 160, 15);
  clk.drawString(text2, 160, 60);
  clk.pushSprite(0,60);
  clk.deleteSprite();
  clk.unloadFont();
}

void drawLoading(bool firstTime, String text, int *angle){
  // 绗竴娆¤繘鏉ワ紝鍏堟竻灞忥紝鍐嶇粯鍒舵枃瀛?
  if(firstTime){
    refreshTFT();
    clk.loadFont(settingPage_22);
    clk.createSprite(320, 30);
    clk.fillSprite(backFillColor);
    clk.setTextDatum(CC_DATUM);
    clk.setTextColor(penColor);
    clk.drawString(text, 160, 15);
    clk.pushSprite(0,60);
    clk.deleteSprite();
    clk.unloadFont();
  }
  clk.createSprite(60, 60);
  clk.fillSprite(backFillColor);
  clk.fillCircle(30 + 20 * cos(*angle * 2 * M_PI / 360), 30 + 20 * sin(*angle * 2 * M_PI / 360), 7, penColor);
  clk.fillCircle(30 + 20 * cos(((*angle - 60) >= 360?(360 - (*angle - 60)) : (*angle - 60)) * 2 * M_PI / 360), 30 + 20 * sin(((*angle - 60) >= 360?(360 - (*angle - 60)) : (*angle - 60)) * 2 * M_PI / 360), 6, penColor);
  clk.fillCircle(30 + 20 * cos(((*angle - 120) >= 360?(360 - (*angle - 120)) : (*angle - 120)) * 2 * M_PI / 360), 30 + 20 * sin(((*angle - 120) >= 360?(360 - (*angle - 120)) : (*angle - 120)) * 2 * M_PI / 360), 4, penColor);
  clk.pushSprite(130,110);
  clk.deleteSprite();
  (*angle)+=3;
  if(*angle >= 360){
    *angle = 0;
  }
}
// 灞忓箷娓愰殣
void fadeOff(){
  setBacklight(bright);
}
// 灞忓箷娓愭樉
void fadeOn(){
  setBacklight(bright);
}
// 缁樺埗椤堕儴鐘舵€佹爮
void drawTop(){
  tft.fillRect(0, 0, 320, 20, backFillColor);
  drawAsciiText(mode == ONLINE_MODE ? "WiFi" : "OFF", 8, 2, 2, penColor);
}
// 绘设置页面
void drawConfigOption(int index){
  String s;
  switch(index){
    case OPTION_VOICE:
      s = "Sound";
      break;
    case OPTION_OFFSET:
      s = "Time format";
      break;  
    case OPTION_THEME:
      s = "Theme";
      break;
    case OPTION_SECURITY:
      s = "Security";
      break;
    case OPTION_SMOKE_TEST:
      s = "Smoke Test";
      break;
    case OPTION_ALARM:
      s = "Alarm";
      break;
    case OPTION_WLAN:
      s = "WiFi";
      break;
    case OPTION_RESET:
      s = "Reset";
      break;
    case OPTION_BACK:
      s = "Back";
      break;
    default:
      break;          
  }
  clk.createSprite(320, CONFIG_ROW_HEIGHT);
  clk.setTextDatum(CC_DATUM);
  if(index == configChoosedIndex){
    clk.fillSprite(penColor);
    clk.setTextColor(backFillColor);
  }else{
    clk.fillSprite(backFillColor);
    clk.setTextColor(penColor);
  }
  clk.loadFont(configOption_18);
  clk.drawString(">", 20, 12);
  clk.drawString(s, 92, 12);
  if(index == OPTION_VOICE){
    if(voice){
      clk.drawString("On", 300, 12);
    }else{
      clk.drawString("Off", 300, 12);
    }
  }
  if(index == OPTION_THEME){
    if(backColor == BACK_BLACK){
      clk.drawString("Dark", 285, 12);
    }else{
      clk.drawString("Light", 285, 12);
    }
  }
  if(index == OPTION_OFFSET){
    clk.drawString(use24HourFormat ? "24H" : "AM/PM", 288, 12);
  }
  if(index == OPTION_SECURITY){
    if(antiTheftMode){
      clk.drawString("On", 300, 12);
    }else{
      clk.drawString("Off", 300, 12);
    }
  }
  if(index == OPTION_SMOKE_TEST){
    clk.drawString(simulatedFireAlarm ? "On" : "Off", 300, 12);
  }
  if(index == OPTION_ALARM){
    clk.drawString(alarmEnabled ? format2(alarmHour) + ":" + format2(alarmMinute) : "--:--", 274, 12);
  }
  clk.unloadFont();
  clk.pushSprite(0, CONFIG_ROW_TOP + (index) * CONFIG_ROW_HEIGHT);
  clk.deleteSprite();
}
// 缁樺埗閰嶇疆缃戠粶鍜屾仮澶嶅嚭鍘傜殑妯℃€佹
void drawModal(String text, bool refresh){
  clk.loadFont(configOption_18);
  clk.setTextDatum(CC_DATUM);
  clk.setTextColor(backFillColor);
  if(refresh){
    tft.fillRoundRect(30, 60, 260, 100, 10, penColor);
    // 缁樺埗鏍囬
    clk.createSprite(200, 30);
    clk.fillSprite(penColor);
    clk.drawString(text,130,15);
    clk.pushSprite(30,70);
    clk.deleteSprite();
  }
  // 缁樺埗宸︽寜閽?
  clk.createSprite(64, 34);
  clk.fillSprite(penColor);
  if(modalLeftChoosed){
    clk.fillRoundRect(0,0,64,34,7,tft.color565(196,203,207));
  }
  clk.fillRoundRect(2,2,60,30,7,tft.color565(23,114,180));
  clk.setTextColor(TFT_WHITE);
  clk.drawString("OK",32,18);
  clk.pushSprite(70,115);
  clk.deleteSprite();
  // 缁樺埗鍙虫寜閽?
  clk.createSprite(64, 34);
  clk.fillSprite(penColor);
  if(!modalLeftChoosed){
    clk.fillRoundRect(0,0,64,34,7,tft.color565(196,203,207));
  }
  clk.fillRoundRect(2,2,60,30,7,tft.color565(237,51,51));
  clk.setTextColor(TFT_WHITE);
  clk.drawString("Cancel",32,18);
  clk.pushSprite(186,115);
  clk.deleteSprite();
  clk.unloadFont();
}
void drawAlarmModal(bool refresh){
  if(refresh){
    tft.fillRoundRect(45, 86, 230, 82, 7, penColor);
  }
  clk.createSprite(210, 64);
  clk.fillSprite(penColor);
  clk.setTextDatum(CC_DATUM);
  clk.setTextColor(backFillColor);
  clk.loadFont(configOption_18);
  clk.drawString("Set Alarm", 105, 12);
  clk.drawString(format2(editAlarmHour) + ":" + format2(editAlarmMinute) + "  M" + String(editAlarmTrack), 105, 36);
  String field = "Edit Music";
  if(alarmEditField == 0){
    field = "Edit Hour";
  }else if(alarmEditField == 1){
    field = "Edit Minute";
  }
  clk.drawString(field, 105, 56);
  clk.unloadFont();
  clk.pushSprite(55, 95);
  clk.deleteSprite();
}
// 鍒囨崲涓婚
void exchangeTheme(){
  if(backColor == BACK_BLACK){
    backColor = BACK_WHITE;
    backFillColor = 0xFFFF;
    penColor = 0x0000;
  }else{
    backColor = BACK_BLACK;
    backFillColor = 0x0000;
    penColor = 0xFFFF;
  }
  setTheme();
}
// 缁樺埗褰撳墠椤甸潰
void drawCurrentPage(){
  if(fireAlertActive()){
    drawFireAlarmPage();
    return;
  }
  if(theftAlertActive()){
    drawSecurityAlarmPage();
    return;
  }
  if(eventRinging){
    drawEventRingingPage();
    return;
  }
  switch(currentPage){
    case PAGE1:
      drawPage1();
      break;
    case PAGE2:
      drawPage2Full();
      break;
    case PAGE3:
      drawPage3(true);
      break;
    case CALENDAR:
      drawCalendar();
      break; 
case CONFIG:
      drawConfig();
      break;  
    default:
      break;
  }
}
//澶勭悊鏄熸湡
String week(int tm_wday){
  String wk[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  String s = wk[tm_wday];
  return s;
}
//澶勭悊鏈堟棩
String monthDay(int tm_mon, int tm_mday){
  String s = "";
  s = s + (tm_mon + 1);
  s = s + "/" + tm_mday;
  return s;
}
//鏍规嵁骞翠唤鍜屾湀浠借幏鍙栬繖涓湀鐨勫ぉ鏁?
int getTotalDays(int year, int month){
  if(month == 1 || month == 3|| month == 5|| month == 7|| month == 8|| month == 10|| month == 12){
    return 31;
  }else if(month == 4 || month == 6 || month == 9 || month == 11){
    return 30;
  }else{
    if((year % 400 == 0) || (year % 4 == 0 && year % 100 != 0)){
      return 29;
    }else{
      return 28;
    }
  }
}
// 鏍规嵁icon鑾峰緱澶╂皵鐘跺喌
String getWea(int icon){
  String s = "";
  switch (icon){
    case 100:
    case 150:
      s = "Sunny";
      break;
    case 104:
      s = "Overcast";
      break;
    case 300:
    case 301:
    case 305:
    case 306:
    case 307:
    case 308:
    case 309:
    case 310:
    case 311:
    case 312:
    case 313:
    case 314:
    case 315:
    case 316:
    case 317:
    case 318:
    case 350:
    case 351:
    case 399:
      s = "Rain";
      break;
    case 101:
    case 102:
    case 103:
    case 151:
    case 152:
    case 153:
      s = "Cloudy";
      break;
    case 304:
      s = "Hail";
      break;
    case 500:
    case 501:
    case 502:
    case 509:
    case 510:
    case 511:
    case 512:
    case 513:
    case 514:
    case 515:
      s = "Fog";
      break;
    case 503:
    case 504:
    case 507:
    case 508:
      s = "Dust";
      break;
    case 302:
    case 303:
      s = "Thunder";
      break;
    case 400:
    case 401:
    case 402:
    case 403:
    case 404:
    case 405:
    case 406:
    case 407:
    case 408:
    case 409:
    case 410:
    case 456:
    case 457:
    case 499:
      s = "Snow";
      break;
    default:
      s = "Sunny";
      break;  
  }
  return s;
}

