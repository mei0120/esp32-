#include <OneButton.h>
#include <Wire.h>
#include <Ticker.h>
#include <FastLED.h>
#include <time.h>
#include "net.h"
#include "Task.h"
#include "tftUtil.h"
#include "PreferencesUtil.h"
#include "MailAlert.h"

enum CurrentPage currentPage = SETTING; // 璁板綍褰撳墠椤甸潰
unsigned long lastRefresh;  // 涓婃鍒锋柊浼犳劅鍣ㄦ暟鎹殑鏃堕棿
bool updateWeather = false; // 鏄惁闇€瑕佹洿鏂板ぉ姘?
// 鎸夐挳
OneButton button1(BTN1, true);
OneButton button2(BTN2, true);
// 绯荤粺鍙橀噺
int mode = OFFLINE_MODE; // 杩愯妯″紡
bool buttonEnable = true; // 鎸夐敭浣胯兘
bool settingChoosed = true; // 閫夋嫨鐨勬槸寮€濮嬮厤缃寜閿?
bool voice = true; // 澹伴煶鏄惁寮€鍚?
bool loadingAnim = false; // 鍔犺浇鍔ㄧ敾鏄惁鎵ц
bool modalShowed = false; // 妯℃€佹鏄惁宸叉樉绀?
unsigned long lastUserAction = 0;
float tempOffset = 0.0f;
float tmpTempOffset = 0.0f;
bool use24HourFormat = false;
String tvoc = "0.00";
String ch2o = "0.00";
String co2 = "0";
String temperature = "0.0";
String humidity = "0.0";
float lightLux = 0.0f;
bool fireAlarm = false;
bool simulatedFireAlarm = false;
bool infraredDetected = false;
bool fireAlarmAcknowledged = false;
bool theftAlarmAcknowledged = false;
volatile bool sensorStateChanged = false;
bool antiTheftMode = true;
bool alarmEnabled = false;
int alarmHour = 7;
int alarmMinute = 0;
int alarmTrack = 1;
int alarmEditField = 0;
int editAlarmHour = 7;
int editAlarmMinute = 0;
int editAlarmTrack = 1;
bool alarmRinging = false;
volatile bool alarmPagePending = false;
enum CurrentPage alarmReturnPage = PAGE2;
bool eventEnabled = false;
int eventYear = 0;
int eventMonth = 0;
int eventDay = 0;
String eventText = "";
bool eventEnabledList[EVENT_SLOT_COUNT] = {false, false, false};
int eventYearList[EVENT_SLOT_COUNT] = {0, 0, 0};
int eventMonthList[EVENT_SLOT_COUNT] = {0, 0, 0};
int eventDayList[EVENT_SLOT_COUNT] = {0, 0, 0};
int eventHourList[EVENT_SLOT_COUNT] = {8, 8, 8};
int eventMinuteList[EVENT_SLOT_COUNT] = {0, 0, 0};
String eventTextList[EVENT_SLOT_COUNT] = {"", "", ""};
int activeEventIndex = -1;
bool eventRinging = false;
volatile bool eventPagePending = false;
bool sht30Ready = false;
bool bh1750Ready = false;
bool ds3231Ready = false;
HardwareSerial jq(2);
CRGB ws2812Leds[WS2812_LED_COUNT];
bool clockSynced = false;
unsigned long baseEpoch = 0;
unsigned long baseMillis = 0;
int lastAlarmDay = -1;
int lastEventDay = -1;
unsigned long lastFireVoiceMillis = 0;
unsigned long lastTheftVoiceMillis = 0;
CRGB lastWarningLight = CRGB::Black;
const uint8_t JQ_DEFAULT_VOLUME = 8;
const uint16_t JQ_BUTTON_TRACK = 1;
const uint16_t JQ_ALARM_TRACKS[] = {1, 4, 5};
const uint8_t JQ_ALARM_TRACK_COUNT = sizeof(JQ_ALARM_TRACKS) / sizeof(JQ_ALARM_TRACKS[0]);
const uint16_t JQ_THEFT_TRACK = 2;
const uint16_t JQ_FIRE_TRACK = 3;
const unsigned long FIRE_VOICE_INTERVAL_MS = 6000UL;
const unsigned long THEFT_VOICE_INTERVAL_MS = 6000UL;
// ADC鍖哄煙
float batteryMin = 2.9f;
float batteryMax = 4.2f;
float batteryVoltage = 0.0f;
int batteryPercent = 0;
float tmpVoltage = 0.00; // 璁板綍涓存椂鍩哄噯鐢靛帇
bool Charging = false; // 鏄惁姝ｅ湪鍏呯數涓?
int creaseTimes = 0;
int decreaseTimes = 0;
float adcOffset = 0.02;

void updateWarningLight();
tm currentTimeInfo();
void jqPlayTrack(uint16_t track);
void jqStop();

bool fireAlertActive(){
  return fireAlarm && !fireAlarmAcknowledged;
}

bool theftAlertActive(){
  return !fireAlertActive() && antiTheftMode && infraredDetected && !theftAlarmAcknowledged;
}

void acknowledgeActiveWarning(){
  bool hadWarning = fireAlertActive() || theftAlertActive();
  if(fireAlertActive()){
    fireAlarmAcknowledged = true;
    if(simulatedFireAlarm){
      simulatedFireAlarm = false;
      fireAlarm = (digitalRead(PIN_MQ2) == MQ2_TRIGGER_LEVEL);
      if(!fireAlarm){
        fireAlarmAcknowledged = false;
      }
    }
  }
  if(theftAlertActive()){
    theftAlarmAcknowledged = true;
  }
  if(hadWarning){
    jqStop();
    sensorStateChanged = false;
    updateWarningLight();
  }
}

