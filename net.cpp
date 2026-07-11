#include <HTTPClient.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <time.h>
#include "ArduinoZlib.h"
#include "PreferencesUtil.h"
#include "tftUtil.h"
#include "Task.h"
#include "net.h"
#include "JwtUtil.h"
#include "MailAlert.h"

// 鍜岄澶╂皵韬唤璁よ瘉锛岄渶瑕佹浛鎹㈡垚浣犱滑鑷繁鐨?
char PrivateKey[] = "MC4CAQAwBQYDK2VwBCIEIK7b7pN85ktwKdhibYw/AfCfr+/1kN5oZJdzcof5W+aY";   // 绉侀挜
char PublicKey[] = "MCowBQYDK2VwAyEA25lz/u/297YFdkRrHlEKDDaIrGHaPR/5sp7xtiv2DxM=";        // 鍏挜
String KeyID = "TBWHPNC7M6";                                                              // 鍑嵁ID
String ProjectID = "3B89AM4BP5";                                                          // 椤圭洰ID
String ApiHost = "m66x8a9rh4.re.qweatherapi.com";                                         // API Host

// Wifi鐩稿叧
String ssid;  //WIFI鍚嶇О
String pass;  //WIFI瀵嗙爜
String city;  // 鍩庡競
String adm; // 涓婄骇鍩庡競鍖哄垝
String location; // 鍩庡競ID
String lat; // 缁忓害
String lon; // 绾害
String WifiNames; // 鏍规嵁鎼滅储鍒扮殑wifi鐢熸垚鐨刼ption瀛楃涓?
bool connected = true; // 鏄惁鎴愬姛杩炴帴缃戠粶
// SoftAP鐩稿叧
const char *APssid = "CC Air Detector";
IPAddress staticIP(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 254);
IPAddress subnet(255, 255, 255, 0);
WebServer server(80);
// 鏌ヨ澶╂皵瓒呮椂鏃堕棿(ms)
int queryTimeout = 5000;
// 澶╂皵鎺ュ彛鐩稿叧
static HTTPClient httpClient;
String data = "";
uint8_t *outbuf;
Weather weather; // 璁板綍鏌ヨ鍒扮殑澶╂皵鏁版嵁
bool queryWeatherSuccess = false;
bool queryAirSuccess = false;
String lastCommandResult = "Ready";
// 瀵规椂鐩稿叧
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP, 8 * 3600, TIME_CHECK_INTERVAL * 1000);

static bool appendDecompressedChunk(uint8_t *input, size_t inputSize, size_t outputSize){
  outbuf = (uint8_t *)malloc(sizeof(uint8_t) * outputSize);
  if(outbuf == nullptr){
    logInfoln("decompress malloc failed");
    return false;
  }
  uint32_t outprintsize = 0;
  int result = ArduinoZlib::libmpq__decompress_zlib(input, inputSize, outbuf, outputSize, outprintsize);
  if(result != 0 && outprintsize == 0){
    free(outbuf);
    return false;
  }
  data.reserve(data.length() + outprintsize + 1);
  for(uint32_t i = 0; i < outprintsize; i++){
    data += (char)outbuf[i];
  }
  free(outbuf);
  return true;
}

// 寮€鍚疭oftAP杩涜閰嶇綉
void wifiConfigBySoftAP(){
  // 寮€鍚疉P妯″紡
  startAP();
  // 鎵弿WiFi,骞跺皢鎵弿鍒扮殑WiFi缁勬垚option閫夐」瀛楃涓?
  scanWiFi();
  // 鍚姩鏈嶅姟鍣?
  startServer();
}
// 寮€鍚疉P妯″紡
void startAP(){
  logInfoln("Start AP mode");
  WiFi.enableAP(true); // 浣胯兘AP妯″紡
  //浼犲叆鍙傛暟闈欐€両P鍦板潃,缃戝叧,鎺╃爜
  WiFi.softAPConfig(staticIP, gateway, subnet);
  if (!WiFi.softAP(APssid)) {
    logInfoln("AP start failed");
  }  
  logInfoln("AP started");
}
// 鎵弿WiFi,骞跺皢鎵弿鍒扮殑Wifi缁勬垚option閫夐」瀛楃涓?
void scanWiFi(){
  logInfoln("Scan WiFi");
  int n = WiFi.scanNetworks();
  if (n){
    logInfo("WiFi networks: ");
    logInfo(String(n));
    logInfoln("");
    WifiNames = "";
    for (size_t i = 0; i < n; i++){
      int32_t rssi = WiFi.RSSI(i);
      String signalStrength;
      if(rssi >= -35){
        signalStrength = " (excellent)";
      }else if(rssi >= -50){
        signalStrength = " (strong)";
      }else if(rssi >= -70){
        signalStrength = " (medium)";
      }else{
        signalStrength = " (weak)";
      }
      WifiNames += "<option value='" + WiFi.SSID(i) + "'>" + WiFi.SSID(i) + signalStrength + "</option>";
    }
  }else{
    logInfoln("No WiFi networks");
  }
}
// 澶勭悊404鎯呭喌鐨勫嚱鏁?handleNotFound'
void handleNotFound(){
  handleRoot();
}
// 澶勭悊缃戠珯鏍圭洰褰曠殑璁块棶璇锋眰
void handleRoot(){
  if(mode == ONLINE_MODE || wifiConnected()){
    server.sendHeader("Location", "/control");
    server.send(303);
    return;
  }
  server.send(200,"text/html", ROOT_HTML_PAGE1 + WifiNames + ROOT_HTML_PAGE2);
}

String htmlEscape(const String &value){
  String escaped = value;
  escaped.replace("&", "&amp;");
  escaped.replace("<", "&lt;");
  escaped.replace(">", "&gt;");
  escaped.replace("\"", "&quot;");
  escaped.replace("'", "&#39;");
  return escaped;
}

String jsonEscape(const String &value){
  String escaped;
  escaped.reserve(value.length() + 8);
  for(size_t i = 0; i < value.length(); i++){
    char c = value[i];
    switch(c){
      case '\\':
        escaped += "\\\\";
        break;
      case '"':
        escaped += "\\\"";
        break;
      case '\n':
        escaped += "\\n";
        break;
      case '\r':
        escaped += "\\r";
        break;
      case '\t':
        escaped += "\\t";
        break;
      default:
        escaped += c;
        break;
    }
  }
  return escaped;
}

String checkedAttr(bool value){
  return value ? " checked" : "";
}

static bool parseTimeInText(const String &text, int &hour, int &minute){
  int colon = text.indexOf(':');
  if(colon < 1){
    return false;
  }
  int start = colon - 1;
  while(start > 0 && isDigit(text[start - 1])){
    start--;
  }
  int end = colon + 1;
  while(end < (int)text.length() && isDigit(text[end])){
    end++;
  }
  if(start >= colon || end <= colon + 1){
    return false;
  }
  hour = text.substring(start, colon).toInt();
  minute = text.substring(colon + 1, end).toInt();
  return hour >= 0 && hour <= 23 && minute >= 0 && minute <= 59;
}

static bool parseDateInText(const String &text, int &year, int &month, int &day, int &dateStart){
  for(int i = 0; i <= (int)text.length() - 10; i++){
    if(isDigit(text[i]) && isDigit(text[i + 1]) && isDigit(text[i + 2]) && isDigit(text[i + 3]) &&
       text[i + 4] == '-' && isDigit(text[i + 5]) && isDigit(text[i + 6]) &&
       text[i + 7] == '-' && isDigit(text[i + 8]) && isDigit(text[i + 9])){
      year = text.substring(i, i + 4).toInt();
      month = text.substring(i + 5, i + 7).toInt();
      day = text.substring(i + 8, i + 10).toInt();
      dateStart = i;
      return year >= 2020 && year <= 2099 && month >= 1 && month <= 12 && day >= 1 && day <= 31;
    }
  }
  return false;
}

