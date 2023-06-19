#include <U8g2lib.h>
//U8G2_SSD1306_64X48_ER_F_HW_I2C u8g2(U8G2_DIR); // WEMOS D1 mini shield
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0);

#include <AimHangul.h>

void setup() {
  Serial.begin(115200);
  Serial.println();

  u8g2.begin();
}

void loop() {

  static unsigned long nextMil = millis() + 5000;
  static int cnt = 0;

  if (millis() > nextMil) {
    nextMil = millis() + 5000;
    cnt++;
    String msg = "안녕하셔요";
    u8g2.firstPage();
    do{
      switch(cnt) {
        case 1 : AimHangul   (1,0,msg); break;
        case 2 : AimHangul_h2(1,0,msg); break;
        case 3 : AimHangul_v2(1,0,msg); break;
        case 4 : AimHangul_x4(1,0,msg); break;
      }
    } while(u8g2.nextPage());
    if (cnt == 4) {
      cnt = 0;
    }
  }    
}