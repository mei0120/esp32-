#ifndef __TASK_H
#define __TASK_H

#include "Common.h"

#define ADC_FREQUENCY   256
#define EVENT_SLOT_COUNT 3

extern enum CurrentPage currentPage;
extern unsigned long lastRefresh;
extern bool updateWeather;
// 数据相关
extern String tvoc;
extern String ch2o;
extern String co2;
extern String temperature;
extern String humidity;
// 系统变量
extern bool settingChoosed;
extern int mode;
extern bool buttonEnable;
extern bool voice;
extern bool loadingAnim;
extern bool modalShowed;
extern unsigned long lastUserAction;
extern float tempOffset;
extern float lightLux;
extern bool use24HourFormat;
extern bool fireAlarm;
extern bool simulatedFireAlarm;
extern bool infraredDetected;
extern bool fireAlarmAcknowledged;
extern bool theftAlarmAcknowledged;
extern volatile bool sensorStateChanged;
extern bool antiTheftMode;
extern bool alarmEnabled;
extern int alarmHour;
extern int alarmMinute;
extern int alarmTrack;
extern int alarmEditField;
extern int editAlarmHour;
extern int editAlarmMinute;
extern int editAlarmTrack;
extern bool alarmRinging;
extern volatile bool alarmPagePending;
extern enum CurrentPage alarmReturnPage;
extern bool eventEnabled;
extern int eventYear;
extern int eventMonth;
extern int eventDay;
extern String eventText;
extern bool eventEnabledList[EVENT_SLOT_COUNT];
extern int eventYearList[EVENT_SLOT_COUNT];
extern int eventMonthList[EVENT_SLOT_COUNT];
extern int eventDayList[EVENT_SLOT_COUNT];
extern int eventHourList[EVENT_SLOT_COUNT];
extern int eventMinuteList[EVENT_SLOT_COUNT];
extern String eventTextList[EVENT_SLOT_COUNT];
extern int activeEventIndex;
extern bool eventRinging;
extern volatile bool eventPagePending;
extern int lastEventDay;
// ADC相关
extern float batteryVoltage;
extern int batteryPercent;
extern bool Charging;
// 初始化函数
void sensorsInit();
void writeTimeToDS3231();
bool clockReady();
void syncClockFromNTP();
bool setClockDateTime(int year, int month, int day, int hour, int minute, int second);
unsigned long currentEpoch();
tm currentTimeInfo();
String currentFormattedTime();
String displayTimeText(bool includeSeconds = false);
int currentHour();
int currentMinute();
int currentSecond();
String eventReminderText();
int nextEventIndex();
String eventDateText(int index);
String eventScreenText(int index);
void syncEventSummary();
void stopEventRinging();
void skipTodayIfAlarmMatchesNow();
void updateWarningLight();
void setSimulatedFireAlarm(bool enabled, bool forceNotify = false);
bool fireAlertActive();
bool theftAlertActive();
void acknowledgeActiveWarning();
void stopAllAlerts();
uint16_t normalizeAlarmTrack(int track);
void stopAlarmRinging();
void playVoiceTrack(int track);
void jqStop();
void btnInit();
void watchBtn();
// FreeRTOS
void createFadeOnTask();
void createAnotherCoreTask();
void createDrawLoadingTask(const char *text);
void createADCTask();
// Ticker
void startTickerUpdateWeather();

#endif

