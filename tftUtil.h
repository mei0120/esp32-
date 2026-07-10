#ifndef __TFTUTIL_H
#define __TFTUTIL_H

#include <TFT_eSPI.h>

#define TEM   0
#define HUM   1
#define TVOC  2
#define CH2O  3
#define CO2   4
#define AQI   5
#define OPTION_VOICE 0
#define OPTION_OFFSET 1
#define OPTION_THEME 2
#define OPTION_SECURITY 3
#define OPTION_SMOKE_TEST 4
#define OPTION_ALARM 5
#define OPTION_WLAN 6
#define OPTION_RESET 7
#define OPTION_BACK 8
#define OPTION_COUNT 9
#define CALENDAR_RIGHT_OFFSET 0
#define CALENDAR_LEFT_OFFSET  1

extern TFT_eSPI tft;
extern TFT_eSprite clk;
extern int backColor;
extern int bright;
extern uint16_t backFillColor;
extern uint16_t penColor;
extern int configChoosedIndex;
extern bool modalLeftChoosed;
extern int displayMinute;
extern int displayHour;
extern int displaySecond;
extern bool getDataFailed;
extern int monthOffset;
extern int offsetDerection;
void tftInit();
void refreshTFT();
void initBacklight();
void setBacklight(int value);
void fadeOff();
void fadeOn();
void drawStartLoadingAnim();
void drawSettingOrOffline(bool refresh, String text);
void draw2LineText(String text1, String text2);
void drawLoading(bool firstTime, String text, int *angle);
void drawPage1();
void drawAlarmRingingPage();
void drawEventRingingPage();
void drawFireAlarmPage();
void drawSecurityAlarmPage();
void drawReminderStatus();
void drawStatusSensorBlock();
void drawTop();
void drawPage2();
void drawPage2Full();
void drawClockSecond();
void drawWeatherIconAt(int x, int y);
void drawPage3(bool refresh);
void drawConfig();
void drawCalendar();
void drawModal(String text, bool refresh);
void drawAlarmModal(bool refresh);
void drawConfigOption(int index);
void exchangeTheme();
void drawCurrentPage();
int getTotalDays(int year, int month);
int weekdayOfDate(int y, int m, int d);
String solarTermForDate(int m, int d);
String lunarDateForDate(int y, int m, int d);
String format2(int value);
String week(int tm_wday);
String monthDay(int tm_mon, int tm_mday);
String getWea(int icon);

#endif