uint16_t normalizeAlarmTrack(int track){
  for(uint8_t i = 0; i < JQ_ALARM_TRACK_COUNT; i++){
    if(track == JQ_ALARM_TRACKS[i]){
      return track;
    }
  }
  return JQ_ALARM_TRACKS[0];
}

uint16_t nextAlarmTrack(uint16_t track, int delta){
  int index = 0;
  for(uint8_t i = 0; i < JQ_ALARM_TRACK_COUNT; i++){
    if(track == JQ_ALARM_TRACKS[i]){
      index = i;
      break;
    }
  }
  index = (index + JQ_ALARM_TRACK_COUNT + delta) % JQ_ALARM_TRACK_COUNT;
  return JQ_ALARM_TRACKS[index];
}

void setSimulatedFireAlarm(bool enabled, bool forceNotify){
  bool oldFireAlarm = fireAlarm;
  simulatedFireAlarm = enabled;
  if(enabled){
    fireAlarmAcknowledged = false;
    fireAlarm = true;
  }else{
    fireAlarm = (digitalRead(PIN_MQ2) == MQ2_TRIGGER_LEVEL);
    if(!fireAlarm){
      fireAlarmAcknowledged = false;
    }
  }
  if(fireAlarm != oldFireAlarm){
    sensorStateChanged = true;
  }
  if(fireAlarm && voice && (forceNotify || !oldFireAlarm || millis() - lastFireVoiceMillis >= FIRE_VOICE_INTERVAL_MS)){
    jqPlayTrack(JQ_FIRE_TRACK);
    lastFireVoiceMillis = millis();
  }
  if(fireAlarm && (forceNotify || !oldFireAlarm)){
    requestFireEmailAlert(forceNotify);
  }
  updateWarningLight();
  drawCurrentPage();
}

void startAlarmEdit(){
  editAlarmHour = alarmHour;
  editAlarmMinute = alarmMinute;
  editAlarmTrack = normalizeAlarmTrack(alarmTrack);
  alarmEditField = 0;
  modalShowed = true;
  drawAlarmModal(true);
}

void skipTodayIfAlarmMatchesNow(){
  if(clockReady() && currentHour() == alarmHour && currentMinute() == alarmMinute){
    tm info = currentTimeInfo();
    lastAlarmDay = info.tm_year * 400 + info.tm_yday;
  }else{
    lastAlarmDay = -1;
  }
}

void finishAlarmEdit(){
  alarmHour = editAlarmHour;
  alarmMinute = editAlarmMinute;
  alarmTrack = normalizeAlarmTrack(editAlarmTrack);
  modalShowed = false;
  alarmEditField = 0;
  alarmEnabled = true;
  alarmRinging = false;
  skipTodayIfAlarmMatchesNow();
  updateWarningLight();
  setAlarmPrefs();
  if(currentPage == PAGE1){
    buttonEnable = false;
    drawPage1();
    buttonEnable = true;
  }else if(currentPage == CONFIG){
    drawConfig();
  }
}

void alarmEditNextField(){
  if(alarmEditField < 2){
    alarmEditField++;
    drawAlarmModal(false);
  }else{
    finishAlarmEdit();
  }
}

void adjustAlarmEdit(int delta){
  if(alarmEditField == 0){
    editAlarmHour = (editAlarmHour + 24 + delta) % 24;
  }else if(alarmEditField == 1){
    editAlarmMinute = (editAlarmMinute + 60 + delta) % 60;
  }else{
    editAlarmTrack = nextAlarmTrack(editAlarmTrack, delta);
  }
  drawAlarmModal(false);
}

void cancelModalOrEdit(){
  modalShowed = false;
  alarmEditField = 0;
  if(currentPage == PAGE1){
    drawPage1();
  }else{
    drawConfig();
  }
}

void stopAlarmRinging(){
  if(!alarmRinging){
    return;
  }
  alarmRinging = false;
  jqStop();
  alarmPagePending = false;
  modalShowed = false;
  alarmEditField = 0;
  updateWarningLight();
  if(alarmReturnPage == SETTING){
    alarmReturnPage = PAGE2;
  }
  currentPage = alarmReturnPage;
  drawCurrentPage();
  lastRefresh = millis();
}

void stopEventRinging(){
  if(!eventRinging){
    return;
  }
  eventRinging = false;
  eventPagePending = false;
  activeEventIndex = -1;
  jqStop();
  updateWarningLight();
  sensorStateChanged = true;
  drawCurrentPage();
  lastRefresh = millis();
}

// Board sensor helpers
bool i2cDeviceReady(uint8_t address){
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}

uint8_t bcdToDec(uint8_t value){
  return ((value >> 4) * 10) + (value & 0x0F);
}

bool clockReady(){
  return clockSynced || timeClient.isTimeSet();
}

void setClockEpoch(unsigned long epoch){
  if(epoch == 0){
    return;
  }
  baseEpoch = epoch;
  baseMillis = millis();
  clockSynced = true;
}

unsigned long currentEpoch(){
  if(timeClient.isTimeSet()){
    setClockEpoch(timeClient.getEpochTime());
  }
  if(clockSynced){
    return baseEpoch + ((millis() - baseMillis) / 1000);
  }
  return 0;
}

tm currentTimeInfo(){
  unsigned long epoch = currentEpoch();
  time_t raw = (time_t)epoch;
  tm info = {};
  if(epoch > 0){
    tm *ptr = gmtime(&raw);
    if(ptr != nullptr){
      info = *ptr;
    }
  }
  return info;
}

String twoDigits(int value){
  return value < 10 ? "0" + String(value) : String(value);
}

