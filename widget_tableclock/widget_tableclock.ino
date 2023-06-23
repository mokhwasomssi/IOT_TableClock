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
#include "DUCK_HI_240.h"
#include "DUCK_TIME_240.h"
#include "DUCK_WEATHER_240.h"
#include "DUCK_BUS_240.h"
#include "DUCK_STOCK_KR_240.h"
#include "DUCK_STOCK_US_240.h"
#include "DUCK_SETTING_240.h"

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
  char wifiId[32];
  char wifiPw[32];
  char city[16];
  char bus[32];
  char KR[32];
  char US[32];

  uint8_t page;
  uint8_t page_we;
  uint8_t page_kr;
  uint8_t page_us;
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
  char city[2][20];
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

  int locationNo1;
  int predictTime1;
  int locationNo2;
  int predictTime2;
} BUS;

typedef struct
{
  char code[3][9];         // 단축코드

  char name[120];          // 종목명
  char date[9];            // 기준일자
  char closePrice[12];     // 종가
  char change[10];         // 대비
  char percentChange[11];  // 등락률
} STOCK_KR;

typedef struct
{
  char name[3][30];        // 종목명

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

USER_DATA myData;
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

using namespace tinyxml2;
DynamicJsonDocument jsonDoc(2048);
char xmlDoc[10000];
/*************************** [Variable Declaration] ************************************/

/*************************** [Function Declaration] ************************************/
void clearStructData();
void getUserData(bool overwrite);
void parseUserData();

void initWiFi();
bool checkWiFiStatus();

bool getDateTime();
bool getWeather(const char* city);

bool getBusStationId(int mobileNo);
bool getStaOrder(int stationId, const char* routeName);
bool getBusArrival(const char* routeName, int stationId, int routeId, int staOrder);

bool getStockPriceKRPreviousDay(const char* code);
bool getStockPriceUSRealTime(const char* name);
/*************************** [Function Declaration] ************************************/

void setup() 
{
    // put your setup code here, to run once:
    Serial.begin(115200);

    Serial.println();
    Serial.println();
    Serial.println("-------------------------------------");
    Serial.println("----------widget_tableclock----------");
    Serial.println("-------------------------------------");
    Serial.println();
    Serial.println();

    Serial.println("-------------------------------------");
    Serial.println("[SETUP] Start]");
    Serial.println("-------------------------------------");

    touch.begin();

    tft.init();
    tft.setRotation(0);
    tft.setSwapBytes(true);
    img.setSwapBytes(true);
    tft.fillScreen(TFT_WHITE);
    img.createSprite(240, 240);

    getUserData(0);

    Serial.println("-------------------------------------");
    Serial.println("[SETUP] End");
    Serial.println("-------------------------------------");
}

uint32_t tick_cur = 0;
uint32_t tick_old[10] = {0};
void loop() 
{
    tick_cur = millis();

    // Keyboard Interrupt
    if (Serial.available()) 
    {
        char key = Serial.read();

        if (key == 'r') getUserData(1);
        // else if (key == 't') getDateTime();
        // else if (key == 'w') while(!getWeather(myWeather.city[0])) delay(1000);
        // else if (key == 'b') while(!getBusArrival(myBus.routeName, myBus.stationId, myBus.routeId, myBus.staOrder)) delay(1000);
        // else if (key == 'k') while(!getStockPriceKRPreviousDay(myStockKR.code[0])) delay(1000);
        // else if (key == 'u') while(!getStockPriceUSRealTime(myStockUS.name[0])) delay(3000);
    }

    if((tick_cur - tick_old[0]) > 500)
    {
        if(touch.available())
        {
            Serial.println("-------------------------------------");
            Serial.println("[TOUCH] Current Page");
            switch (touch.data.gestureID) 
            {
                case SWIPE_LEFT:
                    if(myData.page < 4) myData.page++;
                    Serial.println("SWIPE_LEFT");
                    break;
                case SWIPE_RIGHT:
                    if(myData.page > 0) myData.page--;
                    Serial.println("SWIPE_RIGHT");
                    break;
                case SWIPE_UP:
                    if(myData.page == 1)
                    {
                        if(myData.page_we < 1) myData.page_we++;
                    }
                    else if(myData.page == 3)
                    {
                        if(myData.page_kr < 2) myData.page_kr++;
                    }
                    else if(myData.page == 4)
                    {
                        if(myData.page_us < 2) myData.page_us++;
                    }
                    Serial.println("SWIPE_UP");
                    break;
                case SWIPE_DOWN:
                    if(myData.page == 1)
                    {
                        if(myData.page_we > 0) myData.page_we--;
                    }
                    else if(myData.page == 3) 
                    {
                        if(myData.page_kr > 0) myData.page_kr--;
                    }
                    else if(myData.page == 4) 
                    {
                        if(myData.page_us > 0) myData.page_us--;
                    }
                    Serial.println("SWIPE_DOWN");
                    break;
                default:
                    Serial.println("NONE");
                    goto ignore;
                    break;
            }

            printf("[PAGE] %d\r\n", myData.page);
            printf("[PAGE_WE] %d\r\n", myData.page_we);
            printf("[PAGE_KR] %d\r\n", myData.page_kr);
            printf("[PAGE_US] %d\r\n", myData.page_us);

            switch(myData.page)
            {
                case 0:
                    getDateTime();
                    tick_old[1] = tick_cur;
                break;
                case 1:
                    while(!getWeather(myWeather.city[myData.page_we])) delay(1000);
                    tick_old[2] = tick_cur;
                break;
                case 2:
                    while(!getBusArrival(myBus.routeName, myBus.stationId, myBus.routeId, myBus.staOrder)) delay(1000);
                    tick_old[3] = tick_cur;
                break;
                case 3:
                    while(!getStockPriceKRPreviousDay(myStockKR.code[myData.page_kr])) delay(1000);
                    tick_old[4] = tick_cur;
                break;
                case 4:
                    while(!getStockPriceUSRealTime(myStockUS.name[myData.page_us])) delay(1000);
                    tick_old[5] = tick_cur;
                break;
                default:
                    Serial.println("[ERROR] Wrong Page");
                break;
            }
ignore:
            Serial.println("-------------------------------------");
        }
        tick_old[0] = tick_cur;
    }

    if(((tick_cur - tick_old[1]) > 1000) && (myData.page == 0))
    {
        getDateTime();
        tick_old[1] = tick_cur;
    } 

    if(((tick_cur - tick_old[2]) > 30000) && (myData.page == 1))
    {
        while(!getWeather(myWeather.city[myData.page_we])) delay(1000);
        tick_old[2] = tick_cur;
    } 

    if(((tick_cur - tick_old[3]) > 15000) && (myData.page == 2))
    {
        while(!getBusArrival(myBus.routeName, myBus.stationId, myBus.routeId, myBus.staOrder)) delay(1000);
        tick_old[3] = tick_cur;
    } 

    if(((tick_cur-tick_old[4]) > (1000*60*60)) && (myData.page == 3))
    {
        while(!getStockPriceKRPreviousDay(myStockKR.code[myData.page_kr])) delay(1000);

        if(!checkWiFiStatus())
        {
            Serial.println("[ERROR] WiFi Disconnect");
            getUserData(1);
        }
        tick_old[4] = tick_cur;
    }

    if(((tick_cur-tick_old[5]) > 15000) && (myData.page == 4))
    {
        while(!getStockPriceUSRealTime(myStockUS.name[myData.page_us])) delay(1000);
        tick_old[5] = tick_cur;
    }
}

/***************************Function Definition************************************/
void clearStructData() 
{
  memset(&myData, 0, sizeof(USER_DATA));
  memset(&myTime, 0, sizeof(TIME));
  memset(&myBus, 0, sizeof(BUS));
  memset(&myWeather, 0, sizeof(WEATHER));
  memset(&myStockKR, 0, sizeof(STOCK_KR));
  memset(&myStockUS, 0, sizeof(STOCK_US));
  memset(&myData, 0, sizeof(USER_DATA));
  memset(&myData, 0, sizeof(USER_DATA));

  memset(xmlDoc, 0, sizeof(xmlDoc));
}

void getUserData(bool overwrite) 
{
    // 화면 전환
    img.pushImage(0, 0, 240, 240, DUCK_SETTING_240);
    img.pushSprite(0, 0);

    // setting...
    tft.setCursor(30, 55);  // 중앙정렬
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(3);
    tft.print("Setting...");

    char input[EEPROM_SIZE] = { 0 };
    char output[EEPROM_SIZE] = { 0 };
    uint16_t index = 0;
    uint16_t data = 0;
    bool input_flag = false;
    uint8_t err_chk_1 = 0;
    uint8_t err_chk_2 = 0;
    uint8_t param_start = 0;
    uint8_t param_end = 0;

    if (!EEPROM.begin(EEPROM_SIZE)) 
    {
        Serial.println("failed to initialise EEPROM");
        return;
    }

    if (overwrite)
        goto reset;

    EEPROM.readString(0, output, EEPROM_SIZE);  // read eeprom data
    if (output[0] == '[')
        goto parse;

reset:
    clearStructData();

    Serial.println("--------------------------------");
    Serial.println("[START] Reset User Data");
    Serial.println();

    Serial.println("Clear User Data");
    for (int i = 0; i < EEPROM_SIZE; i++) EEPROM.write(i, 0xFF);
    EEPROM.commit();
    delay(500);

    Serial.println("Enter [WiFi ID][WiFi PW][CITY1, CITY2][BUS, STATION][KR1, KR2, KR3][US1, US2, US3]");

    while (1)  // wait user data
    {
        while (Serial.available()) 
        {
            input[index] = Serial.read();
            if (input[index] == '[') err_chk_1++;
            if (input[index] == ']') err_chk_2++;

            index++;
            input_flag = true;
        }

        if (input_flag) 
        {
            if ((err_chk_1 == 6) && (err_chk_2 == 6))  // check & save user data
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
            } 
            else 
            {
                Serial.println("Input Error!");
                memset(input, '\0', 240);
                index = 0;
                err_chk_1 = 0;
                err_chk_2 = 0;
            }
            input_flag = false;
        }
    }   

    Serial.println("[END] Reset User Data");
    Serial.println("--------------------------------");

parse:
    for (int i = 0; i < EEPROM_SIZE; i++)   
    {
        if (output[i] == '[')
        {
            param_start++;
        }
        else if (output[i] == ']') 
        {
            param_end++;
            if (param_end == 6) break;
            data = 0;
        } 
        else 
        {
            if (param_start == 1) 
            {
                myData.wifiId[data] = output[i];
                data++;
            }
            if (param_start == 2) 
            {
                myData.wifiPw[data] = output[i];
                data++;
            }
            if (param_start == 3) 
            {
                myData.city[data] = output[i];
                data++;
            }
            if (param_start == 4) 
            {
                myData.bus[data] = output[i];
                data++;
            }
            if (param_start == 5) 
            {
                myData.KR[data] = output[i];
                data++;
            }
            if (param_start == 6) 
            {
                myData.US[data] = output[i];
                data++;
            }
        }
    }
    param_start = 0;

    printf("[WIFI ID] %s\r\n", myData.wifiId);
    printf("[WIFI PW] %s\r\n", myData.wifiPw);
    printf("[CITY] %s\r\n", myData.city);
    printf("[BUS] %s\r\n", myData.bus);
    printf("[KR STOCK] %s\r\n", myData.KR);
    printf("[US STOCK] %s\r\n", myData.US);

    parseUserData(); 
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    img.pushImage(0, 0, 240, 240, DUCK_HI_240);
    img.pushSprite(0, 0);

    tft.setCursor(35, 80);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(3);
    tft.print("HI");

    initWiFi();
    delay(3000);

    while(!getBusStationId(myBus.mobileNo)) delay(1000);
    while(!getStaOrder(myBus.stationId, myBus.routeName)) delay(1000);
}

