#include <string.h>
#include <stdio.h>

#include <time.h>
#include "EEPROM.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

#include <ArduinoJson.h>
#include <tinyxml2.h>

#include <TFT_eSPI.h>
TFT_eSPI tft = TFT_eSPI();
#include <AimHangul.h>
#include "CST816S.h"

// Image
#include "DUCK_TIME_240.h"
#include "DUCK_WEATHER_240.h"
#include "DUCK_BUS_240.h"
#include "DUCK_STOCK_KR_240.h"
#include "DUCK_STOCK_US_240.h"

/*************************** [CONSTANT Definition] ************************************/

/* GC9A01 display */
#define TFT_MOSI 3
#define TFT_SCLK 0
#define TFT_CS 1
#define TFT_DC 10

/* CST816S Touch */
#define TOUCH_SDA 4
#define TOUCH_SCL 5
#define TOUCH_INT 6
#define TOUCH_RST 7

#define EEPROM_SIZE 256
/*************************** [CONSTANT Definition] ************************************/

/*************************** [API KEY Definition] ************************************/
#define PUBLIC_DATA_API_KEY "HHec4G%2FFjjMjQIfzZa3yfZuItK93BQh%2BC%2FwkITl%2FCu8X3h%2BAjlF74glKicSnEN%2BVeZEOvstt07Zz%2Be%2BvmAlFVQ%3D%3D"
#define OPENWEATHER_API_KEY "8d27c680ce67b6ffebcf09d005cdd444"
#define FINNHUB_STOCK_API_KEY "ci77auhr01quivoc1e0gci77auhr01quivoc1e10"
/*************************** [API KEY Definition] ************************************/

/*************************** [Structure Definition] ************************************/
typedef enum {
  WIDGET_NULL = 0,
  WIDGET_STOP = 2,
  WIDGET_GO = 3
} TABLECLOCK_STATE;

typedef struct
{
  char wifi_id[32];
  char wifi_pw[32];
  char bus_id[32];
  char stock_num[32];
} USER_DATA;

typedef struct
{
  int16_t year;  // 연도
  int8_t mon;    // 달
  int8_t mday;   // 일
  int8_t wday;   // 요일

  int8_t hour;
  int8_t min;
  int8_t sec;
} TIME;

typedef struct
{
  char city[20];
  char weather[20];
  int temp;
} WEATHER;

typedef struct
{
  int mobileNo;        // 정류소 번호
  char routeName[30];  // 노선 번호

  char stationName[100];  // 정류소명
  int stationId;          // 정류소 아이디
  int routeId;            // 노선 아이디
  int staOrder;           // 정류소 순번

  char routeName_temp[30];

  int locationNo1;
  int predictTime1;
  int locationNo2;
  int predictTime2;
} BUS;

typedef struct
{
  char code[9];  // 단축코드

  char name[120];          // 종목명
  char date[9];            // 기준일자
  char closePrice[12];     // 종가
  char change[10];         // 대비
  char percentChange[11];  // 등락률
} STOCK_KR;

typedef struct
{
  char name[120];  // 종목명

  int date;             // 기준일자
  float currentPrice;   // 현재가
  float change;         // 대비
  float percentChange;  // 등락률

} STOCK_US;
/*************************** [Structure Definition] ************************************/

/*************************** [Variable Declaration] ************************************/
TFT_eSprite img = TFT_eSprite(&tft);
CST816S touch(TOUCH_SDA, TOUCH_SCL, TOUCH_RST, TOUCH_INT);

TABLECLOCK_STATE my_state = WIDGET_NULL;

USER_DATA my_data;

TIME myTime;
BUS myBus;
WEATHER myWeather;
STOCK_KR myStockKR;
STOCK_US myStockUS;

// NTP server to request epoch time
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 32400;  // +9:00 (9hours*60mins*60seconds)
const int daylightOffset_sec = 0;

wifi_mode_t esp_wifi_mode = WIFI_MODE_NULL;
WiFiClient mySocket;
HTTPClient myHTTP;

using namespace tinyxml2;
DynamicJsonDocument jsonDoc(2048);
char xmlDoc[4096];
/*************************** [Variable Declaration] ************************************/