String currentFormattedTime(){
  if(timeClient.isTimeSet()){
    return timeClient.getFormattedTime();
  }
  if(!clockSynced){
    return "00:00:00";
  }
  tm info = currentTimeInfo();
  return twoDigits(info.tm_hour) + ":" + twoDigits(info.tm_min) + ":" + twoDigits(info.tm_sec);
}

String displayTimeText(bool includeSeconds){
  int h = currentHour();
  int m = currentMinute();
  int s = currentSecond();
  if(use24HourFormat){
    String text = twoDigits(h) + ":" + twoDigits(m);
    if(includeSeconds){
      text += ":" + twoDigits(s);
    }
    return text;
  }
  int h12 = h % 12;
  if(h12 == 0){
    h12 = 12;
  }
  String text = twoDigits(h12) + ":" + twoDigits(m);
  if(includeSeconds){
    text += ":" + twoDigits(s);
  }
  text += h < 12 ? " AM" : " PM";
  return text;
}

int currentHour(){
  return currentTimeInfo().tm_hour;
}

int currentMinute(){
  return currentTimeInfo().tm_min;
}

int currentSecond(){
  return currentTimeInfo().tm_sec;
}

void syncTimeFromDS3231(){
  if(!ds3231Ready){
    return;
  }
  Wire.beginTransmission(0x68);
  Wire.write(0x00);
  if(Wire.endTransmission() != 0){
    return;
  }
  if(Wire.requestFrom(0x68, 7) != 7){
    return;
  }
  int second = bcdToDec(Wire.read() & 0x7F);
  int minute = bcdToDec(Wire.read());
  int hour = bcdToDec(Wire.read() & 0x3F);
  Wire.read();
  int day = bcdToDec(Wire.read());
  int month = bcdToDec(Wire.read() & 0x1F);
  int year = 2000 + bcdToDec(Wire.read());
  struct tm tmTime = {};
  tmTime.tm_year = year - 1900;
  tmTime.tm_mon = month - 1;
  tmTime.tm_mday = day;
  tmTime.tm_hour = hour;
  tmTime.tm_min = minute;
  tmTime.tm_sec = second;
  time_t epoch = mktime(&tmTime);
  if(epoch > 0){
    setClockEpoch((unsigned long)epoch);
  }

  logInfo("DS3231 time: ");
  logInfo(String(year));
  logInfo("-");
  logInfo(String(month));
  logInfo("-");
  logInfo(String(day));
  logInfo(" ");
  logInfo(String(hour));
  logInfo(":");
  logInfo(String(minute));
  logInfo(":");
  logInfoln(String(second));
}

void writeTimeToDS3231(){
  if(!ds3231Ready || !timeClient.isTimeSet()){
    return;
  }
  unsigned long epochTime = timeClient.getEpochTime();
  time_t rawTime = (time_t)epochTime;
  struct tm *tm_ptr = gmtime(&rawTime);
  if(tm_ptr == nullptr){
    return;
  }
  auto decToBcd = [](uint8_t value) -> uint8_t {
    return ((value / 10) << 4) | (value % 10);
  };

  Wire.beginTransmission(0x68);
  Wire.write(0x00);
  Wire.write(decToBcd(tm_ptr->tm_sec));
  Wire.write(decToBcd(tm_ptr->tm_min));
  Wire.write(decToBcd(tm_ptr->tm_hour));
  Wire.write(decToBcd(tm_ptr->tm_wday + 1));
  Wire.write(decToBcd(tm_ptr->tm_mday));
  Wire.write(decToBcd(tm_ptr->tm_mon + 1));
  Wire.write(decToBcd((tm_ptr->tm_year + 1900) - 2000));
  Wire.endTransmission();
  syncClockFromNTP();
}

void syncClockFromNTP(){
  if(timeClient.isTimeSet()){
    setClockEpoch(timeClient.getEpochTime());
  }
}

bool setClockDateTime(int year, int month, int day, int hour, int minute, int second){
  if(year < 2020 || year > 2099 || month < 1 || month > 12 || day < 1 || day > 31 ||
     hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59){
    return false;
  }

  struct tm tmTime = {};
  tmTime.tm_year = year - 1900;
  tmTime.tm_mon = month - 1;
  tmTime.tm_mday = day;
  tmTime.tm_hour = hour;
  tmTime.tm_min = minute;
  tmTime.tm_sec = second;
  time_t epoch = mktime(&tmTime);
  if(epoch <= 0){
    return false;
  }
  setClockEpoch((unsigned long)epoch);

  if(ds3231Ready){
    auto decToBcd = [](uint8_t value) -> uint8_t {
      return ((value / 10) << 4) | (value % 10);
    };
    Wire.beginTransmission(0x68);
    Wire.write(0x00);
    Wire.write(decToBcd(second));
    Wire.write(decToBcd(minute));
    Wire.write(decToBcd(hour));
    Wire.write(decToBcd(tmTime.tm_wday + 1));
    Wire.write(decToBcd(day));
    Wire.write(decToBcd(month));
    Wire.write(decToBcd(year - 2000));
    Wire.endTransmission();
  }
  return true;
}

void jqSendCommand(uint8_t command, const uint8_t *data, uint8_t dataLength){
  uint8_t checksum = 0xAA + command + dataLength;
  jq.write((uint8_t)0xAA);
  jq.write(command);
  jq.write(dataLength);
  for(uint8_t i = 0; i < dataLength; i++){
    jq.write(data[i]);
    checksum += data[i];
  }
  jq.write(checksum);
}

void jqSendCommand(uint8_t command){
  jqSendCommand(command, nullptr, 0);
}

void jqSetVolume(uint8_t volume){
  uint8_t data[] = { (uint8_t)constrain(volume, 0, 30) };
  jqSendCommand(0x13, data, 1);
  logInfo("JQ8900 volume set to ");
  logInfoln(String(constrain(volume, 0, 30)));
}