static int parseFirstNumber(const String &text){
  for(int i = 0; i < (int)text.length(); i++){
    if(isDigit(text[i])){
      int end = i + 1;
      while(end < (int)text.length() && isDigit(text[end])){
        end++;
      }
      return text.substring(i, end).toInt();
    }
  }
  return -1;
}

static int timeEndIndexInText(const String &text){
  int colon = text.indexOf(':');
  if(colon < 1){
    return -1;
  }
  int end = colon + 1;
  while(end < (int)text.length() && isDigit(text[end])){
    end++;
  }
  return end;
}

static bool dateFromOffsetDays(int offsetDays, int &year, int &month, int &day){
  if(!clockReady()){
    return false;
  }
  time_t raw = (time_t)currentEpoch() + (time_t)offsetDays * 86400;
  tm *info = gmtime(&raw);
  if(info == nullptr){
    return false;
  }
  year = info->tm_year + 1900;
  month = info->tm_mon + 1;
  day = info->tm_mday;
  return true;
}

static void saveCommandEvent(int year, int month, int day, int hour, int minute, String text){
  text.trim();
  if(text.length() == 0){
    text = "Event";
  }
  if(text.length() > 60){
    text = text.substring(0, 60);
  }
  int slot = 0;
  eventEnabledList[slot] = true;
  eventYearList[slot] = year;
  eventMonthList[slot] = month;
  eventDayList[slot] = day;
  eventHourList[slot] = hour;
  eventMinuteList[slot] = minute;
  eventTextList[slot] = text;
  setEventPrefs();
  sensorStateChanged = true;
  lastCommandResult = "Event 1 saved: " + eventDateText(slot) + " " + eventTextList[slot];
}

static String eventInputDate(int index){
  if(index < 0 || index >= EVENT_SLOT_COUNT || eventYearList[index] <= 0){
    return "";
  }
  return String(eventYearList[index]) + "-" +
         (eventMonthList[index] < 10 ? "0" : "") + String(eventMonthList[index]) + "-" +
         (eventDayList[index] < 10 ? "0" : "") + String(eventDayList[index]);
}

static String eventInputTime(int index){
  if(index < 0 || index >= EVENT_SLOT_COUNT){
    return "08:00";
  }
  return (eventHourList[index] < 10 ? "0" : "") + String(eventHourList[index]) + ":" +
         (eventMinuteList[index] < 10 ? "0" : "") + String(eventMinuteList[index]);
}

