#include <TFT_eSPI.h>        // Hardware-specific library
TFT_eSPI tft = TFT_eSPI();   // Invoke library
#include <AimHangul.h>

void setup() {
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(BLACK);
  String msg ="반갑습니다.Abc" ;
  AimHangul   (0,20,         msg,WHITE);  
  AimHangul_h2(0,20+20,      msg,GREEN);
  AimHangul_v2(0,20+20+35,   msg,RED);
  AimHangul_x4(0,20+20+35+40,msg,YELLOW);  
}

void loop() {
 
}