/*************************** [Function Declaration] ************************************/
void clearUserData();
void resetUserData(bool overwrite);

void initWiFi();
bool checkWiFiStatus();

void getDateTime();
void getWeather(const char* city);

void getBusStationId(int mobileNo);
void getStaOrder();
void getBusArrival(const char* routeName, int stationId, int routeId, int staOrder);

void getStockPriceKRPreviousDay(const char* code);
void getStockPriceUSRealTime(const char* name);
/*************************** [Function Declaration] ************************************/

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  touch.begin();

  tft.init();
  tft.setRotation(0);
  tft.setSwapBytes(true);
  img.setSwapBytes(true);
  tft.fillScreen(TFT_WHITE);
  img.createSprite(240, 240);

  Serial.println();
  Serial.println();
  Serial.println("-------------------------------------");
  Serial.println("----------widget_tableclock----------");
  Serial.println("-------------------------------------");
  Serial.println();
  Serial.println();

  resetUserData(0);
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  img.pushImage(0, 0, 240, 240, DUCK_TIME_240);
  img.pushSprite(0, 0);
}

void loop() {
  // Keyboard Interrupt
  if (Serial.available()) {
    char key = Serial.read();

    if (key == 'r') resetUserData(1);
    else if (key == 't') getDateTime();
    else if (key == 'w') getWeather("Yongin");
    else if (key == 'b') getBusArrival("66-4", 200000177, 200000037, 19);
    else if (key == 'k') getStockPriceKRPreviousDay("005930");
    else if (key == 'u') getStockPriceUSRealTime("MSFT");
  }

  // if(!checkWiFiStatus())
  // {
  //     Serial.println("WiFi Disconnected, Check WiFi Signal");
  //     resetUserData(1);
  // }

  // if(touch.available())
  // {
  //     Serial.print(touch.data.x);
  //     Serial.print(" ");
  //     Serial.print(touch.data.y);
  //     Serial.print(" ");
  //     Serial.println(touch.gesture().c_str());

  //     tft.fillScreen(TFT_BLACK);
  //     tft.setCursor(100, 120);
  //     tft.setTextColor(TFT_WHITE);
  //     tft.setTextSize(2);
  //     tft.print(touch.data.x);
  //     tft.print(" ");
  //     tft.println(touch.data.y);
  //     tft.setCursor(80, 160);
  //     tft.println(touch.gesture().c_str());
  // }
}

/***************************Function Definition************************************/
void clearUserData() {
  memset(my_data.wifi_id, '\0', 32);
  memset(my_data.wifi_pw, '\0', 32);
  memset(my_data.bus_id, '\0', 32);
  memset(my_data.stock_num, '\0', 32);
}

