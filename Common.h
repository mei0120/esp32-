#ifndef __COMMON_H
#define __COMMON_H

#include "LogUtil.h"
#include "board_pins.h"
#include "img/wea/xue.h"
#include "img/wea/lei.h"
#include "img/wea/shachen.h"
#include "img/wea/wu.h"
#include "img/wea/bingbao.h"
#include "img/wea/yun.h"
#include "img/wea/yu.h"
#include "img/wea/yin.h"
#include "img/wea/qing.h"
#include "img/wea/xue_black.h"
#include "img/wea/lei_black.h"
#include "img/wea/shachen_black.h"
#include "img/wea/wu_black.h"
#include "img/wea/bingbao_black.h"
#include "img/wea/yun_black.h"
#include "img/wea/yu_black.h"
#include "img/wea/yin_black.h"
#include "img/wea/qing_black.h"
#include "img/page2_white.h"
#include "img/page2_black.h"
#include "fonts/name_24.h"
#include "fonts/projectName_26.h"
#include "fonts/settingPage_22.h"
#include "fonts/iconFont_16.h"
#include "fonts/iconFont_20.h"
#include "fonts/iconFont_22.h"
#include "fonts/unit_time_16.h"
#include "fonts/pollutantName_18.h"
#include "fonts/page1Num_64.h"
#include "fonts/page1Num_35.h"
#include "fonts/page2SensorNum_22.h"
#include "fonts/page2sensor_16.h"
#include "fonts/page3Num_90.h"
#include "fonts/page3_18.h"
#include "fonts/configTitle_26.h"
#include "fonts/configOption_18.h"
#include "fonts/batteryNum_14.h"
#include "fonts/calendar_22.h"
#include "fonts/calendar_18.h"

#define DEVELOP_MODE  false
#define NTP   "ntp5.aliyun.com"
#define HTTP_CODE_OK  200
#define ONLINE_MODE   0
#define OFFLINE_MODE  1
#define BACK_BLACK    0
#define BACK_WHITE    1
#define BRIGHT        50  //灞忓箷榛樿浜害
#define MAX_BRIGHT    196  //灞忓箷鏈€澶т寒搴?
#define MIN_BRIGHT    1    //灞忓箷鏈€灏忎寒搴?
#define BL         PIN_TFT_BL  //灞忓箷鑳屽厜
#define BTN1       PIN_BTN1 // 鎸夐挳1 宸︾Щ
#define BTN2       PIN_BTN2 // 鎸夐挳2 纭畾
#define BTN3       PIN_BTN3 // 鎸夐挳3 鍙崇Щ
#define TIME_CHECK_INTERVAL   10800 // NTP瀵规椂闂撮殧锛坰锛?3灏忔椂杩涜涓€娆″鏃?
#define UPDATE_WEATHER_INTERVAL   7200 // 鏇存柊澶╂皵闂撮殧锛坰锛?2灏忔椂鏇存柊涓€娆″ぉ姘?
#define DATA_FAILED_TIMES  10  // NTP鍜岃幏鍙栧ぉ姘斿け璐ユ鏁帮紝杈惧埌鎸囧畾娆℃暟锛岃繘鍏ョ绾挎ā寮?

// 鍜岄澶╂皵鎺ュ彛
const String cityURL = "/geo/v2/city/lookup";  // 鏌ヨ鍩庡競浠ｇ爜鐨勬帴鍙?
const String nowURL = "/v7/weather/now";  // 瀹炴椂澶╂皵鎺ュ彛
const String dailyURL = "/v7/weather/3d";  // 3-day weather forecast
const String airURL = "/airquality/v1/current/";  // 绌烘皵璐ㄩ噺鎺ュ彛
// 椤甸潰鏋氫妇
enum CurrentPage{
  SETTING, PAGE1, PAGE2, PAGE3, CALENDAR, CONFIG
};
// 瀹氫箟缁撴瀯浣?
typedef struct {
  String date;
  String text;
  int icon;
  int tempMin;
  int tempMax;
} ForecastDay;

typedef struct {
  String text;
  int icon;
  int temp;
  String feelsLike;
  String win;
  String vis;
  int humidity;
  int air;
  String pm10;
  String pm2p5;
  String no2;
  String so2;
  String co;
  String o3;
  ForecastDay forecast[3];
  int forecastCount;
  bool forecastReady;
} Weather;
// 閰嶇疆WiFi鐨勭綉椤典唬鐮?
// WiFi configuration web page
const String ROOT_HTML_PAGE1 PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang='en'>
<head>
  <meta charset='UTF-8'>
  <meta name='viewport' content='width=device-width, initial-scale=1.0'>
  <title>CC Air Detector Setup</title>
  <style>
    body{font-family:Arial,sans-serif;margin:0;background:#f4f6f8;color:#1f2933}
    .wrap{max-width:520px;margin:0 auto;padding:20px}
    h1{text-align:center;font-size:24px;margin:18px 0 6px}
    .hint{text-align:center;color:#64748b;font-size:14px;margin-bottom:18px}
    label{display:block;margin:14px 0 6px;font-weight:bold}
    select,input,button{box-sizing:border-box;width:100%;font-size:16px;padding:11px;border:1px solid #c9d1d9;border-radius:6px;background:white}
    button{margin-top:22px;background:#2563eb;color:white;border:0;font-weight:bold}
  </style>
</head>
<body>
  <div class='wrap'>
    <h1>CC Air Detector</h1>
    <div class='hint'>WiFi and city setup</div>
    <form action='configwifi' method='post' id='form' accept-charset='UTF-8'>
      <label for='ssid'>WiFi name</label>
      <select name='ssid' id='ssid'>
        <option value=''></option>
)rawliteral";
const String ROOT_HTML_PAGE2 PROGMEM = R"rawliteral(
      </select>
      <label for='pass'>WiFi password</label>
      <input type='text' placeholder='Enter WiFi password' name='pass' id='pass'>
      <label for='city'>City name</label>
      <input type='text' placeholder='Example: Jiangyin' name='city' id='city'>
      <label for='adm'>Region, optional</label>
      <input type='text' placeholder='Example: Jiangsu' name='adm' id='adm'>
      <button type='button' onclick='doSubmit()'>Save</button>
    </form>
  </div>
  <script>
    function doSubmit(){
      if(document.getElementById('ssid').value === ''){ alert('Please choose WiFi'); return; }
      if(document.getElementById('pass').value === ''){ alert('Please enter WiFi password'); return; }
      if(document.getElementById('city').value === ''){ alert('Please enter city'); return; }
      document.getElementById('form').submit();
    }
  </script>
</body>
</html>
)rawliteral";
#endif