void parseUserData()
{
    Serial.println("-------------------------------------");
    Serial.println("[PARSE] User Data");

    // City1, 2 파싱
    for(int i = 0, num = 0, index = 0; i < sizeof(myData.city); i++)
    {
        if(num == 0)
        {
            if(myData.city[i] == ',')
            {
                num++;
                index = 0;
                continue;
            }
            myWeather.city[num][index++] = myData.city[i];
        }
        else if(num == 1)
        {
            if(myData.city[i] == '\0')
            {
                break;
            }
            myWeather.city[num][index++] = myData.city[i];
        }
    }
    printf("[CITY1] %s\r\n", myWeather.city[0]);
    printf("[CITY2] %s\r\n", myWeather.city[1]);

    // 정류장, 버스 파싱
    char mobileNoBuf[10];
    for(int i = 0, num = 0, index = 0; i < sizeof(myData.bus); i++)
    {
        if(num == 0)
        {
            if(myData.bus[i] == ',')
            {
                num++;
                index = 0;
                continue;
            }
            myBus.routeName[index++] = myData.bus[i];
        }
        else if(num == 1)
        {
            if(myData.bus[i] == '\0')
            {
                break;
            }
            mobileNoBuf[index++] = myData.bus[i];
        }
    }
    myBus.mobileNo = atoi(mobileNoBuf);
    printf("[BUS] %s\r\n", myBus.routeName);
    printf("[STATION] %d\r\n", myBus.mobileNo);

    // 국내 주식 파싱
    for(int i = 0, num = 0, index = 0; i < sizeof(myData.KR); i++)
    {
        if(num == 0)
        {
            if(myData.KR[i] == ',')
            {
                num++;
                index = 0;
                continue;
            }
            myStockKR.code[num][index++] = myData.KR[i];
        }
        else if(num == 1)
        {
            if(myData.KR[i] == ',')
            {
                num++;
                index = 0;
                continue;
            }
            myStockKR.code[num][index++] = myData.KR[i];
        }
        else if(num == 2)
        {
            if(myData.KR[i] == '\0')
            {
                break;
            }
            myStockKR.code[num][index++] = myData.KR[i];
        }
    }
    printf("[KR1] %s\r\n", myStockKR.code[0]);
    printf("[KR2] %s\r\n", myStockKR.code[1]);
    printf("[KR3] %s\r\n", myStockKR.code[2]);

    // 해외 주식 파싱
    for(int i = 0, num = 0, index = 0; i < sizeof(myData.US); i++)
    {
        if(num == 0)
        {
            if(myData.US[i] == ',')
            {
                myStockUS.name[num][index++] = '\0';
                num++;
                index = 0;
                continue;
            }
            myStockUS.name[num][index++] = myData.US[i];
        }
        else if(num == 1)
        {
            if(myData.US[i] == ',')
            {
                 myStockUS.name[num][index++] = '\0';
                num++;
                index = 0;
                continue;
            }
            myStockUS.name[num][index++] = myData.US[i];
        }
        else if(num == 2)
        {
            if(myData.US[i] == '\0')
            {
                myStockUS.name[num][index++] = '\0';
                break;
            }
            myStockUS.name[num][index++] = myData.US[i];
        }
    }
    printf("[US1] %s\r\n", myStockUS.name[0]);
    printf("[US2] %s\r\n", myStockUS.name[1]);
    printf("[US3] %s\r\n", myStockUS.name[2]);

    Serial.println("-------------------------------------");
}