void handleControl(){
  String alarmHH = alarmHour < 10 ? "0" + String(alarmHour) : String(alarmHour);
  String alarmMM = alarmMinute < 10 ? "0" + String(alarmMinute) : String(alarmMinute);
  String alarmOptions = "";
  const int alarmTrackValues[] = {1, 4, 5};
  const char *alarmTrackLabels[] = {"Music 1 Haiyu Ni", "Music 4 Tianfu", "Music 5 Ai"};
  for(int i = 0; i < 3; i++){
    alarmOptions += "<option value='" + String(alarmTrackValues[i]) + "'";
    if(alarmTrack == alarmTrackValues[i]){
      alarmOptions += " selected";
    }
    alarmOptions += ">" + String(alarmTrackLabels[i]) + "</option>";
  }
  String voiceOptions = "";
  for(int i = 1; i <= 5; i++){
    String label = "Track " + String(i);
    if(i == 1){
      label += " Haiyu Ni";
    }else if(i == 2){
      label += " theft";
    }else if(i == 3){
      label += " smoke/fire";
    }else if(i == 4){
      label += " Tianfu";
    }else{
      label += " Ai";
    }
    voiceOptions += "<option value='" + String(i) + "'>" + label + "</option>";
  }
  String eventsHtml = "";
  for(int i = 0; i < EVENT_SLOT_COUNT; i++){
    eventsHtml += "<form action='/setevent' method='post'><input type='hidden' name='eventSlot' value='" + String(i) + "'>";
    eventsHtml += "<h3><span class='dot'></span>Plan " + String(i + 1) + "</h3>";
    eventsHtml += "<label class='check'><input type='checkbox' name='eventEnabled'" + checkedAttr(eventEnabledList[i]) + ">Show this plan</label>";
    eventsHtml += "<label>Date</label><input type='date' name='eventDate' value='" + eventInputDate(i) + "'>";
    eventsHtml += "<label>Time</label><input type='time' name='eventTime' value='" + eventInputTime(i) + "'>";
    eventsHtml += "<label>Note</label><textarea name='eventText' maxlength='60'>" + htmlEscape(eventTextList[i]) + "</textarea>";
    eventsHtml += "<button>Save Plan " + String(i + 1) + "</button></form>";
  }

  String page = "<!DOCTYPE html><html lang='zh'><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  page += "<link href='https://cdn.jsdelivr.net/npm/qweather-icons@1.8.0/font/qweather-icons.css' rel='stylesheet'>";
  page += "<title>Smart Calendar</title><style>:root{--bg:#fff7fb;--panel:#fff;--ink:#213047;--muted:#738096;--line:#eadfea;--blue:#6b8cff;--mint:#35b6a3;--green:#42a66b;--red:#e94f64;--amber:#eaa33a;--pink:#ff8fb3;--soft:#fffafd}*{box-sizing:border-box}html{scroll-behavior:smooth}body{margin:0;background:linear-gradient(180deg,#fff1f7 0,#eef9ff 250px,#fffdf8 100%);color:var(--ink);font-family:Arial,'Microsoft YaHei',sans-serif}.wrap{max-width:780px;margin:0 auto;padding:12px 14px 92px}.topbar{display:flex;justify-content:space-between;align-items:center;margin:4px 0 10px;color:#667085;font-size:13px}.hero{background:linear-gradient(135deg,#7bd9c6,#89a8ff 58%,#ff9fbd);color:white;border-radius:8px;padding:18px 16px;margin-bottom:12px;box-shadow:0 14px 30px rgba(130,150,255,.22)}.heroHead{display:flex;justify-content:space-between;gap:14px;align-items:flex-start}.hero h1{font-size:29px;margin:0 0 5px;letter-spacing:0}.hero .muted{color:rgba(255,255,255,.88)}.heroTime{font-size:34px;font-weight:900;line-height:1}.chips{display:flex;gap:7px;flex-wrap:wrap;margin-top:14px}.chip{border:1px solid rgba(255,255,255,.40);background:rgba(255,255,255,.22);border-radius:999px;padding:6px 9px;font-size:12px;color:white}.quick{display:grid;grid-template-columns:repeat(4,1fr);gap:8px;margin:8px 0 12px}.tile{background:white;border:1px solid var(--line);border-radius:8px;padding:11px 8px;box-shadow:0 6px 16px rgba(102,112,133,.07);border-top:4px solid var(--blue)}.tile:nth-child(2){border-top-color:var(--mint)}.tile:nth-child(3){border-top-color:var(--amber)}.tile:nth-child(4){border-top-color:var(--pink)}.tile span{display:block;color:var(--muted);font-size:12px}.tile b{display:block;font-size:20px;margin-top:4px}.mini{float:right;background:#f8f1ff;color:#7c3aed;border-radius:999px;padding:2px 6px;font-size:11px;font-weight:900}.grid{display:grid;grid-template-columns:1fr 1fr;gap:12px}.card{background:rgba(255,255,255,.96);border:1px solid var(--line);border-radius:8px;padding:15px;margin:12px 0;box-shadow:0 8px 20px rgba(102,112,133,.07)}.card h2{font-size:17px;margin:0 0 12px}.sectionTitle{display:flex;align-items:center;justify-content:space-between;gap:8px}.badge{font-size:12px;border-radius:999px;padding:5px 8px;background:#fff0f6;color:#be4166}.value{font-size:28px;font-weight:900}.stat{font-size:14px;line-height:1.75}.bad{color:var(--red);font-weight:800}.ok{color:var(--green);font-weight:800}.wxTop{display:flex;align-items:center;justify-content:space-between;gap:12px}.wxIcon{font-size:58px;color:#f6a72f}.forecast{margin-top:9px;border-top:1px dashed var(--line);padding-top:8px;color:#536175}.actions{display:grid;grid-template-columns:repeat(3,1fr);gap:9px}.actionForm{margin:0}.actionBtn{margin:0;min-height:46px}.danger{background:var(--red)}.gray{background:#7a88a0}.green{background:var(--mint)}label{display:block;margin:10px 0 6px;color:#42526b;font-weight:800;font-size:14px}input,textarea,select,button{width:100%;font-size:16px;padding:11px;border:1px solid #d8dce8;border-radius:8px;background:#fff}textarea{min-height:74px}button{margin-top:14px;background:var(--blue);color:white;border:0;font-weight:900;box-shadow:0 5px 12px rgba(107,140,255,.18)}.check{display:flex;gap:8px;align-items:center}.check input{width:auto}.muted{color:var(--muted);font-size:14px;line-height:1.5}.result{background:#f3f8ff;border-left:4px solid var(--blue);padding:10px;margin-top:10px;border-radius:6px}.row{display:flex;gap:10px}.row form{flex:1}.eventGrid{display:grid;grid-template-columns:1fr 1fr;gap:10px}.eventBox{border:1px solid var(--line);background:var(--soft);border-radius:8px;padding:12px}.eventBox h3{margin:0 0 8px;font-size:15px}.dot{display:inline-block;width:8px;height:8px;border-radius:50%;background:var(--pink);margin-right:6px}.bottom{position:fixed;left:0;right:0;bottom:0;background:rgba(255,255,255,.96);border-top:1px solid var(--line);padding:10px 14px;z-index:8}.bottomInner{max-width:780px;margin:0 auto;display:grid;grid-template-columns:1fr 1fr;gap:10px}.bottom button{margin:0}.alertFull{display:none;position:fixed;inset:0;z-index:20;background:var(--red);color:#fff;align-items:center;justify-content:center;padding:20px}.alertBox{max-width:360px;text-align:center}.alertIcon{width:86px;height:86px;border:5px solid #fff;border-radius:50%;display:flex;align-items:center;justify-content:center;font-size:56px;font-weight:900;margin:0 auto 18px}.alertBox h1{font-size:34px;margin:0 0 10px}.alertBox p{font-size:17px;line-height:1.5}.alertBox button{background:#fff;color:var(--red);font-weight:900}@media(max-width:560px){.wrap{padding-left:12px;padding-right:12px}.heroHead{display:block}.heroTime{font-size:31px;margin-top:10px}.quick{grid-template-columns:repeat(2,1fr)}.grid,.eventGrid{grid-template-columns:1fr}.actions{grid-template-columns:1fr 1fr}.row{display:block}.bottomInner{grid-template-columns:1fr 1fr}}</style></head><body><div id='alert' class='alertFull'><div class='alertBox'><div class='alertIcon'>!</div><h1>FIRE ALARM</h1><p>Smoke or fire detected. Check the device now.</p><form action='/smoketest' method='post'><input type='hidden' name='mode' value='off'><button>Clear Demo Alarm</button></form></div></div><div class='wrap'>";
  page += "<div class='topbar'><span>Live little panel</span><span>" + WiFi.localIP().toString() + "</span></div>";
  page += "<div class='hero'><div class='heroHead'><div><h1>Music Calendar</h1><div class='muted'>A tiny clock for weather, music and reminders</div></div><div class='heroTime'>" + currentFormattedTime().substring(0, 5) + "</div></div><div class='chips'><span class='chip'>" + String(wifiConnected() ? "WiFi ready" : "WiFi off") + "</span><span class='chip'>" + String(alarmEnabled ? "Wake-up on" : "Wake-up off") + "</span><span class='chip'>" + String(antiTheftMode ? "Guard on" : "Guard off") + "</span><span class='chip'>" + String(fireEmailEnabled() ? "Mail ready" : "Mail off") + "</span></div></div>";
  page += "<div class='quick'><div class='tile'><span><span class='mini'>T</span>Room temp</span><b>" + temperature + "C</b></div><div class='tile'><span><span class='mini'>H</span>Humidity</span><b>" + humidity + "%</b></div><div class='tile'><span><span class='mini'>A</span>Air</span><b>" + String(weather.air) + "</b></div><div class='tile'><span><span class='mini'>L</span>Light</span><b>" + String((int)lightLux) + "</b></div></div>";
  page += "<div class='grid'><div class='card stat'><div class='sectionTitle'><h2>Cozy Room</h2><span class='badge'>" + String(fireAlarm || infraredDetected ? "Check me" : "All good") + "</span></div><div class='value'>" + temperature + " C</div><div>Humidity " + humidity + "%</div><div>Light " + String((int)lightLux) + " lux</div><div class='" + String(fireAlarm ? "bad" : "ok") + "'>Smoke " + String(fireAlarm ? "ALARM" : "normal") + "</div><div class='" + String(infraredDetected ? "bad" : "ok") + "'>Guard " + String(infraredDetected ? "detected" : "quiet") + "</div></div>";
  String forecastHtml = "";
  if(weather.forecastReady){
    for(int i = 0; i < weather.forecastCount; i++){
      forecastHtml += "<div>" + htmlEscape(weather.forecast[i].date.substring(5)) + " " + htmlEscape(weather.forecast[i].text) + " " + String(weather.forecast[i].tempMin) + "-" + String(weather.forecast[i].tempMax) + " C</div>";
    }
  }else{
    forecastHtml = "<div>Forecast waiting</div>";
  }
  page += "<div class='card stat'><div class='wxTop'><div><div class='sectionTitle'><h2>Sky Today</h2><span class='badge'>" + String(wifiConnected() ? "Fresh" : "Offline") + "</span></div><div class='value'>" + htmlEscape(city) + "</div><div>" + htmlEscape(weather.text) + " / " + String(weather.temp) + " C</div></div><i class='qi-" + String(weather.icon) + " wxIcon'></i></div><div>AQI " + String(weather.air) + " / PM2.5 " + htmlEscape(weather.pm2p5) + "</div><div>Wind " + htmlEscape(weather.win) + "</div><div class='forecast'>" + forecastHtml + "</div></div></div>";
  page += "<div class='card'><div class='sectionTitle'><h2>Little Controls</h2><span class='badge'>Show time</span></div><div class='actions'><form class='actionForm' action='/smoketest' method='post'><input type='hidden' name='mode' value='on'><button class='actionBtn danger'>Fire Demo</button></form><form class='actionForm' action='/stopalarm' method='post'><button class='actionBtn gray'>Stop Sound</button></form><form class='actionForm' action='/refreshweather' method='post'><button class='actionBtn green'>New Sky</button></form></div></div>";
  page += "<div class='grid'><div class='card'><h2>Wake-up Music</h2><form action='/setalarm' method='post'><label class='check'><input type='checkbox' name='alarmEnabled'" + checkedAttr(alarmEnabled) + ">Use this alarm</label><label>Time</label><input type='time' name='alarmTime' value='" + alarmHH + ":" + alarmMM + "' required><label>Song</label><select name='alarmTrack'>" + alarmOptions + "</select><button>Save wake-up</button></form><div class='row'><form action='/stopalarm' method='post'><button class='danger'>Stop</button></form><form action='/playtrack' method='post'><input type='hidden' name='track' value='" + String(alarmTrack) + "'><button class='gray'>Try song</button></form></div></div>";
  page += "<div class='card'><h2>Guard and Voice</h2><form action='/setsecurity' method='post'><label class='check'><input type='checkbox' name='antiTheftMode'" + checkedAttr(antiTheftMode) + ">Guard mode</label><button>Save guard</button></form><form action='/setsound' method='post'><label class='check'><input type='checkbox' name='voice'" + checkedAttr(voice) + ">Voice module</label><button>Save voice</button></form></div></div>";
  page += "<div class='card'><div class='sectionTitle'><h2>Say It Simply</h2><span class='badge'>Text command</span></div><form action='/command' method='post'><label>Type a command</label><input name='cmd' maxlength='80' placeholder='event tomorrow 8:00 report'><button>Do it</button></form><div class='result'>" + htmlEscape(lastCommandResult) + "</div><p class='muted'>Try: alarm 7:30, music 1, guard on, fire demo, stop all, event tomorrow 8:00 report.</p></div>";
  page += "<div class='card'><div class='sectionTitle'><h2>Tiny Plans</h2><span class='badge'>3 notes</span></div><div class='eventGrid'>" + eventsHtml + "</div><p class='muted'>One day before a plan, the screen shows a small subtitle. Today plans pop up full screen. Current subtitle: " + htmlEscape(eventReminderText()) + "</p></div>";
  page += "<div class='grid'><div class='card'><h2>Time Tune</h2><form action='/setclock' method='post'><label>Date</label><input type='date' name='date' required><label>Time</label><input type='time' name='time' step='1' required><button>Set clock</button></form></div>";
  page += "<div class='card'><h2>Music Box</h2><form action='/playtrack' method='post'><label>Choose track</label><select name='track'>" + voiceOptions + "</select><button>Play track</button></form><p class='muted'>Tracks: 1 Haiyu Ni, 2 guard, 3 smoke/fire, 4 Tianfu, 5 Ai.</p><div>Mail alert " + String(fireEmailEnabled() ? "ready" : "off") + "</div><div>Status " + htmlEscape(fireEmailStatusText()) + "</div><p class='muted'><a href='/status'>JSON status</a></p></div></div>";
  page += "<div class='bottom'><div class='bottomInner'><form action='/smoketest' method='post'><input type='hidden' name='mode' value='on'><button class='danger'>Fire Demo</button></form><form action='/stopalarm' method='post'><button class='gray'>Stop Alarm</button></form></div></div>";
  page += "<script>let lastFire=false;async function poll(){try{const r=await fetch('/status',{cache:'no-store'});const s=await r.json();const a=document.getElementById('alert');const active=s.fireAlertActive||s.theftAlertActive;if(active){a.style.display='flex';a.querySelector('h1').textContent=s.fireAlertActive?'FIRE ALARM':'SECURITY ALARM';a.querySelector('p').textContent=s.fireAlertActive?'Smoke or fire detected. Check the device now.':'IR detected in anti-theft mode.';if(!lastFire){if(navigator.vibrate)navigator.vibrate([300,150,300,150,700]);if('Notification'in window){if(Notification.permission==='granted')new Notification('Smart Calendar alarm',{body:s.fireAlertActive?'Smoke/fire alarm detected':'Security alarm detected'});else if(Notification.permission!=='denied')Notification.requestPermission();}}}else{a.style.display='none';}lastFire=active;}catch(e){}}setInterval(poll,1200);poll();</script>";
  page += "</div></body></html>";
  server.send(200, "text/html", page);
}

