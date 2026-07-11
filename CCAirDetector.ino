#include "Common.h"
#include "Task.h"
#include "net.h"
#include "PreferencesUtil.h"
#include "tftUtil.h"
#include "MailAlert.h"

/**
CC娓╂箍搴︿华  鐗堟湰1.3

鏈鏇存柊鍐呭锛?

鍜岄澶╂皵韬唤楠岃瘉鏀圭増锛屽鑷存柊鐢宠鐨勫拰椋庤处鍙锋棤娉曢『鍒╄幏鍙栧ぉ姘斾俊鎭紝杩欎釜鐗堟湰鍔犲叆浜嗕綔鑰呰嚜宸辩紪鍐欑殑搴擄紝
浠嶪DE->椤圭洰->瀵煎叆搴?>娣诲姞.zip搴擄紝灏嗕綔鑰呭紑婧愮殑JwtUtil搴撳畨瑁咃紝鐒跺悗鍐峮et.cpp鏂囦欢涓紝鏇挎崲5涓拰椋庣浉鍏崇殑鍐呭鍗冲彲鐑у綍銆?
鍏蜂綋鏇存柊鎿嶄綔锛岃鍙傜収Dudu鏃堕挓杩欎竴鏈熺殑鏁欑▼锛屸€滄墜鎶婃墜澶嶅埢2.4瀵窪udu澶╂皵鏃堕挓 宸查€傞厤鍜岄澶╂皵JWT璁よ瘉鈥濓紝
https://www.bilibili.com/video/BV1N3JEz4E57/?vd_source=c106ac8c60f1249e356ef02bdcc85de7

*/


// 鑱旂綉鑾峰彇淇℃伅澶辫触鏃讹紝璺宠嚦鏄惁绂荤嚎浣跨敤椤甸潰
void step2OffLine(){
  // 鍏抽棴鍔犺浇鍔ㄧ敾
  loadingAnim = false;
  fadeOff();
  delay(200);
  // 缁樺埗setting椤甸潰
  drawSettingOrOffline(true, "Network failed");
  // 浣胯兘鎸夐敭
  buttonEnable = true;
  // 鏂紑WIFI
  // Keep WiFi alive so the phone control page remains available.
  // 鍒涘缓灞忓箷娓愭樉浠诲姟
  createFadeOnTask();
}