void initWiFi() {
  uint8_t wifi_timeout = 0;
  Serial.println("WIFI Connecting");
  WiFi.begin(myData.wifiId, myData.wifiPw);

  if (checkWiFiStatus())
    Serial.println("WIFI CONNECTED");
  else {
    Serial.println();
    Serial.println("WiFi Not Connected, Check [WiFi ID][WiFi PW]");
    getUserData(1);
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

bool getDateTime() 
{
    Serial.println("-------------------------------------");
    Serial.println("[REQUEST] Date/Time");

    // 화면 전환
    img.pushImage(0, 0, 240, 240, DUCK_TIME_240);
    img.pushSprite(0, 0);

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println("[ERROR]");
        Serial.println("-------------------------------------");
        return false;
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

    // 월/일 시간:분
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
    return true;
}

bool getWeather(const char* city) 
{
    Serial.println("-------------------------------------");
    Serial.println("[REQUEST] Weather");

    // 화면 전환
    img.pushImage(0, 0, 240, 240, DUCK_WEATHER_240);
    img.pushSprite(0, 0);

    WiFiClient mySocket;
    HTTPClient myHTTP;

    int httpCode;
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "http://api.openweathermap.org/data/2.5/weather?q=%s&appid=%s", city, OPENWEATHER_API_KEY);
    myHTTP.begin(mySocket, buffer);
    delay(500);

    httpCode = myHTTP.GET();
    printf("[HTTP CODE] %d \r\n", httpCode);

    if (httpCode == HTTP_CODE_OK) 
    {
        Serial.println("[OK]");
        deserializeJson(jsonDoc, myHTTP.getString());
    } 
    else 
    {
        Serial.println("[ERROR]");
        myHTTP.end();
        Serial.println("-------------------------------------");
        return false;
    }
    myHTTP.end();

    const char* weather = jsonDoc["weather"][0]["main"];
    strcpy(myWeather.weather, weather);
    myWeather.temp = (int)(jsonDoc["main"]["temp"]) - 273.0;

    printf("[도시] %s \r\n", city);
    printf("[날씨] %s \r\n", myWeather.weather);
    printf("[기온] %d \r\n", myWeather.temp);

    // 도시
    tft.setCursor(60, 40);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(3);
    tft.print(city);

    // 날씨
    tft.setCursor(60, 70);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(3);
    tft.print(myWeather.weather);

    // 기온
    tft.setCursor(60, 100);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(3);
    tft.print(myWeather.temp);
    tft.print("C");

    Serial.println("-------------------------------------");
    return true;
}

bool getBusStationId(int mobileNo) 
{
    Serial.println("-------------------------------------");
    Serial.println("[REQUEST] Bus Station ID");

    WiFiClient mySocket;
    HTTPClient myHTTP;

    int httpCode;
    char buffer[300];
    myBus.mobileNo = mobileNo;

    snprintf(buffer, sizeof(buffer), "http://apis.data.go.kr/6410000/busstationservice/getBusStationList?serviceKey=%s&keyword=%d",PUBLIC_DATA_API_KEY, myBus.mobileNo);
    myHTTP.begin(mySocket, buffer);
    delay(500);

    httpCode = myHTTP.GET();
    printf("[HTTP CODE] %d \r\n", httpCode);

    if (httpCode == HTTP_CODE_OK) 
    {
        Serial.println("[OK]");
        strcpy(xmlDoc, myHTTP.getString().c_str());
    } 
    else 
    {
        Serial.println("[ERROR]");
        myHTTP.end();
        Serial.println("-------------------------------------");
        return false;
    }
    myHTTP.end();

    XMLDocument xmlDocument;
    if (xmlDocument.Parse(xmlDoc) != XML_SUCCESS) 
    {
        Serial.println("[PARSE ERROR]");
        myHTTP.end();
        Serial.println("-------------------------------------");
        return false;
    };

    XMLNode* root = xmlDocument.RootElement();

    // 정류소아이디
    XMLElement* element = root->FirstChildElement("msgBody")->FirstChildElement("busStationList")->FirstChildElement("stationId");
    element->QueryIntText(&myBus.stationId);
    printf("[정류소아이디] %d\r\n", myBus.stationId);

    // 정류소명
    element = root->FirstChildElement("msgBody")->FirstChildElement("busStationList")->FirstChildElement("stationName");
    strcpy(myBus.stationName, element->GetText());
    printf("[정류소명] %s\r\n", myBus.stationName);

    Serial.println("-------------------------------------");
    return true;
}

bool getStaOrder(int stationId, const char* routeName) 
{
    Serial.println("-------------------------------------");
    Serial.println("[REQUEST] Bus StaOrder");

    WiFiClient mySocket;
    HTTPClient myHTTP;

    int httpCode;
    char buffer[300];

    snprintf(buffer, sizeof(buffer), "http://apis.data.go.kr/6410000/busstationservice/getBusStationViaRouteList?serviceKey=%s&stationId=%d", PUBLIC_DATA_API_KEY, stationId);
    myHTTP.begin(mySocket, buffer);
    delay(500);

    if (myHTTP.GET() == HTTP_CODE_OK) {
        Serial.println("[OK]");
        strcpy(xmlDoc, myHTTP.getString().c_str());
    } 
    else 
    {
        Serial.println("[ERROR]");
        myHTTP.end();
        Serial.println("-------------------------------------");
        return false;
    }
    myHTTP.end();

    XMLDocument xmlDocument;
    if (xmlDocument.Parse(xmlDoc) != XML_SUCCESS) 
    {
        Serial.println("[PARSE ERROR]");
        myHTTP.end();
        Serial.println("-------------------------------------");
        return false;
  };

    XMLNode* root = xmlDocument.RootElement();
    XMLElement* element = root->FirstChildElement("msgBody")->FirstChildElement("busRouteList");

    char routeNameBuf[30];
    for (XMLElement* routeList = element; routeList != NULL; routeList = routeList->NextSiblingElement()) 
    {
        strcpy(routeNameBuf, routeList->FirstChildElement("routeName")->GetText());
        printf("%s\r\n", routeNameBuf);

        if (strcmp(routeNameBuf, routeName) == 0) 
        {
            routeList->FirstChildElement("routeId")->QueryIntText(&myBus.routeId);
            routeList->FirstChildElement("staOrder")->QueryIntText(&myBus.staOrder);
            break;
        }
    }

    printf("[노선아이디] %d\r\n", myBus.routeId);
    printf("[정류소순번] %d\r\n", myBus.staOrder);

    Serial.println("-------------------------------------");
    return true;
}

bool getBusArrival(const char* routeName, int stationId, int routeId, int staOrder)  // getBusArrivalItem Operation
{
    Serial.println("-------------------------------------");
    Serial.println("[REQUEST] BUS Arrival");

    // 화면 전환
    img.pushImage(0, 0, 240, 240, DUCK_BUS_240);
    img.pushSprite(0, 0);

    WiFiClient mySocket;
    HTTPClient myHTTP;

    int httpCode;
    char buffer[300];
    memset(xmlDoc, 0, sizeof(xmlDoc));

    snprintf(buffer, sizeof(buffer), "http://apis.data.go.kr/6410000/busarrivalservice/getBusArrivalItem?serviceKey=%s&stationId=%d&routeId=%d&staOrder=%d", PUBLIC_DATA_API_KEY, stationId, routeId, staOrder);
    myHTTP.begin(mySocket, buffer);
    delay(500);

    httpCode = myHTTP.GET();
    printf("[HTTP CODE] %d \r\n", httpCode);

    if (httpCode == HTTP_CODE_OK) 
    {
        Serial.println("[OK]");
        strcpy(xmlDoc, myHTTP.getString().c_str());
    } 
    else 
    {
        Serial.println("[ERROR]");
        myHTTP.end();
        Serial.println("-------------------------------------");
        return false;
    }
    myHTTP.end();

    XMLDocument xmlDocument;
    if (xmlDocument.Parse(xmlDoc) != XML_SUCCESS) 
    {
        Serial.println("[PARSE ERROR]");
        Serial.println("-------------------------------------");
        return false;
    };

    XMLNode* root = xmlDocument.RootElement();

    // 첫번째차량 위치정보
    XMLElement* element = root->FirstChildElement("msgBody")->FirstChildElement("busArrivalItem")->FirstChildElement("locationNo1");
    element->QueryIntText(&myBus.locationNo1);
    printf("[첫번째버스 위치정보] %d번째전 정류소\r\n", myBus.locationNo1);

    // 첫번째차량 도착예상시간
    element = root->FirstChildElement("msgBody")->FirstChildElement("busArrivalItem")->FirstChildElement("predictTime1");
    element->QueryIntText(&myBus.predictTime1);
    printf("[첫번째버스 도착예상시간] %d분후\r\n", myBus.predictTime1);

    // 정류소명
    String staStr(myBus.stationName);
    int nameLen = strlen(myBus.stationName) / 3;
    AimHangul_v2(120 - (float)(nameLen/2) * 16, 50, staStr, TFT_WHITE);  // 중앙정렬

    // 버스와 도착 시간
    char preBuf[20];
    snprintf(preBuf, sizeof(preBuf), "%s  %d분", myBus.routeName, myBus.predictTime1);
    String preStr(preBuf);
    AimHangul_v2(35, 100, preStr, TFT_WHITE);  // 중앙정렬

    Serial.println("-------------------------------------");
    return true;
}

bool getStockPriceKRPreviousDay(const char* code)  // 한국주식 전날 시세
{
    Serial.println("-------------------------------------");
    Serial.println("[REQUEST] KR stock");

    // 화면 전환
    img.pushImage(0, 0, 240, 240, DUCK_STOCK_KR_240);
    img.pushSprite(0, 0);

    WiFiClient mySocket;
    HTTPClient myHTTP;

    int httpCode;
    char buffer[300];

    snprintf(buffer, sizeof(buffer), "http://apis.data.go.kr/1160100/service/GetStockSecuritiesInfoService/getStockPriceInfo?serviceKey=%s&numOfRows=1&pageNo=1&likeSrtnCd=%s", PUBLIC_DATA_API_KEY, code);
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
        return false;
    }
    myHTTP.end();

    XMLDocument xmlDocument;
    if (xmlDocument.Parse(xmlDoc) != XML_SUCCESS) {
        Serial.println("[PARSE ERROR]");
        Serial.println("-------------------------------------");
        return false;
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

    // 날짜
    tft.setCursor(120-(12*4), 30); // 중앙정렬
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    tft.printf("%s", myStockKR.date);

    // 종목
    String str(myStockKR.name);
    int nameLen = strlen(myStockKR.name) / 3;
    // printf("name len %d\r\n", nameLen);
    AimHangul_v2(120 - (float)(nameLen/2) * 16, 50, str, TFT_WHITE);  // 중앙정렬

    // 종가
    int closePriceLen = strlen(myStockKR.closePrice);
    // printf("closePriceLen %d\r\n", closePriceLen);
    tft.setCursor(120 - (float)(closePriceLen/2) * 20, 50 + 36);  // 중앙정렬
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(3);
    tft.print(myStockKR.closePrice);
    // printf("%d\r\n", tft.getCursorX());

    // 대비, 등락률
    char changeBuffer[50];
    snprintf(changeBuffer, sizeof(changeBuffer), "%s %s%%", myStockKR.change, myStockKR.percentChange);
    int changeBufferLen = strlen(changeBuffer);
    // printf("changeBufferLen %d\r\n", changeBufferLen);
    tft.setCursor(120 - (float)(changeBufferLen/2) * 12, 50 + 36 + 30);  // 중앙정렬
    if (myStockKR.change[0] == '-') tft.setTextColor(TFT_BLUE, TFT_WHITE, 1);
    else tft.setTextColor(TFT_RED, TFT_WHITE, 1);
    tft.setTextSize(2);
    tft.print(changeBuffer);

    Serial.println("-------------------------------------");
    return true;
}

bool getStockPriceUSRealTime(const char* name)  // 미국주식 실시간 시세
{
    Serial.println("-------------------------------------");
    Serial.println("[REQUEST] US Stock");
 
    // 화면 전환
    img.pushImage(0, 0, 240, 240, DUCK_STOCK_US_240);
    img.pushSprite(0, 0);

    WiFiClientSecure* client = new WiFiClientSecure;
    HTTPClient myHTTP;

    int httpCode;
    char buffer[300];
    snprintf(buffer, sizeof(buffer), "https://finnhub.io/api/v1/quote?symbol=%s&token=%s", name, FINNHUB_STOCK_API_KEY);
    client->setInsecure();
    myHTTP.begin(*client, buffer);
    delay(500);

    httpCode = myHTTP.GET();
    printf("[HTTP CODE] %d \r\n", httpCode);

    if (httpCode == HTTP_CODE_OK) 
    {
        Serial.println("[OK]");
        deserializeJson(jsonDoc, myHTTP.getString());
    } 
    else 
    {
        Serial.println("[ERROR]");
        myHTTP.end();
        Serial.println("-------------------------------------");
        return false;
    }
    myHTTP.end();

    myStockUS.date = (int)(jsonDoc["t"]);
    myStockUS.currentPrice = (float)(jsonDoc["c"]);
    myStockUS.change = (float)(jsonDoc["d"]);
    myStockUS.percentChange = (float)(jsonDoc["dp"]);

    printf("[종목명] %s \r\n", name);
    printf("[기준일자] %d \r\n", myStockUS.date);
    printf("[현재가] %.2f \r\n", myStockUS.currentPrice);
    printf("[대비] %.2f \r\n", myStockUS.change);
    printf("[등락률] %.2f \r\n", myStockUS.percentChange);

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
    int nameLen = strlen(name);
    tft.setCursor(120 - (float)(nameLen/2) * 18, 55);  // 중앙정렬
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(3);
    tft.print(name);

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
    return true;
}
/***************************Function Definition************************************/