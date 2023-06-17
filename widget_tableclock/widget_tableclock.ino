#include <string.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "EEPROM.h"
#include "time.h"
#include <tinyxml2.h>

// 공공데이터포털 개인 API 인증키
#define PUBLIC_DATA_API_KEY "HHec4G%2FFjjMjQIfzZa3yfZuItK93BQh%2BC%2FwkITl%2FCu8X3h%2BAjlF74glKicSnEN%2BVeZEOvstt07Zz%2Be%2BvmAlFVQ%3D%3D" 

// OpenWeather API 키
#define OPENWEATHER_API_KET "8d27c680ce67b6ffebcf09d005cdd444"

#define EEPROM_SIZE 256

typedef enum
{
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
    int busStationName;
    int stationId;
    int routeName;
    int routeId;
    int staOrder;

    int routeName_temp;

    int locationNo1;
    int predictTime1;
    int locationNo2;
    int predictTime2;
} BUS;

TABLECLOCK_STATE my_state = WIDGET_NULL;
USER_DATA my_data;
BUS my_bus;

// NTP server to request epoch time
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 32400; // +9:00 (9hours*60mins*60seconds)
const int daylightOffset_sec = 0;

wifi_mode_t esp_wifi_mode = WIFI_MODE_NULL;
WiFiClient mySocket;
HTTPClient myHTTP;
DynamicJsonDocument doc(2048);

using namespace tinyxml2;
// char * xmlDoc = "<root><body><element>7</element><element2>8</element2>></body></root>";
// char * xmlDoc = "<response><comMsgHeader/><msgHeader><queryTime>2023-06-18 01:25:40.486</queryTime><resultCode>0</resultCode><resultMessage>정상적으로 처리되었습니다.</resultMessage></msgHeader><msgBody><busStationList><centerYn>N</centerYn><stationId>228000162</stationId></busStationList></msgBody></response>";

char xmlDoc[4096];

void getTime()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%a, %b %d %Y %H:%M:%S");
}

void getWeather()
{
    myHTTP.begin(mySocket, "http://api.openweathermap.org/data/2.5/weather?q=Yongin&appid=8d27c680ce67b6ffebcf09d005cdd444");

    if(myHTTP.GET() == HTTP_CODE_OK)
    {
    printf("weather Received! \r\n");
    deserializeJson(doc, myHTTP.getString());
    const char *cityStr = doc["name"];
    const char *currWeather = doc["weather"][0]["main"];
    float temp = (float)(doc["main"]["temp"]) - 273.0;
    printf("%s의 날씨 : %s, 온도 : %f \r\n", cityStr, currWeather, temp);
    }
    else
    {
        printf("Server error! \r\n");
    }
    myHTTP.end();
}

void getBusStationId() // getBusStationList Operation
{
    char buffer[512];
    snprintf(buffer, 512, "http://apis.data.go.kr/6410000/busstationservice/getBusStationList?serviceKey=%s&keyword=%d", PUBLIC_DATA_API_KEY, my_bus.busStationName);

    myHTTP.begin(mySocket, buffer);

    if(myHTTP.GET() == HTTP_CODE_OK)
    {
        printf("BusStationListResponse \r\n");
        strcpy(xmlDoc, myHTTP.getString().c_str());
        printf("%s \r\n", xmlDoc);
    }
    else
    {
        printf("BusStationListResponse error! \r\n");
    }

    XMLDocument xmlDocument;
    if(xmlDocument.Parse(xmlDoc)!= XML_SUCCESS)
    {
        Serial.println("BusStationListResponse error parsing");  
        return;
    };

    XMLNode * root = xmlDocument.RootElement();
    XMLElement * element = root->FirstChildElement("msgBody")->FirstChildElement("busStationList")->FirstChildElement("stationId");

    element->QueryIntText(&my_bus.stationId);	
    Serial.print("BusStationId : ");
    Serial.println(my_bus.stationId);

    myHTTP.end();
}