void setup(){
  Serial.begin(115200);
  // 鑾峰彇鍒濆鍖栧弬鏁?
  // setInfo4Test(); // 鍐欏叆娴嬭瘯鍙傛暟
  getInfo();
  // 鍒濆鍖朤FT_ESPI
  tftInit();
  tft.fillScreen(TFT_RED);
  delay(250);
  tft.fillScreen(TFT_GREEN);
  delay(250);
  tft.fillScreen(TFT_BLUE);
  delay(250);
  tft.fillScreen(TFT_WHITE);
  delay(250);
  refreshTFT();
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(penColor, backFillColor);
  tft.drawString("TFT OK", 160, 105);
  tft.drawString("Booting...", 160, 135);
  // 鍒濆鍖栦紶鎰熷櫒
  sensorsInit();
  // 缁樺埗寮€鍦哄姩鐢?
  if(!DEVELOP_MODE){
    // Skip the slow startup animation during hardware bring-up.
    refreshTFT();
    if(ssid.length() == 0 || pass.length() == 0 || city.length() == 0){ // 鏈厤杩囩綉
      currentPage = SETTING; // 灏嗛〉闈㈢疆涓洪厤缃〉闈?
      drawSettingOrOffline(true, "Start WiFi setup?");
      createFadeOnTask();
    }else{ // 宸查厤缃繃锛屽皾璇曡繛鎺?
      createDrawLoadingTask("Connecting WiFi");
      delay(200);
      createFadeOnTask();
      // Connect WiFi briefly, then fall back to the setup/offline page.
      connectWiFi(8);
      if(connected){ // 鎴愬姛鑱旂綉
        // 鍒濆鍖栧鏃跺伐鍏?
        startServer();
        timeClient.begin();
        // 寮€濮嬪鏃?
        logInfoln("Start NTP sync");
        int NTPtimes = 1;
        while(!timeClient.isTimeSet()){
          logInfo("NTP try ");
          logInfo(String(NTPtimes));
          logInfoln("");
          timeClient.update();
          delay(4000);
          if(NTPtimes >= DATA_FAILED_TIMES){
            logInfoln("NTP sync failed");
            getDataFailed = true;
            break;
          }
          NTPtimes++;
        }
        if(getDataFailed){ // NTP瀵规椂鍏ㄩ儴澶辫触锛岃烦鑷虫槸鍚︾绾夸娇鐢ㄩ〉闈?
          step2OffLine();
        }else{ // NTP瀵规椂鎴愬姛锛屾墠浼氳繘琛屼笅闈㈢殑閫昏緫
          logInfo("NTP sync ok: ");
          logInfoln(timeClient.getFormattedTime());
          syncClockFromNTP();
          writeTimeToDS3231();
          // Serial.println(timeClient.getEpochTime() - 8 * 3600);
          if(location.equals("")){ // 娌℃湁鍩庡競淇℃伅
            int cityCode = 0;
            int cityTimes = 1;
            while(cityCode != HTTP_CODE_OK){
              if(cityTimes >= DATA_FAILED_TIMES){
                logInfoln("Get city id failed");
                getDataFailed = true;
                break;
              }
              cityCode = getCityID();
              cityTimes++;
            }
          }
          // 鏈変簡鍩庡競淇℃伅锛岃繘琛孨TP瀵规椂锛屾煡璇㈠ぉ姘斾俊鎭?
          if(getDataFailed || location.equals("")){
            if(location.equals("")){
              logInfoln("City ID empty");
            }
            getDataFailed = true;
            step2OffLine();
          }else{
            // 鏌ヨ澶╂皵
            int weatherCode = 0;
            int weatherTimes = 1;
            while(weatherCode != HTTP_CODE_OK){
              if(weatherTimes >= DATA_FAILED_TIMES){
                logInfoln("Get weather failed");
                getDataFailed = true;
                break;
              }
              weatherCode = getWeather();
              weatherTimes++;
            }
            if(getDataFailed){ // 鑾峰彇澶╂皵淇℃伅鍏ㄩ儴澶辫触锛岃烦鑷虫槸鍚︾绾夸娇鐢ㄩ〉闈?
              step2OffLine();
            }else{ // 鑾峰彇澶╂皵淇℃伅鎴愬姛锛岀户缁幏鍙栫┖姘旇川閲忎俊鎭?
              // 鏌ヨ绌烘皵璐ㄩ噺
              int airCode = 0;
              int airTimes = 1;
              while(airCode != HTTP_CODE_OK){
                if(airTimes >= DATA_FAILED_TIMES){
                  logInfoln("Get air failed");
                  getDataFailed = true;
                  break;
                }
                airCode = getAir();
                airTimes++;              
              }
              if(getDataFailed){ // 鑾峰彇澶╂皵淇℃伅鍏ㄩ儴澶辫触锛岃烦鑷虫槸鍚︾绾夸娇鐢ㄩ〉闈?
                step2OffLine();
              }else{ // 鑾峰彇绌烘皵璐ㄩ噺淇℃伅鎴愬姛锛岀户缁笅闈㈢殑閫昏緫
                if(queryWeatherSuccess && queryAirSuccess){
                  mode = ONLINE_MODE;
                }
                currentPage = PAGE2;
                // 鍏抽棴鍔犺浇鍔ㄧ敾
                loadingAnim = false;
                fadeOff();
                delay(200);
                // Draw the main clock page.
                drawPage2Full();
                // 浣胯兘鎸夐敭
                buttonEnable = true;
                // 鍒涘缓灞忓箷娓愭樉浠诲姟
                createFadeOnTask();
              }
            }  
          }
        }
      }else{
        getDataFailed = true;
        step2OffLine();
      }
    }
  }else{
    // 娴嬭瘯浠ｇ爜
    currentPage = PAGE2;
    delay(1000);
    mode = ONLINE_MODE;
    drawPage2Full();
  }
  // 鍒涘缓鏍稿績0鐨勪紶鎰熷櫒浠诲姟
  createAnotherCoreTask();
  // 鍒涘缓鏍稿績0鐨凙DC閲囨牱浠诲姟
  createADCTask();
}