void resetUserData(bool overwrite) {
  // neopixelWrite(RGB_BUILTIN,0xFF,0xFF,0x00); // yellow

  char input[EEPROM_SIZE] = { 0 };
  char output[EEPROM_SIZE] = { 0 };
  uint16_t index = 0;
  uint16_t data = 0;
  bool input_flag = false;
  uint8_t err_chk_1 = 0;
  uint8_t err_chk_2 = 0;
  uint8_t param_start = 0;
  uint8_t param_end = 0;

  if (!EEPROM.begin(EEPROM_SIZE)) {
    Serial.println("failed to initialise EEPROM");
    return;
  }

  if (overwrite)
    goto reset;

  EEPROM.readString(0, output, EEPROM_SIZE);  // read eeprom data
  if (output[0] == '[')
    goto parse;

reset:
  clearUserData();

  Serial.println("--------------------------------");
  Serial.println("Reset User Data Start");
  Serial.println("--------------------------------");

  Serial.println("Clear User Data");
  for (int i = 0; i < EEPROM_SIZE; i++)
    EEPROM.write(i, 0xFF);
  EEPROM.commit();
  delay(500);

  Serial.println("Enter [WiFi ID][WiFi PW][BUS1, BUS2, BUS3][STOCK1,STOCK2,STOCK3]");

  while (1)  // wait user data
  {
    while (Serial.available()) {
      input[index] = Serial.read();
      if (input[index] == '[') err_chk_1++;
      if (input[index] == ']') err_chk_2++;

      index++;
      input_flag = true;
    }

    if (input_flag) {
      if ((err_chk_1 == 4) && (err_chk_2 == 4))  // check & save user data
      {
        EEPROM.writeString(0, input);
        EEPROM.commit();
        delay(500);

        Serial.print("Save User Data ");
        EEPROM.readString(0, output, EEPROM_SIZE);
        Serial.println(output);
        index = 0;
        err_chk_1 = 0;
        err_chk_2 = 0;
        input_flag = false;
        break;
      } else {
        Serial.println("Input Error!");
        memset(input, '\0', 240);
        index = 0;
        err_chk_1 = 0;
        err_chk_2 = 0;
      }
      input_flag = false;
    }
  }

  Serial.println("--------------------------------");
  Serial.println("Reset User Data END");
  Serial.println("--------------------------------");

parse:
  for (int i = 0; i < EEPROM_SIZE; i++) {
    if (output[i] == '[') {
      param_start++;
    } else if (output[i] == ']') {
      param_end++;
      if (param_end == 4) break;
      data = 0;
    } else {
      if (param_start == 1) {
        my_data.wifi_id[data] = output[i];
        data++;
      }
      if (param_start == 2) {
        my_data.wifi_pw[data] = output[i];
        data++;
      }
      if (param_start == 3) {
        my_data.bus_id[data] = output[i];
        data++;
      }

      if (param_start == 4) {
        my_data.stock_num[data] = output[i];
        data++;
      }
    }
  }

  param_start = 0;

  printf("[WIFI ID] %s \r\n", my_data.wifi_id);
  printf("[WIFI PW] %s \r\n", my_data.wifi_pw);
  printf("[BUS] %s \r\n", my_data.bus_id);
  printf("[STOCK] %s \r\n", my_data.stock_num);

  initWiFi();

  // neopixelWrite(RGB_BUILTIN,0x00,0xFF,0x00); // green
}

void initWiFi() {
  uint8_t wifi_timeout = 0;
  Serial.println("WIFI Connecting");
  WiFi.begin(my_data.wifi_id, my_data.wifi_pw);

  if (checkWiFiStatus())
    Serial.println("WIFI CONNECTED");
  else {
    Serial.println();
    Serial.println("WiFi Not Connected, Check [WiFi ID][WiFi PW]");
    resetUserData(1);
  }
}

bool checkWiFiStatus() {
  uint8_t wifi_timeout = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    wifi_timeout++;
    Serial.print(".");
    if (wifi_timeout == 20) {
      return 0;
    }
  }
  return 1;
}

void getDateTime() {
  Serial.println("-------------------------------------");
  Serial.println("[REQUEST] Date/Time");

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("[ERROR]");
    Serial.println("-------------------------------------");
    return;
  }

  myTime.year = timeinfo.tm_year + 1900;
  printf("[연도] %d\r\n", myTime.year);

  myTime.mon = timeinfo.tm_mon + 1;
  printf("[월] %d\r\n", myTime.mon);

  myTime.mday = timeinfo.tm_mday;
  printf("[일] %d\r\n", myTime.mday);

  myTime.wday = timeinfo.tm_wday;
  printf("[요일] %d\r\n", myTime.wday);

  myTime.hour = timeinfo.tm_hour;
  printf("[시] %d\r\n", myTime.hour);

  myTime.min = timeinfo.tm_min;
  printf("[분] %d\r\n", myTime.min);

  myTime.sec = timeinfo.tm_sec;
  printf("[초] %d\r\n", myTime.sec);

  img.pushImage(0, 0, 240, 240, DUCK_TIME_240);
  img.pushSprite(0, 0);

  tft.setCursor(80, 50);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(3);

  if (myTime.mon < 10) tft.print("0");
  tft.print(myTime.mon);
  tft.print("/");
  if (myTime.mday < 10) tft.print("0");
  tft.print(myTime.mday);

  tft.setCursor(80, 80);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(3);

  if (myTime.hour < 10) tft.print("0");
  tft.print(myTime.hour);
  tft.print(":");
  if (myTime.min < 10) tft.print("0");
  tft.println(myTime.min);

  Serial.println("-------------------------------------");
}