void getStaOrder() // getBusStationViaRouteList Operation, Route : 노선 번호(=버스 번호)
{
    char buffer[512];
    snprintf(buffer, 512, "http://apis.data.go.kr/6410000/busstationservice/getBusStationViaRouteList?serviceKey=%s&stationId=%d", PUBLIC_DATA_API_KEY, my_bus.stationId);

    myHTTP.begin(mySocket, buffer);

    if(myHTTP.GET() == HTTP_CODE_OK)
    {
        printf("getBusStationViaRouteList \r\n");
        strcpy(xmlDoc, myHTTP.getString().c_str());
        printf("%s \r\n", xmlDoc);
    }
    else
    {
        printf("getBusStationViaRouteList error! \r\n");
    }

    XMLDocument xmlDocument;
    if(xmlDocument.Parse(xmlDoc)!= XML_SUCCESS)
    {
        Serial.println("getBusStationViaRouteList error parsing");  
        return;
    };

    XMLNode* root = xmlDocument.RootElement();
    XMLElement* element = root->FirstChildElement("msgBody")->FirstChildElement("busStationList")->FirstChildElement("routeName");

    for(XMLElement* routeName = element; routeName != NULL; routeName->NextSiblingElement())
    {
        routeName->QueryIntText(&my_bus.routeName_temp);

        if(my_bus.routeName_temp == my_bus.routeName)
        {
            XMLElement* ele1 = root->FirstChildElement("msgBody")->FirstChildElement("busStationList")->FirstChildElement("routeId");
            ele1->QueryIntText(&my_bus.routeId);

            XMLElement* ele2 = root->FirstChildElement("msgBody")->FirstChildElement("busStationList")->FirstChildElement("staOrder");
            ele2->QueryIntText(&my_bus.staOrder);
            
            break;
        }
    }

    Serial.print("Bus routeId : ");
    Serial.println(my_bus.routeId);
    Serial.print("Bus staOrder : ");
    Serial.println(my_bus.staOrder);

    myHTTP.end();
}

void getBusArrival() // getBusArrivalItem Operation
{
    char buffer[512];
    snprintf(buffer, 512, "http://apis.data.go.kr/6410000/busstationservice/getBusArrivalItem?serviceKey=%s&stationId=%d&routeId=%d&staOrder=%d", PUBLIC_DATA_API_KEY, my_bus.stationId, my_bus.routeId, my_bus.staOrder);

    myHTTP.begin(mySocket, buffer);

    if(myHTTP.GET() == HTTP_CODE_OK)
    {
        printf("getBusArrivalItem \r\n");
        strcpy(xmlDoc, myHTTP.getString().c_str());
        printf("%s \r\n", xmlDoc);
    }
    else
    {
        printf("getBusArrivalItem error! \r\n");
    }

    XMLDocument xmlDocument;
    if(xmlDocument.Parse(xmlDoc)!= XML_SUCCESS)
    {
        Serial.println("getBusArrivalItem error parsing");  
        return;
    };

    XMLNode * root = xmlDocument.RootElement();
    XMLElement * element1 = root->FirstChildElement("msgHeader")->FirstChildElement("busArrivalItem")->FirstChildElement("locationNo1");
    XMLElement * element2 = root->FirstChildElement("msgHeader")->FirstChildElement("busArrivalItem")->FirstChildElement("predictTime1");
    XMLElement * element3 = root->FirstChildElement("msgHeader")->FirstChildElement("busArrivalItem")->FirstChildElement("locationNo2");
    XMLElement * element4 = root->FirstChildElement("msgHeader")->FirstChildElement("busArrivalItem")->FirstChildElement("predictTime2");

    element1->QueryIntText(&my_bus.locationNo1);	
    element2->QueryIntText(&my_bus.predictTime1);	
    element3->QueryIntText(&my_bus.locationNo2);	
    element4->QueryIntText(&my_bus.predictTime2);	

    Serial.println(my_bus.locationNo1);
    Serial.println(my_bus.predictTime1);
    Serial.println(my_bus.locationNo2);
    Serial.println(my_bus.predictTime2);

    myHTTP.end();
}