void jqPlayTrack(uint16_t track){
  logInfo("JQ8900 play track ");
  logInfoln(String(track));
  uint8_t data[] = { (uint8_t)(track >> 8), (uint8_t)(track & 0xFF) };
  jqSendCommand(0x07, data, 2);
}

void jqStop(){
  logInfoln("JQ8900 stop");
  jqSendCommand(0x04);
}

void playVoiceTrack(int track){
  jqPlayTrack((uint16_t)constrain(track, 1, 999));
}

void sensorsInit(){
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  pinMode(PIN_MQ2, INPUT);
  pinMode(PIN_IR, INPUT);
  jq.begin(9600, SERIAL_8N1, PIN_JQ_RX_FROM_MODULE, PIN_JQ_TX_TO_MODULE);
  jqSetVolume(JQ_DEFAULT_VOLUME);
  FastLED.addLeds<WS2812B, PIN_WS2812, WS2812_COLOR_ORDER>(ws2812Leds, WS2812_LED_COUNT);
  FastLED.setBrightness(96);
  FastLED.clear(true);

  sht30Ready = i2cDeviceReady(0x44);
  bh1750Ready = i2cDeviceReady(0x23);
  ds3231Ready = i2cDeviceReady(0x68);

  if(bh1750Ready){
    Wire.beginTransmission(0x23);
    Wire.write(0x10);
    Wire.endTransmission();
  }
  if(ds3231Ready){
    syncTimeFromDS3231();
  }

  logInfoln(sht30Ready ? "SHT30 init ok" : "SHT30 not found");
  logInfoln(bh1750Ready ? "BH1750 init ok" : "BH1750 not found");
  logInfoln(ds3231Ready ? "DS3231 init ok" : "DS3231 not found");
  logInfoln("JQ8900 init ok");
}
// 鎸夐敭澹?
void Dida(){
  // Keep button sound disabled during hardware bring-up to avoid power dips.
}
///////////////////////////////////Freertos鍖哄煙///////////////////////////////////////
// Sensor data
void getDHTData(){
  if(!sht30Ready){
    return;
  }
  Wire.beginTransmission(0x44);
  Wire.write(0x2C);
  Wire.write(0x06);
  if(Wire.endTransmission() != 0){
    return;
  }
  vTaskDelay(20);
  if(Wire.requestFrom(0x44, 6) != 6){
    return;
  }
  uint16_t rawTemp = ((uint16_t)Wire.read() << 8) | Wire.read();
  Wire.read();
  uint16_t rawHum = ((uint16_t)Wire.read() << 8) | Wire.read();
  Wire.read();
  float tmp = -45.0f + 175.0f * ((float)rawTemp / 65535.0f);
  float hum = 100.0f * ((float)rawHum / 65535.0f);
  tmp-=tempOffset; 
  if(temperature.equals("0.0")){
    temperature = String(tmp, 1);
    humidity = String(hum, 1);
  }else{
    // 鍙栧钩鍧囧€硷紝澧炲姞鏁版嵁骞虫粦鎬?
    temperature = String((temperature.toFloat() + tmp) / 2, 1);
    humidity = String((humidity.toFloat() + hum) / 2, 1);
  }
  logDebug("temperature: ");logDebug(temperature);logDebugln(" C");
  logDebug("humidity: ");logDebug(humidity);logDebugln(" %RH");
  logDebugln("------------------------------------------------------");
}

void getBH1750Data(){
  if(!bh1750Ready){
    return;
  }
  if(Wire.requestFrom(0x23, 2) != 2){
    return;
  }
  uint16_t rawLux = ((uint16_t)Wire.read() << 8) | Wire.read();
  lightLux = rawLux / 1.2f;
}

void setWarningLight(const CRGB &color){
  fill_solid(ws2812Leds, WS2812_LED_COUNT, color);
  FastLED.show();
}

void updateWarningLight(){
  CRGB target = CRGB::Black;
  if(fireAlertActive()){
    target = CRGB::Red;
  }else if(alarmRinging){
    target = CRGB::Yellow;
  }else if(theftAlertActive()){
    target = CRGB::Red;
  }
  if(target != lastWarningLight){
    setWarningLight(target);
    lastWarningLight = target;
  }
}

void scanAlarmInputs(){
  bool oldFireAlarm = fireAlarm;
  bool oldInfraredDetected = infraredDetected;
  int mq2 = digitalRead(PIN_MQ2);
  int ir = digitalRead(PIN_IR);
  fireAlarm = simulatedFireAlarm || (mq2 == MQ2_TRIGGER_LEVEL);
  infraredDetected = (ir == IR_TRIGGER_LEVEL);
  if(fireAlarm && !oldFireAlarm){
    fireAlarmAcknowledged = false;
    lastFireVoiceMillis = 0;
  }else if(!fireAlarm){
    fireAlarmAcknowledged = false;
  }
  if(antiTheftMode && infraredDetected && !oldInfraredDetected){
    theftAlarmAcknowledged = false;
    lastTheftVoiceMillis = 0;
  }else if(!infraredDetected || !antiTheftMode){
    theftAlarmAcknowledged = false;
  }
  if(fireAlarm != oldFireAlarm || infraredDetected != oldInfraredDetected){
    sensorStateChanged = true;
  }
  if(fireAlertActive() && voice && (lastFireVoiceMillis == 0 || millis() - lastFireVoiceMillis >= FIRE_VOICE_INTERVAL_MS)){
    jqPlayTrack(JQ_FIRE_TRACK);
    lastFireVoiceMillis = millis();
  }
  if(fireAlarm && !oldFireAlarm){
    requestFireEmailAlert();
  }
  if(theftAlertActive() && voice && (lastTheftVoiceMillis == 0 || millis() - lastTheftVoiceMillis >= THEFT_VOICE_INTERVAL_MS)){
    jqPlayTrack(JQ_THEFT_TRACK);
    lastTheftVoiceMillis = millis();
  }
  updateWarningLight();
  logDebug("MQ2: ");logDebugln(String(mq2));
  logDebug("IR: ");logDebugln(String(ir));
}

