#include <M5Stack.h>
#include <AimHangul.h>

void setup() {
  M5.begin();
  M5.Lcd.fillScreen(BLACK);
  String msg ="반갑습니다.Abc" ;
  AimHangul   (0,20,         msg,WHITE);  
  AimHangul_h2(0,20+20,      msg,GREEN);
  AimHangul_v2(0,20+20+35,   msg,RED);
  AimHangul_x4(0,20+20+35+40,msg,YELLOW);  
}

void loop() {
 
}