void clearUserData()
{
    memset(my_data.wifi_id, '\0', 32);
    memset(my_data.wifi_pw, '\0', 32);
    memset(my_data.bus_id, '\0', 32);
    memset(my_data.stock_num, '\0', 32);
}

void resetUserData(bool overwrite)
{
    neopixelWrite(RGB_BUILTIN,0xFF,0xFF,0x00); // yellow

    char input[EEPROM_SIZE] = {0};
    char output[EEPROM_SIZE] = {0};
    uint16_t index = 0;
    uint16_t data = 0;
    bool input_flag = false;
    uint8_t err_chk_1 = 0;
    uint8_t err_chk_2 = 0;
    uint8_t param_start = 0;
    uint8_t param_end = 0;

    if(!EEPROM.begin(EEPROM_SIZE))
    {
        Serial.println("failed to initialise EEPROM");
        return;
    }

    if(overwrite)
        goto reset;

    EEPROM.readString(0, output, EEPROM_SIZE); // read eeprom data
    if(output[0] == '[')
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

    while(1) // wait user data
    {
        while(Serial.available())
        {
            input[index] = Serial.read();
            if(input[index] == '[') err_chk_1++;
            if(input[index] == ']') err_chk_2++;
            
            index++;
            input_flag = true;
        }

        if(input_flag)
        {
            if((err_chk_1 == 4) && (err_chk_2 == 4)) // check & save user data
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

    Serial.println("--------------------------------");
    Serial.println("Reset User Data END");
    Serial.println("--------------------------------");

parse:
    for(int i = 0; i < EEPROM_SIZE; i++)
    {
        if(output[i] == '[')
        {
            param_start++;
        }
        else if(output[i] == ']')
        {
            param_end++;
            if(param_end == 4) break;
            data = 0;
        }
        else
        {
            if(param_start == 1)
            {
                my_data.wifi_id[data] = output[i];
                data++;
            }
            if(param_start == 2)
            {
                my_data.wifi_pw[data] = output[i];
                data++;
            }
            if(param_start == 3)
            {
                my_data.bus_id[data] = output[i];
                data++;
            }

            if(param_start == 4)
            {
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

    neopixelWrite(RGB_BUILTIN,0x00,0xFF,0x00); // green
}

void initWiFi()
{
    uint8_t wifi_timeout = 0;
    Serial.println("WIFI Connecting");
    WiFi.begin(my_data.wifi_id,my_data.wifi_pw);
    
    if(checkWiFiStatus())
        Serial.println("WIFI CONNECTED");
    else
    {
        Serial.println();
        Serial.println("WiFi Not Connected, Check [WiFi ID][WiFi PW]");
        resetUserData(1);
    }
}

bool checkWiFiStatus()
{
    uint8_t wifi_timeout = 0;
    while(WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        wifi_timeout++;
        Serial.print(".");
        if(wifi_timeout == 20)
        {
            return 0;
        }
    }
    return 1;
}


void setup() {
  // put your setup code here, to run once:
    Serial.begin(115200);
    Serial.println();
    Serial.println();
    Serial.println("-------------------------------------");
    Serial.println("----------widget_tableclock----------");
    Serial.println("-------------------------------------");
    Serial.println();
    Serial.println();

    resetUserData(0);

    // Init and get the time
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    getBusStationId();
    getStaOrder();
}


void loop() {
  // put your main code here, to run repeatedly:
    if(Serial.available())
    {
        char key = Serial.read();

        if(key == 'r')
            resetUserData(1);
        else if(key == 't')
            getTime();
        else if(key == 'w')
            getWeather();
        else if(key == 'b')
            getBusArrival();
    }

    if(!checkWiFiStatus())
    {
        Serial.println("WiFi Disconnected, Check WiFi Signal");
        resetUserData(1);
    }
}