static long daysUntilEvent(int index){
  if(index < 0 || index >= EVENT_SLOT_COUNT || !clockReady()){
    return 99999;
  }
  struct tm eventTime = {};
  eventTime.tm_year = eventYearList[index] - 1900;
  eventTime.tm_mon = eventMonthList[index] - 1;
  eventTime.tm_mday = eventDayList[index];
  eventTime.tm_hour = eventHourList[index];
  eventTime.tm_min = eventMinuteList[index];
  eventTime.tm_sec = 0;
  time_t eventEpoch = mktime(&eventTime);
  if(eventEpoch <= 0){
    return 99999;
  }

  tm nowInfo = currentTimeInfo();
  tm todayInfo = nowInfo;
  todayInfo.tm_hour = 0;
  todayInfo.tm_min = 0;
  todayInfo.tm_sec = 0;
  tm eventDayInfo = eventTime;
  eventDayInfo.tm_hour = 0;
  eventDayInfo.tm_min = 0;
  eventDayInfo.tm_sec = 0;
  time_t todayEpoch = mktime(&todayInfo);
  time_t eventDayEpoch = mktime(&eventDayInfo);
  return (eventDayEpoch - todayEpoch) / 86400;
}

String eventReminderText(){
  if(!clockReady()){
    return "";
  }

  int bestIndex = -1;
  time_t bestEpoch = 2147483647;
  for(int i = 0; i < EVENT_SLOT_COUNT; i++){
    if(!eventEnabledList[i] || eventTextList[i].length() == 0 || daysUntilEvent(i) != 1){
      continue;
    }
    struct tm eventTime = {};
    eventTime.tm_year = eventYearList[i] - 1900;
    eventTime.tm_mon = eventMonthList[i] - 1;
    eventTime.tm_mday = eventDayList[i];
    eventTime.tm_hour = eventHourList[i];
    eventTime.tm_min = eventMinuteList[i];
    eventTime.tm_sec = 0;
    time_t eventEpoch = mktime(&eventTime);
    if(eventEpoch > 0 && eventEpoch < bestEpoch){
      bestEpoch = eventEpoch;
      bestIndex = i;
    }
  }
  if(bestIndex >= 0){
    return "Tomorrow: " + eventTextList[bestIndex];
  }
  return "";
}

int nextEventIndex(){
  if(!clockReady()){
    for(int i = 0; i < EVENT_SLOT_COUNT; i++){
      if(eventEnabledList[i] && eventTextList[i].length() > 0){
        return i;
      }
    }
    return -1;
  }

  int bestIndex = -1;
  long bestDelta = 2147483647L;
  time_t nowEpoch = (time_t)currentEpoch();
  for(int i = 0; i < EVENT_SLOT_COUNT; i++){
    if(!eventEnabledList[i] || eventTextList[i].length() == 0){
      continue;
    }
    struct tm eventTime = {};
    eventTime.tm_year = eventYearList[i] - 1900;
    eventTime.tm_mon = eventMonthList[i] - 1;
    eventTime.tm_mday = eventDayList[i];
    eventTime.tm_hour = eventHourList[i];
    eventTime.tm_min = eventMinuteList[i];
    eventTime.tm_sec = 0;
    time_t eventEpoch = mktime(&eventTime);
    if(eventEpoch <= 0){
      continue;
    }
    long delta = eventEpoch - nowEpoch;
    if(delta >= 0 && delta < bestDelta){
      bestDelta = delta;
      bestIndex = i;
    }
  }
  return bestIndex;
}

String eventDateText(int index){
  if(index < 0 || index >= EVENT_SLOT_COUNT || eventYearList[index] <= 0){
    return "--";
  }
  return String(eventYearList[index]) + "/" + twoDigits(eventMonthList[index]) + "/" +
         twoDigits(eventDayList[index]) + " " + twoDigits(eventHourList[index]) + ":" +
         twoDigits(eventMinuteList[index]);
}

String eventScreenText(int index){
  if(index < 0 || index >= EVENT_SLOT_COUNT || eventTextList[index].length() == 0){
    return "No event";
  }
  return eventTextList[index];
}

void syncEventSummary(){
  int index = nextEventIndex();
  if(index < 0){
    eventEnabled = false;
    eventYear = 0;
    eventMonth = 0;
    eventDay = 0;
    eventText = "";
    return;
  }
  eventEnabled = eventEnabledList[index];
  eventYear = eventYearList[index];
  eventMonth = eventMonthList[index];
  eventDay = eventDayList[index];
  eventText = eventTextList[index];
}

void checkEventReminder(){
  if(!clockReady()){
    return;
  }
  tm info = currentTimeInfo();
  int todayKey = info.tm_year * 400 + info.tm_yday;

  for(int i = 0; i < EVENT_SLOT_COUNT; i++){
    if(!eventEnabledList[i] || eventTextList[i].length() == 0){
      continue;
    }
    int reminderKey = todayKey * EVENT_SLOT_COUNT + i;
    if(daysUntilEvent(i) == 1 && lastEventDay != reminderKey){
      activeEventIndex = i;
      eventRinging = false;
      eventPagePending = false;
      lastEventDay = reminderKey;
      sensorStateChanged = true;
      syncEventSummary();
      logInfoln("Event reminder subtitle shown");
      return;
    }
  }
  syncEventSummary();
}