void getWeather(const char* city) {
  Serial.println("-------------------------------------");
  Serial.println("[REQUEST] Weather");

  int httpCode;
  char buffer[256];
  strcpy(myWeather.city, city);

  snprintf(buffer, sizeof(buffer), "http://api.openweathermap.org/data/2.5/weather?q=%s&appid=%s", myWeather.city, OPENWEATHER_API_KEY);
  myHTTP.begin(mySocket, buffer);
  delay(100);

  httpCode = myHTTP.GET();
  printf("[HTTP CODE] %d \r\n", httpCode);

  if (httpCode == HTTP_CODE_OK) {
    Serial.println("[OK]");
    deserializeJson(jsonDoc, myHTTP.getString());
  } else {
    Serial.println("[ERROR]");
  }
  myHTTP.end();

  const char* city_r = jsonDoc["name"];
  const char* weather = jsonDoc["weather"][0]["main"];
  myWeather.temp = (int)(jsonDoc["main"]["temp"]) - 273.0;

  strcpy(myWeather.city, city_r);
  strcpy(myWeather.weather, weather);

  printf("[도시] %s \r\n", myWeather.city);
  printf("[날씨] %s \r\n", myWeather.weather);
  printf("[기온] %d \r\n", myWeather.temp);

  img.pushImage(0, 0, 240, 240, DUCK_WEATHER_240);
  img.pushSprite(0, 0);

  tft.setCursor(60, 40);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(3);
  tft.print(myWeather.city);

  tft.setCursor(60, 70);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(3);
  tft.print(myWeather.weather);

  tft.setCursor(60, 100);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(3);
  tft.print(myWeather.temp);
  tft.print("C");

  Serial.println("-------------------------------------");
}

void getBusStationId(int mobileNo) {
  Serial.println("-------------------------------------");
  Serial.println("[REQUEST] Bus Station ID");

  int httpCode;
  char buffer[300];
  myBus.mobileNo = mobileNo;

  snprintf(buffer, sizeof(buffer), "http://apis.data.go.kr/6410000/busstationservice/getBusStationList?serviceKey=%s&keyword=%d", PUBLIC_DATA_API_KEY, myBus.mobileNo);
  myHTTP.begin(mySocket, buffer);

  httpCode = myHTTP.GET();
  printf("[HTTP CODE] %d \r\n", httpCode);

  if (httpCode == HTTP_CODE_OK) {
    Serial.println("[OK]");
    strcpy(xmlDoc, myHTTP.getString().c_str());
  } else {
    Serial.println("[ERROR]");
    myHTTP.end();
    Serial.println("-------------------------------------");
    return;
  }
  myHTTP.end();

  XMLDocument xmlDocument;
  if (xmlDocument.Parse(xmlDoc) != XML_SUCCESS) {
    Serial.println("[PARSE ERROR]");
    myHTTP.end();
    Serial.println("-------------------------------------");
    return;
  };

  XMLNode* root = xmlDocument.RootElement();

  // 정류소 아이디
  XMLElement* element = root->FirstChildElement("msgBody")->FirstChildElement("busStationList")->FirstChildElement("stationId");
  element->QueryIntText(&myBus.stationId);
  printf("[정류소 아이디] %d\r\n", myBus.stationId);

  // 정류소 명
  element = root->FirstChildElement("msgBody")->FirstChildElement("busArrivalItem")->FirstChildElement("stationName");
  strcpy(myBus.stationName, element->GetText());
  printf("[정류소 명] %d분후\r\n", myBus.stationName);

  Serial.println("-------------------------------------");
}