void handleSetClock(){
  if(!server.hasArg("date") || !server.hasArg("time")){
    server.send(400, "text/plain; charset=UTF-8", "Missing date or time");
    return;
  }
  String date = server.arg("date");
  String time = server.arg("time");
  if(date.length() < 10 || time.length() < 5){
    server.send(400, "text/plain; charset=UTF-8", "Invalid time");
    return;
  }
  int year = date.substring(0, 4).toInt();
  int month = date.substring(5, 7).toInt();
  int day = date.substring(8, 10).toInt();
  int hour = time.substring(0, 2).toInt();
  int minute = time.substring(3, 5).toInt();
  int second = time.length() >= 8 ? time.substring(6, 8).toInt() : 0;
  if(!setClockDateTime(year, month, day, hour, minute, second)){
    server.send(400, "text/plain; charset=UTF-8", "Invalid time");
    return;
  }
  sensorStateChanged = true;
  server.sendHeader("Location", "/control");
  server.send(303);
}

void handleSetAlarm(){
  alarmEnabled = server.hasArg("alarmEnabled");
  if(server.hasArg("alarmTime")){
    String value = server.arg("alarmTime");
    if(value.length() < 5){
      server.send(400, "text/plain; charset=UTF-8", "Invalid alarm time");
      return;
    }
    alarmHour = value.substring(0, 2).toInt();
    alarmMinute = value.substring(3, 5).toInt();
  }
  if(server.hasArg("alarmTrack")){
    alarmTrack = normalizeAlarmTrack(server.arg("alarmTrack").toInt());
  }
  alarmRinging = false;
  skipTodayIfAlarmMatchesNow();
  updateWarningLight();
  setAlarmPrefs();
  server.sendHeader("Location", "/control");
  server.send(303);
}

void handleSetSecurity(){
  antiTheftMode = server.hasArg("antiTheftMode");
  sensorStateChanged = true;
  updateWarningLight();
  setAlarmPrefs();
  server.sendHeader("Location", "/control");
  server.send(303);
}

void handleSetSound(){
  voice = server.hasArg("voice");
  setVoice();
  server.sendHeader("Location", "/control");
  server.send(303);
}

void handleStopAlarm(){
  stopAllAlerts();
  server.sendHeader("Location", "/control");
  server.send(303);
}

void handlePlayTrack(){
  if(server.hasArg("track")){
    playVoiceTrack(constrain(server.arg("track").toInt(), 1, 5));
  }
  server.sendHeader("Location", "/control");
  server.send(303);
}