void playAlarmSound(){
  if(!voice){
    return;
  }
  jqPlayTrack(normalizeAlarmTrack(alarmTrack));
}

void checkAlarm(){
  if(!alarmEnabled || !clockReady()){
    if(alarmRinging){
      alarmRinging = false;
      updateWarningLight();
    }
    return;
  }
  if(currentPage == PAGE1 && modalShowed){
    return;
  }
  tm info = currentTimeInfo();
  int dayKey = info.tm_year * 400 + info.tm_yday;
  if(info.tm_hour == alarmHour && info.tm_min == alarmMinute && lastAlarmDay != dayKey){
    alarmRinging = true;
    lastAlarmDay = dayKey;
    logInfoln("Alarm triggered");
    playAlarmSound();
    updateWarningLight();
    modalShowed = false;
    alarmEditField = 0;
    alarmPagePending = true;
  }
}
uint32_t adcRead(){
#if PIN_BAT_ADC >= 0
  long sum = 0;
  for (int i = 0; i < ADC_FREQUENCY; i++){
    sum += analogReadMilliVolts(PIN_BAT_ADC);
    vTaskDelay(1);
  }
  return sum / ADC_FREQUENCY;
#else
  return 0;
#endif
}
// 浠诲姟鍙ユ焺
TaskHandle_t anotherCoreTask;
TaskHandle_t drawLoadingTask;
TaskHandle_t fadeOnTask;
TaskHandle_t adcTask;
// 浠诲姟鍐呭
void anotherCore_task(void *pvParameters){
  logInfoln(String("鏍稿績") + String(xPortGetCoreID()) + String("寮€濮嬫墽琛屼紶鎰熷櫒浠诲姟"));
  // 鍒濆鍖栨寜閿?
  btnInit();
  logInfoln("Buttons init ok");
  // 寮€濮嬪畾鏃舵洿鏂板ぉ姘旂殑浠诲姟
  startTickerUpdateWeather();
  // 寰幆鑾峰彇鏁版嵁
  while(true){
    if(mode == ONLINE_MODE && wifiConnected()){
      if(timeClient.update()){
        syncClockFromNTP();
        writeTimeToDS3231();
      }
    }    
    getDHTData();
    getBH1750Data();
    scanAlarmInputs();
    checkAlarm();
    checkEventReminder();
    updateWarningLight();
    vTaskDelay(1000);
  }
  vTaskDelete(anotherCoreTask);
}
void drawLoading_task(void *pvParameters){
  String text = (char *)pvParameters;
  int angle = 0;
  drawLoading(true, text, &angle);
  while(loadingAnim){
    drawLoading(false, text, &angle);
    vTaskDelay(10);
  }
  vTaskDelete(drawLoadingTask);
}
void fadeOn_task(void *pvParameters){
  while(true){
    fadeOn();
    break;
  }
  vTaskDelete(fadeOnTask);
}
void adc_task(void *pvParameters){
  logInfoln(String("鏍稿績") + String(xPortGetCoreID()) + String("寮€濮嬫墽琛孉DC閲囨牱浠诲姟"));
#if PIN_BAT_ADC < 0
  batteryVoltage = 0.0f;
  batteryPercent = 100;
  Charging = true;
  logInfoln("Battery ADC disabled");
  vTaskDelete(adcTask);
  return;
#else
  analogReadResolution(12);
  analogSetPinAttenuation(PIN_BAT_ADC, ADC_11db);
#endif
  // 寰幆鑾峰彇鏁版嵁
  while(true){
    // 鑾峰彇adc鐢靛帇(mv)
    float adcVoltage = (float)adcRead() / 1000.0f + adcOffset;
    // logDebug("ADC鐢靛帇: ");
    // logDebug(String(adcVoltage));
    // logDebugln("V");
    // 鍒嗗帇鐢甸樆闃诲€?:1锛屾墍浠ュ疄闄呯數鍘嬩负ADC鐢靛帇鐨勫洓鍊?
    batteryVoltage = 4 * adcVoltage;
    if(tmpVoltage == 0.00){
      tmpVoltage = batteryVoltage;
    }else{
      if(batteryVoltage >= 4.3f){// 澶т簬4.3V锛岃偗瀹氭槸typeC渚涚數
        Charging = true;
        tmpVoltage = batteryVoltage;
      }else{
        if((batteryVoltage - tmpVoltage) > 0.01){
          creaseTimes++;
          if(creaseTimes >= 10){
            Charging = true;
            tmpVoltage = batteryVoltage;
            creaseTimes = 0;
          }
        }else{
          creaseTimes = 0;
        }
        if((tmpVoltage - batteryVoltage) > 0.01){
          decreaseTimes++;
          if(decreaseTimes >= 5){
            Charging = false;
            tmpVoltage = batteryVoltage;
            decreaseTimes = 0;
          }
        }else{
          decreaseTimes = 0;
        }
      }
    }
    batteryPercent = ((batteryVoltage - batteryMin) / (batteryMax - batteryMin)) * 100;
    if(batteryPercent > 100){
      batteryPercent = 100;
    }
    if(batteryPercent < 0){
      batteryPercent = 0;
    }
    logDebug("鐢垫睜鐢靛帇: ");
    logDebug(String(batteryVoltage));
    logDebugln("V");        
    logDebug("鐢甸噺: ");
    logDebug(String(batteryPercent));
    logDebugln("%");
    logDebugln("======================================================");
    vTaskDelay(500);
  }
  vTaskDelete(adcTask);
}
// 鍒涘缓浠诲姟
void createAnotherCoreTask(){
  xTaskCreatePinnedToCore(anotherCore_task, "anotherCore_task", 6 * 1024, NULL, 1, &anotherCoreTask, 0);
}
void createDrawLoadingTask(const char *text){
  loadingAnim = false;
  int angle = 0;
  drawLoading(true, String(text), &angle);
}
void createFadeOnTask(){
  fadeOn();
}
void createADCTask(){
  xTaskCreatePinnedToCore(adc_task, "adc_task", 2 * 1024, NULL, 1, &adcTask, 0);
}
//////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////瀹氭椂鍣ㄥ尯鍩?/////////////////////////////////////////
Ticker ticker_updateWeather;
void updateWeatherTask(){
  if(mode == OFFLINE_MODE || !wifiConnected()){
    return;
  }
  updateWeather = true;
}
void startTickerUpdateWeather(){
  // 姣忛殧涓€娈垫椂闂存洿鏂颁竴娆″ぉ姘?
  ticker_updateWeather.attach(UPDATE_WEATHER_INTERVAL, updateWeatherTask);
}
//////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////// 鎸夐敭鍖?///////////////////////////////////////
// 鎸夐敭鏂规硶
void showPrevMainPage(){
  switch(currentPage){
    case PAGE1:
      currentPage = CALENDAR;
      monthOffset = 0;
      offsetDerection = CALENDAR_RIGHT_OFFSET;
      drawCalendar();
      break;
    case PAGE2:
      currentPage = CONFIG;
      drawConfig();
      break;
    case PAGE3:
      currentPage = PAGE2;
      drawPage2Full();
      break;
    case CALENDAR:
      currentPage = PAGE3;
      drawPage3(true);
      break;
    case CONFIG:
      currentPage = PAGE1;
      drawPage1();
      break;
    default:
      break;
  }
  lastRefresh = millis();
}