void getStaOrder(int stationId, const char* routeName) {
  Serial.println("-------------------------------------");
  Serial.println("[REQUEST] Bus StaOrder");

  int httpCode;
  char buffer[300];
  myBus.stationId = stationId;
  strcpy(myBus.routeName, routeName);

  snprintf(buffer, sizeof(buffer), "http://apis.data.go.kr/6410000/busstationservice/getBusStationViaRouteList?serviceKey=%s&stationId=%d", PUBLIC_DATA_API_KEY, myBus.stationId);
  myHTTP.begin(mySocket, buffer);

  if (myHTTP.GET() == HTTP_CODE_OK) {
    Serial.println("[OK]");
    strcpy(xmlDoc, myHTTP.getString().c_str());
  } else {
    Serial.println("[ERROR]");
    myHTTP.end();
    Serial.println("-------------------------------------");
    return;
  }

  XMLDocument xmlDocument;
  if (xmlDocument.Parse(xmlDoc) != XML_SUCCESS) {
    Serial.println("[PARSE ERROR]");
    myHTTP.end();
    Serial.println("-------------------------------------");
    return;
  };

  XMLNode* root = xmlDocument.RootElement();
  XMLElement* element = root->FirstChildElement("msgBody")->FirstChildElement("busStationList")->FirstChildElement("routeName");

  for (XMLElement* routeName = element; routeName != NULL; routeName->NextSiblingElement()) {
    strcpy(myBus.routeName_temp, element->GetText());

    if (strcpy(myBus.routeName_temp, myBus.routeName) == 0) {
      XMLElement* ele1 = root->FirstChildElement("msgBody")->FirstChildElement("busStationList")->FirstChildElement("routeId");
      ele1->QueryIntText(&myBus.routeId);

      XMLElement* ele2 = root->FirstChildElement("msgBody")->FirstChildElement("busStationList")->FirstChildElement("staOrder");
      ele2->QueryIntText(&myBus.staOrder);

      break;
    }
  }

  Serial.print("Bus routeId : ");
  Serial.println(myBus.routeId);
  Serial.print("Bus staOrder : ");
  Serial.println(myBus.staOrder);

  myHTTP.end();
}

void getBusArrival(const char* routeName, int stationId, int routeId, int staOrder)  // getBusArrivalItem Operation
{
  Serial.println("-------------------------------------");
  Serial.println("[REQUEST] BUS Arrival");

  int httpCode;
  char buffer[300];
  char routeName_local[30];

  strcpy(routeName_local, routeName);
  myBus.stationId = stationId;
  myBus.routeId = routeId;
  myBus.staOrder = staOrder;

  snprintf(buffer, sizeof(buffer), "http://apis.data.go.kr/6410000/busarrivalservice/getBusArrivalItem?serviceKey=%s&stationId=%d&routeId=%d&staOrder=%d", PUBLIC_DATA_API_KEY, myBus.stationId, myBus.routeId, myBus.staOrder);
  myHTTP.begin(mySocket, buffer);

  httpCode = myHTTP.GET();
  printf("[HTTP CODE] %d \r\n", httpCode);

  if (httpCode == HTTP_CODE_OK) {
    Serial.println("[OK]");
    strcpy(xmlDoc, myHTTP.getString().c_str());
  } else {
    Serial.println("[ERROR]");
    myHTTP.end();
    Serial.println("-------------------------------------");
    return;
  }

  XMLDocument xmlDocument;
  if (xmlDocument.Parse(xmlDoc) != XML_SUCCESS) {
    Serial.println("[PARSE ERROR]");
    Serial.println("-------------------------------------");
    return;
  };
  myHTTP.end();

  XMLNode* root = xmlDocument.RootElement();

  // 첫번째차량 위치정보
  XMLElement* element = root->FirstChildElement("msgBody")->FirstChildElement("busArrivalItem")->FirstChildElement("locationNo1");
  element->QueryIntText(&myBus.locationNo1);
  printf("[첫번째버스 위치정보] %d번째전 정류소\r\n", myBus.locationNo1);

  // 첫번째차량 도착예상시간
  element = root->FirstChildElement("msgBody")->FirstChildElement("busArrivalItem")->FirstChildElement("predictTime1");
  element->QueryIntText(&myBus.predictTime1);
  printf("[첫번째버스 도착예상시간] %d분후\r\n", myBus.locationNo1);

  // 두번째차량 위치정보
  element = root->FirstChildElement("msgBody")->FirstChildElement("busArrivalItem")->FirstChildElement("locationNo2");
  element->QueryIntText(&myBus.locationNo2);
  printf("[두번재버스 위치정보] %d번째전 정류소\r\n", myBus.locationNo2);

  // 두째차량 도착예상시간
  element = root->FirstChildElement("msgBody")->FirstChildElement("busArrivalItem")->FirstChildElement("predictTime2");
  element->QueryIntText(&myBus.predictTime2);
  printf("[두번재버스 도착예상시간] %d분후\r\n", myBus.predictTime2);

  img.pushImage(0, 0, 240, 240, DUCK_BUS_240);
  img.pushSprite(0, 0);

  tft.setCursor(80, 50);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(3);

  tft.print(routeName_local);

  Serial.println("-------------------------------------");
}