void handleCommand(){
  if(!server.hasArg("cmd")){
    lastCommandResult = "No command";
    server.sendHeader("Location", "/control");
    server.send(303);
    return;
  }

  String cmd = server.arg("cmd");
  cmd.trim();
  String lower = cmd;
  lower.toLowerCase();
  lastCommandResult = "Unknown command: " + cmd;

  if(lower == "alarm on" || lower == "enable alarm"){
    alarmEnabled = true;
    skipTodayIfAlarmMatchesNow();
    setAlarmPrefs();
    sensorStateChanged = true;
    lastCommandResult = "Alarm enabled";
  }else if(lower == "alarm off" || lower == "disable alarm"){
    alarmEnabled = false;
    alarmRinging = false;
    updateWarningLight();
    setAlarmPrefs();
    sensorStateChanged = true;
    lastCommandResult = "Alarm disabled";
  }else if(lower.startsWith("set alarm") || lower.startsWith("alarm ")){
    int h = 0;
    int m = 0;
    if(parseTimeInText(lower, h, m)){
      alarmHour = h;
      alarmMinute = m;
      alarmEnabled = true;
      alarmRinging = false;
      skipTodayIfAlarmMatchesNow();
      updateWarningLight();
      setAlarmPrefs();
      sensorStateChanged = true;
      lastCommandResult = "Alarm set to " + String(h < 10 ? "0" : "") + String(h) + ":" + String(m < 10 ? "0" : "") + String(m);
    }else{
      lastCommandResult = "Alarm command needs time, example: set alarm 07:30";
    }
  }else if(lower == "security on" || lower == "anti theft on" || lower == "antitheft on" || lower == "guard on"){
    antiTheftMode = true;
    sensorStateChanged = true;
    updateWarningLight();
    setAlarmPrefs();
    lastCommandResult = "Security enabled";
  }else if(lower == "security off" || lower == "anti theft off" || lower == "antitheft off" || lower == "guard off"){
    antiTheftMode = false;
    sensorStateChanged = true;
    updateWarningLight();
    setAlarmPrefs();
    lastCommandResult = "Security disabled";
  }else if(lower == "sound on" || lower == "voice on"){
    voice = true;
    setVoice();
    lastCommandResult = "Sound enabled";
  }else if(lower == "sound off" || lower == "voice off"){
    voice = false;
    alarmRinging = false;
    jqStop();
    updateWarningLight();
    setVoice();
    lastCommandResult = "Sound disabled";
  }else if(lower.startsWith("play music") || lower.startsWith("play track") || lower.startsWith("music ") || lower.startsWith("track ")){
    int parsedTrack = parseFirstNumber(lower);
    if(parsedTrack < 1 || parsedTrack > 5){
      lastCommandResult = "Play command needs track 1-5, example: play music 2";
    }else{
      int track = constrain(parsedTrack, 1, 5);
      playVoiceTrack(track);
      lastCommandResult = "Playing track " + String(track);
    }
  }else if(lower == "refresh weather" || lower == "update weather"){
    updateWeather = true;
    lastCommandResult = "Weather refresh started";
  }else if(lower == "smoke test" || lower == "fire test" || lower == "fire demo" || lower == "simulate smoke" || lower == "simulate fire"){
    setSimulatedFireAlarm(true, true);
    lastCommandResult = "Simulated smoke/fire alarm started";
  }else if(lower == "clear smoke" || lower == "clear fire" || lower == "stop smoke test" || lower == "clear smoke test"){
    setSimulatedFireAlarm(false);
    lastCommandResult = "Simulated smoke/fire alarm cleared";
  }else if(lower == "stop alarm" || lower == "stop" || lower == "stop all" || lower == "clear alarm" || lower == "clear alert"){
    stopAllAlerts();
    lastCommandResult = "Alerts stopped";
  }else if(lower.startsWith("event tomorrow ") || lower.startsWith("event today ")){
    int y = 0;
    int mo = 0;
    int d = 0;
    int offsetDays = lower.startsWith("event tomorrow ") ? 1 : 0;
    if(!dateFromOffsetDays(offsetDays, y, mo, d)){
      lastCommandResult = "Clock not ready for today/tomorrow event";
    }else{
      int eventH = 8;
      int eventM = 0;
      if(!parseTimeInText(lower, eventH, eventM)){
        lastCommandResult = "Event command needs time, example: event tomorrow 8:00 report";
      }else{
        int textStart = timeEndIndexInText(lower);
        String eventNote = textStart >= 0 ? cmd.substring(textStart) : "";
        saveCommandEvent(y, mo, d, eventH, eventM, eventNote);
      }
    }
  }else if(lower.startsWith("event ")){
    int y = 0;
    int mo = 0;
    int d = 0;
    int dateStart = -1;
    if(parseDateInText(lower, y, mo, d, dateStart)){
      int eventH = 8;
      int eventM = 0;
      parseTimeInText(lower, eventH, eventM);
      int textStart = dateStart + 10;
      while(textStart < (int)cmd.length() && (cmd[textStart] == ' ' || cmd[textStart] == '-' || cmd[textStart] == '/')){
        textStart++;
      }
      if(textStart + 5 <= (int)cmd.length() && cmd[textStart + 2] == ':'){
        textStart += 5;
      }
      while(textStart < (int)cmd.length() && cmd[textStart] == ' '){
        textStart++;
      }
      saveCommandEvent(y, mo, d, eventH, eventM, cmd.substring(textStart));
    }else{
      lastCommandResult = "Event command needs date, example: event 2026-07-10 submit report";
    }
  }else if(lower == "clear event"){
    for(int i = 0; i < EVENT_SLOT_COUNT; i++){
      eventEnabledList[i] = false;
      eventTextList[i] = "";
    }
    setEventPrefs();
    sensorStateChanged = true;
    lastCommandResult = "Event cleared";
  }

  server.sendHeader("Location", "/control");
  server.send(303);
}

void handleRefreshWeather(){
  updateWeather = true;
  server.sendHeader("Location", "/control");
  server.send(303);
}

void handleSmokeTest(){
  String modeArg = server.hasArg("mode") ? server.arg("mode") : "on";
  modeArg.toLowerCase();
  bool enable = modeArg != "off" && modeArg != "clear" && modeArg != "0";
  setSimulatedFireAlarm(enable, enable);
  server.sendHeader("Location", "/control");
  server.send(303);
}

void handleSetEvent(){
  int slot = server.hasArg("eventSlot") ? server.arg("eventSlot").toInt() : 0;
  slot = constrain(slot, 0, EVENT_SLOT_COUNT - 1);
  eventEnabledList[slot] = server.hasArg("eventEnabled");
  if(server.hasArg("eventDate")){
    String date = server.arg("eventDate");
    if(date.length() >= 10){
      eventYearList[slot] = date.substring(0, 4).toInt();
      eventMonthList[slot] = date.substring(5, 7).toInt();
      eventDayList[slot] = date.substring(8, 10).toInt();
    }else if(eventEnabledList[slot]){
      server.send(400, "text/plain; charset=UTF-8", "Invalid event date");
      return;
    }
  }
  if(server.hasArg("eventTime")){
    String time = server.arg("eventTime");
    if(time.length() >= 5){
      eventHourList[slot] = time.substring(0, 2).toInt();
      eventMinuteList[slot] = time.substring(3, 5).toInt();
      eventHourList[slot] = constrain(eventHourList[slot], 0, 23);
      eventMinuteList[slot] = constrain(eventMinuteList[slot], 0, 59);
    }
  }
  if(server.hasArg("eventText")){
    eventTextList[slot] = server.arg("eventText");
    eventTextList[slot].trim();
    if(eventTextList[slot].length() > 60){
      eventTextList[slot] = eventTextList[slot].substring(0, 60);
    }
  }
  if(eventEnabledList[slot] && eventTextList[slot].length() == 0){
    eventTextList[slot] = "Event";
  }
  setEventPrefs();
  sensorStateChanged = true;
  server.sendHeader("Location", "/control");
  server.send(303);
}