void loop(){
  static bool alarmOverlayShown = false;
  static int shownWarningType = 0;
  doClient();
  processMailAlert();
  if(updateWeather){
    logInfoln("Update weather");
    createDrawLoadingTask("Updating weather");
    unsigned long start = millis();
    getWeather();
    getAir();
    while((millis() - start) < 2000){
      delay(10);
    }
    // 鏇存柊澶╂皵鏍囧織閲嶆柊缃负false
    updateWeather = false;
    // 鍏抽棴鍔犺浇鍔ㄧ敾
    loadingAnim = false;
    fadeOff();
    delay(200);
    // 閲嶆柊缁樺埗鏇存柊澶╂皵涔嬪墠鐨勯〉闈?
    drawCurrentPage();
    createFadeOnTask();
  }
  watchBtn();
  int warningType = fireAlertActive() ? 1 : (theftAlertActive() ? 2 : 0);
  if(warningType != 0){
    if(!alarmOverlayShown || shownWarningType != warningType){
      drawCurrentPage();
      alarmOverlayShown = true;
      shownWarningType = warningType;
      sensorStateChanged = false;
      lastRefresh = millis();
    }
    return;
  }else if(alarmOverlayShown){
    alarmOverlayShown = false;
    shownWarningType = 0;
    sensorStateChanged = false;
    drawCurrentPage();
    lastRefresh = millis();
    return;
  }
  if(alarmPagePending){
    alarmPagePending = false;
    if(currentPage == SETTING || modalShowed){
      alarmReturnPage = PAGE2;
    }else{
      alarmReturnPage = currentPage;
    }
    modalShowed = false;
    alarmEditField = 0;
    currentPage = PAGE1;
    buttonEnable = true;
    drawAlarmRingingPage();
    lastRefresh = millis();
  }
  if(eventPagePending){
    eventPagePending = false;
    modalShowed = false;
    alarmEditField = 0;
    buttonEnable = true;
    drawEventRingingPage();
    lastRefresh = millis();
  }
  switch(currentPage){
    case SETTING:
      break;
    case PAGE1:
      if(!buttonEnable){
        return;
      }
      if(!modalShowed && !alarmRinging && (sensorStateChanged || (clockReady() && currentMinute() != displayMinute))){
        drawReminderStatus();
        sensorStateChanged = false;
        displayMinute = currentMinute();
        lastRefresh = millis();
      }
      break;
    case PAGE2:
      if(sensorStateChanged){
        drawPage2();
        sensorStateChanged = false;
        lastRefresh = millis();
      }else if(clockReady() && currentMinute() != displayMinute){
        drawPage2();
        displayMinute = currentMinute();
        lastRefresh = millis();
      }else if(clockReady() && currentSecond() != displaySecond){
        drawClockSecond();
        displaySecond = currentSecond();
      }
      break;
    case PAGE3:
      if(sensorStateChanged || (millis() - lastRefresh) >= 10000 || lastRefresh > millis()){
        drawPage3(false);
        sensorStateChanged = false;
        lastRefresh = millis();
      }
      break;
    case CALENDAR:
      // Keep the calendar mostly static; full redraws make button presses feel slow.
      if((millis() - lastRefresh) >= 5000 || lastRefresh > millis()){
        drawTop();
        lastRefresh = millis();
      }
      break;
    case CONFIG:
      if(!buttonEnable){
        return;
      }
      if((millis() - lastRefresh) >= 5000 || lastRefresh > millis()){
        drawTop();
        lastRefresh = millis();
      }
      break;  
    default:
      break;
  }
}