void getStockPriceKRPreviousDay(const char* code)  // 한국주식 전날 시세
{
    Serial.println("-------------------------------------");
    Serial.println("[REQUEST] KR stock");

    int httpCode;
    char buffer[300];
    strcpy(myStockKR.code, code);

    snprintf(buffer, sizeof(buffer), "http://apis.data.go.kr/1160100/service/GetStockSecuritiesInfoService/getStockPriceInfo?serviceKey=%s&numOfRows=1&pageNo=1&likeSrtnCd=%s", PUBLIC_DATA_API_KEY, myStockKR.code);
    myHTTP.begin(mySocket, buffer);
    delay(500);

    httpCode = myHTTP.GET();
    printf("[HTTP CODE] %d \r\n", httpCode);

    if (httpCode == HTTP_CODE_OK) {
        Serial.println("[OK]");
        strcpy(xmlDoc, myHTTP.getString().c_str());
    } else {
        Serial.println("[ERROR]");
        myHTTP.end();
        Serial.println("-------------------------------------");
        return;
    }
    myHTTP.end();

    XMLDocument xmlDocument;
    if (xmlDocument.Parse(xmlDoc) != XML_SUCCESS) {
        Serial.println("[PARSE ERROR]");
        Serial.println("-------------------------------------");
        return;
    };

    XMLNode* root = xmlDocument.RootElement();

    // 종목명
    XMLElement* element = root->FirstChildElement("body")->FirstChildElement("items")->FirstChildElement("item")->FirstChildElement("itmsNm");
    strcpy(myStockKR.name, element->GetText());
    printf("[종목명] %s \r\n", myStockKR.name);

    // 기준일자
    element = root->FirstChildElement("body")->FirstChildElement("items")->FirstChildElement("item")->FirstChildElement("basDt");
    strcpy(myStockKR.date, element->GetText());
    myStockKR.date[8] = '\0';
    printf("[기준일자] %s \r\n", myStockKR.date);

    // 종가
    element = root->FirstChildElement("body")->FirstChildElement("items")->FirstChildElement("item")->FirstChildElement("clpr");
    strcpy(myStockKR.closePrice, element->GetText());
    printf("[종가] %s \r\n", myStockKR.closePrice);

    // 대비
    element = root->FirstChildElement("body")->FirstChildElement("items")->FirstChildElement("item")->FirstChildElement("vs");
    strcpy(myStockKR.change, element->GetText());
    printf("[대비] %s \r\n", myStockKR.change);

    // 등락률
    element = root->FirstChildElement("body")->FirstChildElement("items")->FirstChildElement("item")->FirstChildElement("fltRt");
    strcpy(myStockKR.percentChange, element->GetText());
    printf("[등락률] %s \r\n", myStockKR.percentChange);

    img.pushImage(0, 0, 240, 240, DUCK_STOCK_KR_240);
    img.pushSprite(0, 0);

    // 날짜
    tft.setCursor(120-(12*4), 30); // 중앙정렬
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    tft.printf("%s", myStockKR.date);

    // 종목
    String str(myStockKR.name);
    int nameLen = strlen(myStockKR.name) / 3;
    printf("name len %d\r\n", nameLen);
    AimHangul_v2(120 - (float)(nameLen/2) * 16, 50, str, TFT_WHITE);  // 중앙정렬

    // 종가
    int closePriceLen = strlen(myStockKR.closePrice);
    printf("closePriceLen %d\r\n", closePriceLen);
    tft.setCursor(120 - (float)(closePriceLen/2) * 20, 50 + 36);  // 중앙정렬
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(3);
    tft.print(myStockKR.closePrice);
    printf("%d\r\n", tft.getCursorX());

    // 대비, 등락률
    char changeBuffer[50];
    snprintf(changeBuffer, sizeof(changeBuffer), "%s %s%%", myStockKR.change, myStockKR.percentChange);
    int changeBufferLen = strlen(changeBuffer);
    printf("changeBufferLen %d\r\n", changeBufferLen);
    tft.setCursor(120 - (float)(changeBufferLen/2) * 12, 50 + 36 + 30);  // 중앙정렬
    if (myStockKR.change[0] == '+') tft.setTextColor(TFT_RED, TFT_WHITE, 1);
    else tft.setTextColor(TFT_BLUE, TFT_WHITE, 1);
    tft.setTextSize(2);
    tft.print(changeBuffer);

    Serial.println("-------------------------------------");
}