void handleStatus(){
  String json = "{";
  json += "\"time\":\"" + currentFormattedTime() + "\",";
  json += "\"city\":\"" + jsonEscape(city) + "\",";
  json += "\"weather\":\"" + jsonEscape(weather.text) + "\",";
  json += "\"outdoorTemp\":" + String(weather.temp) + ",";
  json += "\"aqi\":" + String(weather.air) + ",";
  json += "\"pm2p5\":\"" + jsonEscape(weather.pm2p5) + "\",";
  json += "\"temperature\":\"" + temperature + "\",";
  json += "\"humidity\":\"" + humidity + "\",";
  json += "\"lightLux\":" + String(lightLux, 1) + ",";
  json += "\"fireAlarm\":" + String(fireAlarm ? "true" : "false") + ",";
  json += "\"simulatedFireAlarm\":" + String(simulatedFireAlarm ? "true" : "false") + ",";
  json += "\"fireAlertActive\":" + String(fireAlertActive() ? "true" : "false") + ",";
  json += "\"infraredDetected\":" + String(infraredDetected ? "true" : "false") + ",";
  json += "\"antiTheftMode\":" + String(antiTheftMode ? "true" : "false") + ",";
  json += "\"theftAlertActive\":" + String(theftAlertActive() ? "true" : "false") + ",";
  json += "\"alarmEnabled\":" + String(alarmEnabled ? "true" : "false") + ",";
  json += "\"alarmTime\":\"" + String(alarmHour < 10 ? "0" : "") + String(alarmHour) + ":" + String(alarmMinute < 10 ? "0" : "") + String(alarmMinute) + "\",";
  json += "\"alarmTrack\":" + String(alarmTrack) + ",";
  json += "\"alarmRinging\":" + String(alarmRinging ? "true" : "false") + ",";
  json += "\"voice\":" + String(voice ? "true" : "false") + ",";
  json += "\"fireEmailEnabled\":" + String(fireEmailEnabled() ? "true" : "false") + ",";
  json += "\"fireEmailPending\":" + String(fireEmailPending() ? "true" : "false") + ",";
  json += "\"fireEmailSending\":" + String(fireEmailSending() ? "true" : "false") + ",";
  json += "\"lastFireEmailOk\":" + String(lastFireEmailOk() ? "true" : "false") + ",";
  json += "\"fireEmailStatus\":\"" + jsonEscape(fireEmailStatusText()) + "\",";
  json += "\"forecastReady\":" + String(weather.forecastReady ? "true" : "false") + ",";
  json += "\"eventReminder\":\"" + jsonEscape(eventReminderText()) + "\",";
  json += "\"events\":[";
  for(int i = 0; i < EVENT_SLOT_COUNT; i++){
    if(i > 0){
      json += ",";
    }
    json += "{";
    json += "\"enabled\":" + String(eventEnabledList[i] ? "true" : "false") + ",";
    json += "\"date\":\"" + jsonEscape(eventDateText(i)) + "\",";
    json += "\"time\":\"" + String(eventHourList[i] < 10 ? "0" : "") + String(eventHourList[i]) + ":" + String(eventMinuteList[i] < 10 ? "0" : "") + String(eventMinuteList[i]) + "\",";
    json += "\"text\":\"" + jsonEscape(eventTextList[i]) + "\"";
    json += "}";
  }
  json += "]";
  json += "}";
  server.send(200, "application/json; charset=UTF-8", json);
}
// 鎻愪氦鏁版嵁鍚庣殑鎻愮ず椤甸潰
void handleConfigWifi(){
  //鍒ゆ柇鏄惁鏈塛iFi鍚嶇О
  if (server.hasArg("ssid")){
    logInfo("鑾峰緱WiFi鍚嶇О:");
    ssid = server.arg("ssid");
    logInfoln(ssid);
  }else{
    logInfoln("閿欒, 娌℃湁鍙戠幇WiFi鍚嶇О");
    server.send(200, "text/html", "<meta charset='UTF-8'>閿欒, 娌℃湁鍙戠幇WiFi鍚嶇О");
    return;
  }
  //鍒ゆ柇鏄惁鏈塛iFi瀵嗙爜
  if (server.hasArg("pass")){
    logInfo("WiFi password received: ");
    pass = server.arg("pass");
    logInfoln("******");
  }else{
    logInfoln("閿欒, 娌℃湁鍙戠幇WiFi瀵嗙爜");
    server.send(200, "text/html", "<meta charset='UTF-8'>閿欒, 娌℃湁鍙戠幇WiFi瀵嗙爜");
    return;
  }
  //鍒ゆ柇鏄惁鏈塩ity鍚嶇О
  if (server.hasArg("city")){
    if(!server.arg("city").equals(city) ){
      location = "";
    }
    logInfo("鑾峰緱鍩庡競:");
    city = server.arg("city");
    logInfoln(city);
  }else{
    logInfoln("閿欒, 娌℃湁鍙戠幇鍩庡競鍚嶇О");
    server.send(200, "text/html", "<meta charset='UTF-8'>閿欒, 娌℃湁鍙戠幇鍩庡競鍚嶇О");
    return;
  }
  logInfo("鑾峰緱涓婄骇鍖哄垝:");
  adm = server.arg("adm");
  logInfoln(adm);
  // 灏嗕俊鎭瓨鍏vs涓?
  setInfo();
  // 鑾峰緱浜嗘墍闇€瑕佺殑涓€鍒囦俊鎭紝缁欏鎴风鍥炲
  server.send(200, "text/html", "<meta charset='UTF-8'><style type='text/css'>body {font-size: 2rem;}</style><br/><br/>WiFi: " + htmlEscape(ssid) + "<br/>Password: ******<br/>City: " + htmlEscape(city) + "<br/>Region: " + htmlEscape(adm) + "<br/>Saved. Device will restart and connect to WiFi. You can close this page.");
  // 缁樺埗閲嶅惎鎻愮ず鏂囧瓧
  draw2LineText("Config saved", "Restarting");
  delay(1500);
  ESP.restart();
}
// 鍚姩鏈嶅姟鍣?
void startServer(){
  // 褰撴祻瑙堝櫒璇锋眰鏈嶅姟鍣ㄦ牴鐩綍(缃戠珯棣栭〉)鏃惰皟鐢ㄨ嚜瀹氫箟鍑芥暟handleRoot澶勭悊锛岃缃富椤靛洖璋冨嚱鏁帮紝蹇呴』娣诲姞绗簩涓弬鏁癏TTP_GET锛屽惁鍒欐棤娉曞己鍒堕棬鎴?
  server.on("/", HTTP_GET, handleRoot);
  // 褰撴祻瑙堝櫒璇锋眰鏈嶅姟鍣?configwifi(琛ㄥ崟瀛楁)鐩綍鏃惰皟鐢ㄨ嚜瀹氫箟鍑芥暟handleConfigWifi澶勭悊
  server.on("/configwifi", HTTP_POST, handleConfigWifi);
  server.on("/control", HTTP_GET, handleControl);
  server.on("/setclock", HTTP_POST, handleSetClock);
  server.on("/setalarm", HTTP_POST, handleSetAlarm);
  server.on("/setsecurity", HTTP_POST, handleSetSecurity);
  server.on("/setsound", HTTP_POST, handleSetSound);
  server.on("/setevent", HTTP_POST, handleSetEvent);
  server.on("/stopalarm", HTTP_POST, handleStopAlarm);
  server.on("/playtrack", HTTP_POST, handlePlayTrack);
  server.on("/command", HTTP_POST, handleCommand);
  server.on("/refreshweather", HTTP_POST, handleRefreshWeather);
  server.on("/smoketest", HTTP_POST, handleSmokeTest);
  server.on("/status", HTTP_GET, handleStatus);
  // 褰撴祻瑙堝櫒璇锋眰鐨勭綉缁滆祫婧愭棤娉曞湪鏈嶅姟鍣ㄦ壘鍒版椂璋冪敤鑷畾涔夊嚱鏁癶andleNotFound澶勭悊   
  server.onNotFound(handleNotFound);
  server.begin();
  logInfoln("鏈嶅姟鍣ㄥ惎鍔ㄦ垚鍔燂紒");
}
// 澶勭悊鏈嶅姟鍣ㄨ姹?
void doClient(){
  server.handleClient();
}
// 杩炴帴WiFi
void connectWiFi(int timeOut_s){
  connected = false;
  logInfoln("姝ｅ湪杩炴帴缃戠粶 ");
  logInfo("ssid: ");logInfoln(ssid);
  logInfo("pass: ");logInfoln("******");
  logInfo("WiFi timeout: ");
  logInfo(String(timeOut_s));
  logInfoln("s");
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  WiFi.begin(ssid, pass);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED){
    logInfo(".");
    delay(500);
    if ((millis() - start) >= (unsigned long)timeOut_s * 1000UL){
      logInfoln("缃戠粶杩炴帴澶辫触");
      connected = false;
      break;
    }
  }
  if(WiFi.status() == WL_CONNECTED){
    connected = true;
  }
  if(connected){
    logInfoln("缃戠粶杩炴帴鎴愬姛");
    logInfo("Device IP: ");
    logInfoln(WiFi.localIP().toString());
    logInfo("Control URL: http://");
    logInfo(WiFi.localIP().toString());
    logInfoln("/control");
  }else{
    // 鍏抽棴鍔犺浇鍔ㄧ敾
    loadingAnim = false;
    fadeOff();
    delay(200);
    // 缁樺埗setting椤甸潰
    drawSettingOrOffline(true, "杩炴帴澶辫触锛岄噸鏂伴厤缃紵");
    // 浣胯兘鎸夐敭
    buttonEnable = true;  
    // 鏂紑WIFI
    WiFi.disconnect();
    // 鍒涘缓灞忓箷娓愭樉浠诲姟
    createFadeOnTask();
  } 
}
// 妫€鏌iFi鐘舵€?
bool wifiConnected(){
  if(WiFi.status() == WL_CONNECTED){
    return true;
  }else{
    return false;
  }
}
// url涓枃缂栫爜
String urlEncode(const String& text){
  String encodedText = "";
  for (size_t i = 0; i < text.length(); i++) {
    char c = text[i];
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') {
      encodedText += c;
    } else if (c == ' ') {
      encodedText += '+';
    } else {
      encodedText += '%';
      char hex[4];
      sprintf(hex, "%02X", (uint8_t)c);
      encodedText += hex;
    }
  }
  return encodedText;
}
// 鏌ヨ鍩庡競id
int getCityID(){
  logInfoln("寮€濮嬭幏鍙栧煄甯俰d");
  // 璁＄畻jwt
  String jwt = generateJWT(PrivateKey, PublicKey, KeyID, ProjectID, timeClient.getEpochTime() - 8 * 3600);
  // Serial.println(jwt);
  data = "";
  bool flag = false; // 鏄惁鎴愬姛鑾峰彇鍒板煄甯俰d鐨勬爣蹇?
  String url = "https://" + ApiHost + cityURL + "?location=" + urlEncode(city) + "&adm=" + urlEncode(adm);
  // logInfoln(url);
  httpClient.setConnectTimeout(queryTimeout * 2);
  httpClient.begin(url);
  httpClient.addHeader("Authorization", "Bearer " + jwt);
  //鍚姩杩炴帴骞跺彂閫丠TTP璇锋眰
  int httpCode = httpClient.GET();
  // 澶勭悊鏈嶅姟鍣ㄧ瓟澶?
  if (httpCode == HTTP_CODE_OK) {
    // 瑙ｅ帇Gzip鏁版嵁娴?
    int len = httpClient.getSize();
    uint8_t buff[2048] = { 0 };
    WiFiClient *stream = httpClient.getStreamPtr();
    while (httpClient.connected() && (len > 0 || len == -1)) {
      size_t size = stream->available();  // 杩樺墿涓嬪灏戞暟鎹病鏈夎瀹岋紵
      if (size) {
        size_t realsize = ((size > sizeof(buff)) ? sizeof(buff) : size);
        size_t readBytesSize = stream->readBytes(buff, realsize);
        if (len > 0) len -= readBytesSize;
        appendDecompressedChunk(buff, readBytesSize, 5120);
      }
      delay(1);
    }
    // 瑙ｅ帇瀹岋紝杞崲json鏁版嵁
    StaticJsonDocument<2048> doc; //澹版槑涓€涓潤鎬丣sonDocument瀵硅薄
    DeserializationError error = deserializeJson(doc, data); //鍙嶅簭鍒楀寲JSON鏁版嵁
    if(!error){ //妫€鏌ュ弽搴忓垪鍖栨槸鍚︽垚鍔?
      //璇诲彇json鑺傜偣
      String code = doc["code"].as<const char*>();
      if(code.equals("200")){
        flag = true;
        // 澶氱粨鏋滅殑鎯呭喌涓嬶紝鍙栫涓€涓?
        city = doc["location"][0]["name"].as<const char*>();
        location = doc["location"][0]["id"].as<const char*>();
        lat = doc["location"][0]["lat"].as<const char*>();
        lon = doc["location"][0]["lon"].as<const char*>();
        logInfoln("鍩庡競id :" + location);
        // 灏嗕俊鎭瓨鍏vs涓?
        setInfo();
      }
    }  
  }
  //鍏抽棴涓庢湇鍔″櫒杩炴帴
  httpClient.end();
  if(!flag){
    logInfo("Get city id error: ");
    logInfoln(String(httpCode));
    if(httpCode == HTTP_CODE_OK){
      // 鍏抽棴鍔犺浇鍔ㄧ敾
      loadingAnim = false;
      fadeOff();
      delay(200);
      // 缁樺埗setting椤甸潰
      drawSettingOrOffline(true, "City ID failed");
      // 浣胯兘鎸夐敭
      buttonEnable = true;  
      // 鏂紑WIFI
      WiFi.disconnect();
      // 鍒涘缓灞忓箷娓愭樉浠诲姟
      createFadeOnTask();
    }
  }else{
    logInfoln("鑾峰彇鎴愬姛");
  }
  return httpCode;
}
// 鏌ヨ瀹炴椂澶╂皵
int getWeather(){
  Serial.println("姝ｅ湪鑾峰彇澶╂皵鏁版嵁");
  // 璁＄畻jwt
  String jwt = generateJWT(PrivateKey, PublicKey, KeyID, ProjectID, timeClient.getEpochTime() - 8 * 3600);
  data = "";
  queryWeatherSuccess = false; // 鍏堢疆涓篺alse
  String url = "https://" + ApiHost + nowURL + "?location=" + location + "&lang=en";
  httpClient.setConnectTimeout(queryTimeout);
  httpClient.begin(url);
  httpClient.addHeader("Authorization", "Bearer " + jwt);
  //鍚姩杩炴帴骞跺彂閫丠TTP璇锋眰
  int httpCode = httpClient.GET();
  // Serial.println(ESP.getFreeHeap());
  //濡傛灉鏈嶅姟鍣ㄥ搷搴擮K鍒欎粠鏈嶅姟鍣ㄨ幏鍙栧搷搴斾綋淇℃伅骞堕€氳繃涓插彛杈撳嚭
  if (httpCode == HTTP_CODE_OK) {
    // 瑙ｅ帇Gzip鏁版嵁娴?
    int len = httpClient.getSize();
    uint8_t buff[2048] = { 0 };
    WiFiClient *stream = httpClient.getStreamPtr();
    while (httpClient.connected() && (len > 0 || len == -1)) {
      size_t size = stream->available();  // 杩樺墿涓嬪灏戞暟鎹病鏈夎瀹岋紵
      // Serial.println(size);
      if (size) {
        size_t realsize = ((size > sizeof(buff)) ? sizeof(buff) : size);
        // Serial.println(realsize);
        size_t readBytesSize = stream->readBytes(buff, realsize);
        // Serial.write(buff,readBytesSize);
        if (len > 0) len -= readBytesSize;
        appendDecompressedChunk(buff, readBytesSize, 5120);
      }
      delay(1);
    }
    // 瑙ｅ帇瀹岋紝杞崲json鏁版嵁
    StaticJsonDocument<2048> doc; //澹版槑涓€涓潤鎬丣sonDocument瀵硅薄
    DeserializationError error = deserializeJson(doc, data); //鍙嶅簭鍒楀寲JSON鏁版嵁
    if(!error){ //妫€鏌ュ弽搴忓垪鍖栨槸鍚︽垚鍔?
      //璇诲彇json鑺傜偣
      String code = doc["code"].as<const char*>();
      if(code.equals("200")){
        queryWeatherSuccess = true;       
        //璇诲彇json鑺傜偣
        weather.icon = doc["now"]["icon"].as<int>();
        weather.text = getWea(weather.icon);
        weather.temp = doc["now"]["temp"].as<int>();
        String feelsLike = doc["now"]["feelsLike"]; // 浣撴劅娓╁害
        weather.feelsLike = "Feels " + feelsLike + " C";
        String windDir = doc["now"]["windDir"];
        String windScale = doc["now"]["windScale"];
        weather.win = windDir + " Lv" + windScale;
        weather.humidity = doc["now"]["humidity"].as<int>();
        String vis = doc["now"]["vis"];
        weather.vis = "VIS " + vis + " KM";
        Serial.println("鑾峰彇鎴愬姛");
      }
    }  
  }
  //鍏抽棴涓庢湇鍔″櫒杩炴帴
  httpClient.end();
  if(!queryWeatherSuccess){
    logInfo("Get weather error: ");
    logInfoln(String(httpCode));
  }else{
    getWeatherForecast();
  }
  return httpCode;
}