void showNextMainPage(){
  switch(currentPage){
    case PAGE2:
      currentPage = PAGE3;
      drawPage3(true);
      break;
    case PAGE3:
      currentPage = CALENDAR;
      monthOffset = 0;
      offsetDerection = CALENDAR_RIGHT_OFFSET;
      drawCalendar();
      break;
    case CALENDAR:
      currentPage = PAGE1;
      drawPage1();
      break;
    case PAGE1:
      currentPage = CONFIG;
      drawConfig();
      break;
    case CONFIG:
      currentPage = PAGE2;
      drawPage2Full();
      break;
    default:
      break;
  }
  lastRefresh = millis();
}

void leaveConfigPage(){
  buttonEnable = false;
  currentPage = PAGE2;
  drawPage2Full();
  lastRefresh = millis();
  buttonEnable = true;
}

void btn1click(){
  if(!buttonEnable){
    return;
  }
  Dida();
  lastUserAction = millis();
  if(fireAlertActive() || theftAlertActive()){
    drawCurrentPage();
    return;
  }
  switch(currentPage){
    int lastChoosedIndex;
    case SETTING:
      settingChoosed = !settingChoosed;
      drawSettingOrOffline(false, "");
      break;
    case PAGE1:
      if(modalShowed){
        adjustAlarmEdit(1);
      }else{
        buttonEnable = false;
        showNextMainPage();
        buttonEnable = true;
      }
      break;
    case PAGE2:
    case PAGE3:
      buttonEnable = false;
      showNextMainPage();
      buttonEnable = true;
      break;
    case CALENDAR:
      buttonEnable = false;
      monthOffset++;
      offsetDerection = CALENDAR_RIGHT_OFFSET;
      drawCalendar();
      buttonEnable = true;
      break;
    case CONFIG:
      if(!modalShowed){
        buttonEnable = false;
        lastChoosedIndex = configChoosedIndex;
        configChoosedIndex = (configChoosedIndex + 1) < OPTION_COUNT ? (configChoosedIndex + 1) : 0;
        drawConfigOption(lastChoosedIndex);
        drawConfigOption(configChoosedIndex);
        buttonEnable = true;
      }else{
        if(configChoosedIndex == OPTION_WLAN || configChoosedIndex == OPTION_RESET){
          modalLeftChoosed = !modalLeftChoosed;
          drawModal("", false);
        }
      }
      break;
    default:
      break;
  }
}
void btn2click(){
  if(!buttonEnable){
    return;
  }
  Dida();
  lastUserAction = millis();
  if(fireAlertActive() || theftAlertActive()){
    acknowledgeActiveWarning();
    return;
  }
  if(alarmRinging){
    stopAlarmRinging();
    return;
  }
  if(eventRinging){
    stopEventRinging();
    return;
  }
  switch(currentPage){
    case SETTING:
      if(settingChoosed){ // 寮€濮嬪惎鍔ㄦ湇鍔″櫒閰嶇綉鎴栭噸鍚噸鏂拌幏鍙栨暟鎹?
        if(getDataFailed){
          ESP.restart();
        }else{
          buttonEnable = false;
          logInfoln("BTN2 start AP");
          createDrawLoadingTask("Starting AP");
          // 寮€鍚疉P閰嶇綉
          wifiConfigBySoftAP();
          logInfoln("AP started");
          // 鍏抽棴鍔犺浇鍔ㄧ敾
          loadingAnim = false;
          fadeOff();
          delay(200);
          // 缁樺埗鏂囧瓧绛夊緟鐢ㄦ埛杩炴帴
          draw2LineText("Connect CC Air Detector", "Open 192.168.1.1");
          createFadeOnTask();
        }      
      }else{ // 绂荤嚎妯″紡锛岃繘鍏age1
        getDataFailed = false;
        mode = OFFLINE_MODE;
        currentPage = PAGE2;
        drawPage2Full();
      }
      break;
    case PAGE1:
      if(!modalShowed){
        startAlarmEdit();
      }else{
        alarmEditNextField();
      }
      break;
    case CONFIG:
      switch(configChoosedIndex){
        case OPTION_VOICE:
          voice = !voice;
          setVoice();
          drawConfigOption(configChoosedIndex);
          break;
        case OPTION_OFFSET:
          use24HourFormat = !use24HourFormat;
          setTimeFormatPrefs();
          displayMinute = -1;
          drawConfigOption(configChoosedIndex);
          break;
        case OPTION_THEME:
          exchangeTheme();
          drawConfig();
          break;
        case OPTION_SECURITY:
          antiTheftMode = !antiTheftMode;
          sensorStateChanged = true;
          setAlarmPrefs();
          drawConfigOption(configChoosedIndex);
          break;
        case OPTION_SMOKE_TEST:
          setSimulatedFireAlarm(!simulatedFireAlarm);
          drawConfigOption(configChoosedIndex);
          break;
        case OPTION_ALARM:
          alarmEnabled = !alarmEnabled;
          if(!alarmEnabled){
            alarmRinging = false;
            jqStop();
            updateWarningLight();
          }
          setAlarmPrefs();
          drawConfigOption(configChoosedIndex);
          break;
        case OPTION_WLAN:
          if(!modalShowed){
            modalLeftChoosed = false;
            drawModal("Start WiFi config?", true);
            modalShowed = true;
          }else{
            if(modalLeftChoosed){
              disconnectWiFi();
              buttonEnable = false;
              currentPage = SETTING;
              logInfoln("Config menu start AP");
              createDrawLoadingTask("Starting AP");
              // 寮€鍚疉P閰嶇綉
              wifiConfigBySoftAP();
              logInfoln("AP started");
              // 鍏抽棴鍔犺浇鍔ㄧ敾
              loadingAnim = false;
              fadeOff();
              delay(200);
              // 缁樺埗鏂囧瓧绛夊緟鐢ㄦ埛杩炴帴
              draw2LineText("Connect CC Air Detector", "Open 192.168.1.1");
              createFadeOnTask();
            }else{
              drawConfig();
              modalShowed = false;
            }
          }
          break;
        case OPTION_RESET:
          if(!modalShowed){
            modalLeftChoosed = false;
            drawModal("Factory reset?", true);
            modalShowed = true;
          }else{
            if(modalLeftChoosed){
              clearInfo();
              draw2LineText("Factory reset done", "Restarting");
              delay(1500);
              ESP.restart(); 
            }else{
              drawConfig();
              modalShowed = false;
            }
          }
          break;
        case OPTION_BACK:
          leaveConfigPage();
          break;
        default:
          break;          
      }
      break;
    case CALENDAR:
      buttonEnable = false;
      showNextMainPage();
      buttonEnable = true;
      break;
    default:
      break;
  }
}
void btn1LongClick(){
  if(!buttonEnable){
    return;
  }
  Dida();
  lastUserAction = millis();
  if(fireAlertActive() || theftAlertActive()){
    drawCurrentPage();
    return;
  }
  if(modalShowed){
    if(currentPage == PAGE1){
      adjustAlarmEdit(-1);
    }else if(configChoosedIndex == OPTION_WLAN || configChoosedIndex == OPTION_RESET){
      modalLeftChoosed = !modalLeftChoosed;
      drawModal("", false);
    }
    return;
  }
  if(currentPage == CALENDAR){
    buttonEnable = false;
    monthOffset--;
    offsetDerection = CALENDAR_LEFT_OFFSET;
    drawCalendar();
    buttonEnable = true;
    return;
  }
  buttonEnable = false;
  showPrevMainPage();
  buttonEnable = true;
}
void btn2LongClick(){
  Dida();
  lastUserAction = millis();
  if(fireAlertActive() || theftAlertActive()){
    acknowledgeActiveWarning();
    return;
  }
  if(alarmRinging){
    stopAlarmRinging();
    return;
  }
  if(eventRinging){
    stopEventRinging();
    return;
  }
  if(currentPage == CONFIG){
    if(modalShowed){
      cancelModalOrEdit();
    }else{
      leaveConfigPage();
    }
    return;
  }
  if(currentPage == PAGE1 && modalShowed){
    cancelModalOrEdit();
    return;
  }
  currentPage = PAGE2;
  drawPage2Full();
}
void btn1DuringLongPress(){
  if(modalShowed){
    if(currentPage == PAGE1){
      adjustAlarmEdit(-1);
    }
    delay(10);
  }
}
// 鍒濆鍖栧悇鎸夐敭
void btnInit(){
  button1.attachClick(btn1click);
  button1.setDebounceMs(20); //璁剧疆娑堟姈鏃堕暱 
  button2.attachClick(btn2click);
  button2.setDebounceMs(20); //璁剧疆娑堟姈鏃堕暱 
  button1.attachLongPressStart(btn1LongClick);
  button1.setPressMs(1200); //璁剧疆闀挎寜鏃堕棿
  button2.attachLongPressStart(btn2LongClick);
  button2.setPressMs(1200); //璁剧疆闀挎寜鏃堕棿
  button1.attachDuringLongPress(btn1DuringLongPress);
  lastUserAction = millis();
}
// 鐩戞帶鎸夐敭
void watchBtn(){
  button1.tick();
  button2.tick();
  if(currentPage != PAGE2 && currentPage != SETTING && !modalShowed && buttonEnable &&
     (millis() - lastUserAction) >= 30000UL){
    currentPage = PAGE2;
    drawPage2Full();
    lastUserAction = millis();
  }
}
//////////////////////////////////////////////////////////////////////////////////////

