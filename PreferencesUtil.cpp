#include <Preferences.h>
#include "Net.h"
#include "Common.h"
#include "tftUtil.h"
#include "Task.h"

Preferences prefs; // 声明Preferences对象

// 读取一系列初始化参数
void getInfo(){
  prefs.begin("project");
  ssid = prefs.getString("ssid", "");
  pass = prefs.getString("pass", "");
  city = prefs.getString("city", "");
  adm = prefs.getString("adm", "");
  lat = prefs.getString("lat", "");
  lon = prefs.getString("lon", "");
  location = prefs.getString("location", "");
  bright = prefs.getInt("bright", BRIGHT);
  backColor = prefs.getInt("backColor",BACK_BLACK);
  voice = prefs.getBool("voice", true);
  use24HourFormat = prefs.getBool("use24Hour", false);
  tempOffset = prefs.getFloat("tempOffset", 0.0);
  alarmEnabled = prefs.getBool("alarmEnabled", false);
  alarmHour = prefs.getInt("alarmHour", 7);
  alarmMinute = prefs.getInt("alarmMinute", 0);
  alarmTrack = normalizeAlarmTrack(prefs.getInt("alarmTrack", 1));
  antiTheftMode = prefs.getBool("antiTheftMode", true);
  eventEnabled = prefs.getBool("eventEnabled", false);
  eventYear = prefs.getInt("eventYear", 0);
  eventMonth = prefs.getInt("eventMonth", 0);
  eventDay = prefs.getInt("eventDay", 0);
  eventText = prefs.getString("eventText", "");
  for(int i = 0; i < EVENT_SLOT_COUNT; i++){
    String suffix = String(i);
    eventEnabledList[i] = prefs.getBool(("eventEn" + suffix).c_str(), false);
    eventYearList[i] = prefs.getInt(("eventY" + suffix).c_str(), 0);
    eventMonthList[i] = prefs.getInt(("eventM" + suffix).c_str(), 0);
    eventDayList[i] = prefs.getInt(("eventD" + suffix).c_str(), 0);
    eventTextList[i] = prefs.getString(("eventT" + suffix).c_str(), "");
  }
  if(eventText.length() > 0 && eventYear > 0 && eventYearList[0] == 0){
    eventEnabledList[0] = eventEnabled;
    eventYearList[0] = eventYear;
    eventMonthList[0] = eventMonth;
    eventDayList[0] = eventDay;
    eventTextList[0] = eventText;
  }
  syncEventSummary();
  prefs.end();
}

// 写入初始化参数
void setInfo(){
  prefs.begin("project");
  prefs.putString("ssid", ssid);
  prefs.putString("pass", pass);
  prefs.putString("city", city);
  prefs.putString("adm", adm);
  prefs.putString("location", location);
  prefs.putString("lat", lat);
  prefs.putString("lon", lon);
  prefs.end();
}

// 写入声音参数
void setVoice(){
  prefs.begin("project");
  prefs.putBool("voice", voice);
  prefs.end();
}

// 写入亮度参数
void setBright(){
  prefs.begin("project");
  prefs.putInt("bright", bright);
  prefs.end();
}

void setTimeFormatPrefs(){
  prefs.begin("project");
  prefs.putBool("use24Hour", use24HourFormat);
  prefs.end();
}

// 写入温度偏移值
void setTempOffset(){
  prefs.begin("project");
  prefs.putFloat("tempOffset", tempOffset);
  prefs.end();
}

// 写入主题参数
void setAlarmPrefs(){
  prefs.begin("project");
  prefs.putBool("alarmEnabled", alarmEnabled);
  prefs.putInt("alarmHour", alarmHour);
  prefs.putInt("alarmMinute", alarmMinute);
  prefs.putInt("alarmTrack", alarmTrack);
  prefs.putBool("antiTheftMode", antiTheftMode);
  prefs.end();
}

void setEventPrefs(){
  syncEventSummary();
  prefs.begin("project");
  prefs.putBool("eventEnabled", eventEnabled);
  prefs.putInt("eventYear", eventYear);
  prefs.putInt("eventMonth", eventMonth);
  prefs.putInt("eventDay", eventDay);
  prefs.putString("eventText", eventText);
  for(int i = 0; i < EVENT_SLOT_COUNT; i++){
    String suffix = String(i);
    prefs.putBool(("eventEn" + suffix).c_str(), eventEnabledList[i]);
    prefs.putInt(("eventY" + suffix).c_str(), eventYearList[i]);
    prefs.putInt(("eventM" + suffix).c_str(), eventMonthList[i]);
    prefs.putInt(("eventD" + suffix).c_str(), eventDayList[i]);
    prefs.putString(("eventT" + suffix).c_str(), eventTextList[i]);
  }
  prefs.end();
}

void setTheme(){
  prefs.begin("project");
  prefs.putInt("backColor",backColor);
  prefs.end();
}

// 清除所有初始化参数
void clearInfo(){
  prefs.begin("project");
  prefs.clear();
  prefs.end();
}

// 测试用，在读取NVS之前，先写入自己的Wifi信息，免得每次浪费时间再配网
void setInfo4Test(){
  prefs.begin("project");
  prefs.putString("ssid", "sh.soft");
  prefs.putString("pass", "ws123789");
  prefs.putString("city", "江阴");
  prefs.putString("adm", "");
  prefs.putString("location", "");
  prefs.putInt("bright", 100);
  prefs.putInt("backColor",BACK_BLACK);
  prefs.end();
}
