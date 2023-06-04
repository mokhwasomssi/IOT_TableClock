#include <string.h>
#include <WiFi.h>
#include "EEPROM.h"

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

TABLECLOCK_STATE my_state = WIDGET_NULL;
USER_DATA my_data;

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
}

void loop() {
  // put your main code here, to run repeatedly:
    if(Serial.available())
    {
        char reset = Serial.read();

        if(reset == 'r')
            resetUserData(1);
    }

    if(!checkWiFiStatus())
    {
        Serial.println("WiFi Disconnected, Check WiFi Signal");
        resetUserData(1);
    }


    delay(10);

    // switch(my_state)
    // {
    //     case WIDGET_NULL:
    //         neopixelWrite(RGB_BUILTIN,0xFF,0x00,0x00); // red 

    //         break;
    //     case WIDGET_STOP:
    //         neopixelWrite(RGB_BUILTIN,0xFF,0xFF,0x00); // yellow

    //         break;
    //     case WIDGET_GO:
    //         neopixelWrite(RGB_BUILTIN,0x00,0xFF,0x00); // green

    //         break;

    //     defalut:

    //         break;
    // }
}