void getStockPriceUSRealTime(const char* name)  // 미국주식 실시간 시세
{
    Serial.println("-------------------------------------");
    Serial.println("[REQUEST] US Stock");

    int httpCode;
    char buffer[256];
    strcpy(myStockUS.name, name);
    snprintf(buffer, sizeof(buffer), "https://finnhub.io/api/v1/quote?symbol=%s&token=%s", myStockUS.name, FINNHUB_STOCK_API_KEY);

    WiFiClientSecure* client = new WiFiClientSecure;
    client->setInsecure();
    myHTTP.begin(*client, buffer);
    delay(500);

    httpCode = myHTTP.GET();
    printf("[HTTP CODE] %d \r\n", httpCode);

    if (httpCode == HTTP_CODE_OK) {
        Serial.println("[OK]");
        deserializeJson(jsonDoc, myHTTP.getString());
    } else {
        Serial.println("[ERROR]");
        myHTTP.end();
        Serial.println("-------------------------------------");
    }
    myHTTP.end();

    myStockUS.date = (int)(jsonDoc["t"]);
    myStockUS.currentPrice = (float)(jsonDoc["c"]);
    myStockUS.change = (float)(jsonDoc["d"]);
    myStockUS.percentChange = (float)(jsonDoc["dp"]);

    printf("[종목명] %s \r\n", myStockUS.name);
    printf("[기준일자] %d \r\n", myStockUS.date);
    printf("[현재가] %.2f \r\n", myStockUS.currentPrice);
    printf("[대비] %.2f \r\n", myStockUS.change);
    printf("[등락률] %.2f \r\n", myStockUS.percentChange);

    img.pushImage(0, 0, 240, 240, DUCK_STOCK_US_240);
    img.pushSprite(0, 0);

    time_t rawTime = myStockUS.date;
    struct tm ts;
    ts = *localtime(&rawTime);

    // 시각
    tft.setCursor(72, 30);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);

    if (ts.tm_hour < 10) tft.print("0");
    tft.print(ts.tm_hour);
    tft.print(":");
    if (ts.tm_min < 10) tft.print("0");
    tft.print(ts.tm_min);
    tft.print(":");
    if(ts.tm_sec < 10) tft.print("0");
    tft.print(ts.tm_sec);

    // 종목명
    int nameLen = strlen(myStockUS.name);
    tft.setCursor(120 - (float)(nameLen/2) * 18, 55);  // 중앙정렬
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(3);
    tft.print(myStockUS.name);

    // 현재가
    char curPriceBuf[10];
    snprintf(curPriceBuf, sizeof(curPriceBuf), "%.2f", myStockUS.currentPrice);
    int curPriceLen = strlen(curPriceBuf);
    tft.setCursor(120 - (float)(curPriceLen/2) * 18, 50 + 36);  // 중앙정렬
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(3);
    tft.print(myStockUS.currentPrice);

    // 대비, 등락률
    char changeBuf[50];
    snprintf(changeBuf, sizeof(changeBuf), "%.2f %.2f%%", myStockUS.change, myStockUS.percentChange);
    int changeBufLen = strlen(changeBuf);
    tft.setCursor(120 - (float)(changeBufLen/2) * 12, 50 + 36 + 30);  // 중앙정렬
    if (myStockUS.change >= 0) tft.setTextColor(TFT_RED, TFT_WHITE, 1);
    else tft.setTextColor(TFT_BLUE, TFT_WHITE, 1);
    tft.setTextSize(2);
    tft.print(changeBuf);

  Serial.println("-------------------------------------");
}
/***************************Function Definition************************************/