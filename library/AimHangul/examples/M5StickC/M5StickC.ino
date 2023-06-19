#include <M5StickC.h>
#include <AimHangul.h>

void setup() {
  M5.begin();
  M5.Lcd.fillScreen(BLACK);
}

void loop() {
 
  static unsigned long nextMil = millis() + 5000;
  static int cnt = 0;

  if (millis() > nextMil) {
    nextMil = millis() + 5000;
    cnt++;
    String msg = "안녕하셔요";
    M5.Lcd.setRotation(1);
    M5.Lcd.fillScreen(BLACK);
    switch(cnt) {
      case 1 : AimHangul   (0,0,msg,WHITE); break;
      case 2 : AimHangul_h2(0,0,msg,GREEN); break;
      case 3 : AimHangul_v2(0,0,msg,PINK);  break;
      case 4 : AimHangul_x4(0,0,msg,YELLOW);break;
    }
    if (cnt == 4) {
      cnt = 0;
    }
  }   
}