int getWeatherForecast(){
  logInfoln("Get 3-day weather forecast");
  String jwt = generateJWT(PrivateKey, PublicKey, KeyID, ProjectID, timeClient.getEpochTime() - 8 * 3600);
  data = "";
  weather.forecastReady = false;
  weather.forecastCount = 0;
  String url = "https://" + ApiHost + dailyURL + "?location=" + location + "&lang=en";
  httpClient.setConnectTimeout(queryTimeout);
  httpClient.begin(url);
  httpClient.addHeader("Authorization", "Bearer " + jwt);
  int httpCode = httpClient.GET();
  if(httpCode == HTTP_CODE_OK){
    int len = httpClient.getSize();
    uint8_t buff[2048] = { 0 };
    WiFiClient *stream = httpClient.getStreamPtr();
    while(httpClient.connected() && (len > 0 || len == -1)){
      size_t size = stream->available();
      if(size){
        size_t realsize = ((size > sizeof(buff)) ? sizeof(buff) : size);
        size_t readBytesSize = stream->readBytes(buff, realsize);
        if(len > 0){
          len -= readBytesSize;
        }
        appendDecompressedChunk(buff, readBytesSize, 8192);
      }
      delay(1);
    }

    StaticJsonDocument<4096> doc;
    DeserializationError error = deserializeJson(doc, data);
    if(!error){
      String code = doc["code"].as<const char*>();
      if(code.equals("200")){
        JsonArray daily = doc["daily"];
        int index = 0;
        for(JsonObject day : daily){
          if(index >= 3){
            break;
          }
          weather.forecast[index].date = day["fxDate"].as<const char*>();
          weather.forecast[index].icon = day["iconDay"].as<int>();
          weather.forecast[index].text = getWea(weather.forecast[index].icon);
          weather.forecast[index].tempMin = day["tempMin"].as<int>();
          weather.forecast[index].tempMax = day["tempMax"].as<int>();
          index++;
        }
        weather.forecastCount = index;
        weather.forecastReady = index > 0;
      }
    }
  }
  httpClient.end();
  if(!weather.forecastReady){
    logInfo("Get forecast error: ");
    logInfoln(String(httpCode));
  }
  return httpCode;
}
// 鏌ヨ绌烘皵璐ㄩ噺
int getAir(){
  Serial.println("姝ｅ湪鑾峰彇绌烘皵璐ㄩ噺鏁版嵁");
  // 璁＄畻jwt
  String jwt = generateJWT(PrivateKey, PublicKey, KeyID, ProjectID, timeClient.getEpochTime() - 8 * 3600);
  data = "";
  queryAirSuccess = false; // 鍏堢疆涓篺alse
  String url = "https://" + ApiHost + airURL + lat + "/" + lon;
  httpClient.setConnectTimeout(queryTimeout);
  httpClient.begin(url);
  httpClient.addHeader("Authorization", "Bearer " + jwt);
  //鍚姩杩炴帴骞跺彂閫丠TTP璇锋眰
  int httpCode = httpClient.GET();  
  //濡傛灉鏈嶅姟鍣ㄥ搷搴擮K鍒欎粠鏈嶅姟鍣ㄨ幏鍙栧搷搴斾綋淇℃伅骞堕€氳繃涓插彛杈撳嚭
  if (httpCode == HTTP_CODE_OK) {
    // 瑙ｅ帇Gzip鏁版嵁娴?
    int len = httpClient.getSize();
    uint8_t buff[2048] = { 0 };
    WiFiClient *stream = httpClient.getStreamPtr();
    while (httpClient.connected() && (len > 0 || len == -1)) {
      size_t size = stream->available();  // 杩樺墿涓嬪灏戞暟鎹病鏈夎瀹岋紵
      // Serial.println(size);
      if (size) {
        size_t realsize = ((size > sizeof(buff)) ? sizeof(buff) : size);
        // Serial.println(realsize);
        size_t readBytesSize = stream->readBytes(buff, realsize);
        // Serial.write(buff,readBytesSize);
        if (len > 0) len -= readBytesSize;
        appendDecompressedChunk(buff, readBytesSize, 20480);
      }
      delay(1);
    }
    // 瑙ｅ帇瀹岋紝杞崲json鏁版嵁
    StaticJsonDocument<2048> doc; //澹版槑涓€涓潤鎬丣sonDocument瀵硅薄
    DeserializationError error = deserializeJson(doc, data); //鍙嶅簭鍒楀寲JSON鏁版嵁
    if(!error){ //妫€鏌ュ弽搴忓垪鍖栨槸鍚︽垚鍔?
      //璇诲彇json鑺傜偣
      queryAirSuccess = true;
      weather.air = doc["indexes"][0]["aqi"].as<int>();
      // Serial.println(nowWeather.air);
      // 鑾峰彇 pollutants 鏁扮粍
      JsonArray pollutants = doc["pollutants"];
      // 閬嶅巻鏁扮粍涓殑姣忎釜瀵硅薄
      for (JsonObject pollutant : pollutants) {
        String code = pollutant["code"].as<const char*>(); // 鑾峰彇 code锛屾瘮濡俻m2p5,pm10,no2,so2,co,o3
        unsigned int value = pollutant["concentration"]["value"].as<int>(); // 鑾峰彇鍊?
        // Serial.print(code);
        // Serial.print(": ");
        // Serial.println(value);
        if(code.equals("pm2p5")){
          weather.pm2p5 = String(value);
        }else if(code.equals("pm10")){
          weather.pm10 = String(value);
        }else if(code.equals("no2")){
          weather.no2 = String(value);
        }else if(code.equals("so2")){
          weather.so2 = String(value);
        }else if(code.equals("co")){
          weather.co = String(value);
        }else if(code.equals("o3")){
          weather.o3 = String(value);
        }
      }
      Serial.println("鑾峰彇鎴愬姛");
    }  
  } 
  //鍏抽棴涓庢湇鍔″櫒杩炴帴
  httpClient.end();
  if(!queryAirSuccess){
    logInfo("Get air error: ");
    logInfoln(String(httpCode));
  }
  return httpCode;
}
// 鏂紑Wifi
void disconnectWiFi(){
  WiFi.disconnect();
}


