#ifndef AIM_HANGUL_MS_H
#define AIM_HANGUL_MS_H

#include "ASCFont.h"
#include "KSFont.h"

#if defined(__AVR__)
#include <avr/pgmspace.h> 
#endif
/*
#if defined(_M5STACK_H_)
#define TFT_WIDTH     320
#define TFT_HEIGHT    240
#elif defined(_M5STICKC_H_)
#define TFT_WIDTH     80
#define TFT_HEIGHT    160
#endif 
*/
#define HG_SIZE_NORMAL 0
#define HG_SIZE_H2     2
#define HG_SIZE_V2     3
#define HG_SIZE_X4     4

/*=============================================================================
      한글 font 처리부분 
  =============================================================================*/

byte *getHAN_font(byte HAN1, byte HAN2, byte HAN3, byte* pHgImg){

  const byte cho[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 3, 3, 1, 2, 4, 4, 4, 2, 1, 3, 0 };
  const byte cho2[] = { 0, 5, 5, 5, 5, 5, 5, 5, 5, 6, 7, 7, 7, 6, 6, 7, 7, 7, 6, 6, 7, 5 };
  const byte jong[] = { 0, 0, 2, 0, 2, 1, 2, 1, 2, 3, 0, 2, 1, 3, 3, 1, 2, 1, 3, 3, 1, 1 };

  uint16_t utf16;
  byte first, mid, last;
  byte firstType, midType, lastType;
  byte i;
  byte *pB, *pF;

  /*------------------------------
    UTF-8 을 UTF-16으로 변환한다.

    UTF-8 1110xxxx 10xxxxxx 10xxxxxx
  */------------------------------
  utf16 = (HAN1 & 0x0f) << 12 | (HAN2 & 0x3f) << 6 | HAN3 & 0x3f;

  /*------------------------------
    초,중,종성 코드를 분리해 낸다.

    unicode = {[(초성 * 21) + 중성] * 28}+ 종성 + 0xAC00

          0   1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 
    초성 ㄱ   ㄲ ㄴ ㄷ ㄸ ㄹ ㅁ ㅂ ㅃ ㅅ ㅆ ㅇ ㅈ ㅉ ㅊ ㅋ ㅌ ㅍ ㅎ
    중성 ㅏ   ㅐ ㅑ ㅒ ㅓ ㅔ ㅕ ㅖ ㅗ ㅘ ㅙ ㅚ ㅛ ㅜ ㅝ ㅞ ㅟ ㅠ ㅡ ㅢ ㅣ  
    종성 없음 ㄱ ㄲ ㄳ ㄴ ㄵ ㄶ ㄷ ㄹ ㄺ ㄻ ㄼ ㄽ ㄾ ㄿ ㅀ ㅁ ㅂ ㅄ ㅅ ㅆ ㅇ ㅈ ㅊ ㅋ ㅌ ㅍ ㅎ
  ------------------------------*/
  utf16 -= 0xac00;
  last = utf16 % 28;
  utf16 /= 28;
  mid = utf16 % 21;
  first = utf16 / 21;

  first++;
  mid++;

  /*------------------------------
    초,중,종성 해당 폰트 타입(벌)을 결정한다.
  ------------------------------*/

  /*
   초성 19자:ㄱㄲㄴㄷㄸㄹㅁㅂㅃㅅㅆㅇㅈㅉㅊㅋㅌㅍㅎ
   중성 21자:ㅏㅐㅑㅒㅓㅔㅕㅖㅗㅘㅙㅚㅛㅜㅝㅞㅟㅠㅡㅢㅣ
   종성 27자:ㄱㄲㄳㄴㄵㄶㄷㄹㄺㄻㄼㄽㄾㄿㅀㅁㅂㅄㅆㅇㅈㅊㅋㅌㅍㅎ

   초성
      초성 1벌 : 받침없는 'ㅏㅐㅑㅒㅓㅔㅕㅖㅣ' 와 결합
      초성 2벌 : 받침없는 'ㅗㅛㅡ'
      초성 3벌 : 받침없는 'ㅜㅠ'
      초성 4벌 : 받침없는 'ㅘㅙㅚㅢ'
      초성 5벌 : 받침없는 'ㅝㅞㅟ'
      초성 6벌 : 받침있는 'ㅏㅐㅑㅒㅓㅔㅕㅖㅣ' 와 결합
      초성 7벌 : 받침있는 'ㅗㅛㅜㅠㅡ'
      초성 8벌 : 받침있는 'ㅘㅙㅚㅢㅝㅞㅟ'

   중성
      중성 1벌 : 받침없는 'ㄱㅋ' 와 결합
      중성 2벌 : 받침없는 'ㄱㅋ' 이외의 자음
      중성 3벌 : 받침있는 'ㄱㅋ' 와 결합
      중성 4벌 : 받침있는 'ㄱㅋ' 이외의 자음

   종성
      종성 1벌 : 중성 'ㅏㅑㅘ' 와 결합
      종성 2벌 : 중성 'ㅓㅕㅚㅝㅟㅢㅣ'
      종성 3벌 : 중성 'ㅐㅒㅔㅖㅙㅞ'
      종성 4벌 : 중성 'ㅗㅛㅜㅠㅡ'

  */
  if(!last){  //받침 없는 경우
    firstType = cho[mid];
    if(first == 1 || first == 24) midType = 0;
    else midType = 1;
  }
  else{       //받침 있는 경우
    firstType = cho2[mid];
    if(first == 1 || first == 24) midType = 2;
    else midType = 3;
    lastType = jong[mid];
  }
  //memset(pHgImg, 0, 32);

  //초성 
  pB = pHgImg;
  pF = (byte*)KSFont + (firstType*20 + first)*32;
  i = 32;
  #if defined(__AVR__)
  while(i--) *pB++ = pgm_read_byte_near(pF++); 
  #else
  while(i--) *pB++ = pgm_read_byte(pF++); 
  #endif
  

  //중성
  pB = pHgImg;
  pF = (byte*)KSFont + (8*20 + midType*22 + mid)*32;
  i = 32;
  #if defined(__AVR__)
  while(i--) *pB++ |= pgm_read_byte_near(pF++); 
  #else
  while(i--) *pB++ |= pgm_read_byte(pF++); 
  #endif
  

  //종성
  if(last){
    pB = pHgImg;
    pF = (byte*)KSFont + (8*20 + 4*22 + lastType*28 + last)*32;
    i = 32;
    #if defined(__AVR__)
    while(i--) *pB++ |= pgm_read_byte_near(pF++); 
    #else
    while(i--) *pB++ |= pgm_read_byte(pF++); 
    #endif
  }

  return pHgImg;
}


static byte c1;  // Last character buffer

byte utf8ascii(byte ascii) {
    if ( ascii<128 )   // Standard ASCII-set 0..0x7F handling  
    {   c1=0;
        return( ascii );
    }

    // get previous input
    byte last = c1;   // get last char
    c1=ascii;         // remember actual character

    switch (last)     // conversion depnding on first UTF8-character
    {   case 0xC2: return  (ascii);  break;
        case 0xC3: return  (ascii | 0xC0);  break;
        case 0x82: if(ascii==0xAC) return(0x80);       // special case Euro-symbol
    }

    return  (0);                                     // otherwise: return zero, if character has to be ignored
}

String utf8ascii(String s)
{      
        String r="";
        char c;
        for (int i=0; i<s.length(); i++)
        {
                c = utf8ascii(s.charAt(i));
                if (c!=0) r+=c;
        }
        return r;
}
void utf8ascii(char* s)
{      
        int k=0;
        char c;
        for (int i=0; i<strlen(s); i++)
        {
                c = utf8ascii(s[i]);
                if (c!=0)
                        s[k++]=c;
        }
        s[k]=0;
}

// send nibbleShift a decimal number
// example "nibbleShift(3)" returns '12'
// "0011" becomes "1100"
uint16_t nibbleShift16(uint16_t num) {

 uint16_t var = 0;    
 uint16_t i, x, y, p;
 int s = 16;    // number of bits in 'num'. (This case a 4bit nibble)

 for (i = 0; i < (s / 2); i++) {
   // extract bit on the left, from MSB
   p = s - i - 1;
   x = num & (1 << p);
   x = x >> p;  
   // extract bit on the right, from LSB
   y = num & (1 << i);
   y = y >> i;
 
   var = var | (x << i);       // apply x
   var = var | (y << p);       // apply y
 }
 return var;
}

// send nibbleShift a decimal number
// example "nibbleShift(3)" returns '12'
// "0011" becomes "1100"
uint8_t nibbleShift8(uint8_t num) {

 uint8_t var = 0;    
 uint8_t i, x, y, p;
 int s = 8;    // number of bits in 'num'. (This case a 4bit nibble)

 for (i = 0; i < (s / 2); i++) {
   // extract bit on the left, from MSB
   p = s - i - 1;
   x = num & (1 << p);
   x = x >> p;  
   // extract bit on the right, from LSB
   y = num & (1 << i);
   y = y >> i;
 
   var = var | (x << i);       // apply x
   var = var | (y << p);       // apply y
 }
 return var;
}

inline void expandX2(byte in,byte& out1,byte& out2) {
  byte t8;
  uint16_t t16 = 0;
  for (int j = 0; j < 8; j++) {
    t8 = in << j;
    t8 = t8 & 0x80;
    t8 = t8 >> 7;
    t16 = t16 | t8;
    t16 = t16 << 1;
    t16 = t16 | t8;
    if (j<7) {
     t16 = t16 << 1;
    }
  }
  out1 = t16 / 256;
  out2 = t16 & 0x00ff;
}

void bmpToXbmHg(byte* pHgImg) {
  byte t1,t2;
  uint16_t b1,b2;
  for (int i = 0; i < 16 ; i++) {
    t1 = *(pHgImg + i *2);
    t2 = *(pHgImg + (i *2+1));
        
    b2 = t2 * 256 + t1;
    b2 = nibbleShift16(b2);
    t2 = b2 >> 8;
    t1 = b2 & 0x00ff;

    *(pHgImg + i *2) = t2;
    *(pHgImg + (i *2+1)) = t1;
  }
}

void bmpToXbmAsc(byte* pAscImg,byte* ascImg) {
  for (int i = 0; i <16; i++) {
    ascImg[i] = *(pAscImg+i);
    ascImg[i] = nibbleShift8(ascImg[i]);  // BIT MAP --> XBM
  }
}

inline void enlargeHG_h2(byte *pHgImg,byte *pHgImg2) {
  byte a8,b8,v1,v2,v3,v4;

  for (int i = 0; i < 32 ; i += 2) {

    a8 = pHgImg[i];
    b8 = pHgImg[i+1];   
        
    expandX2(a8,v1,v2);
    expandX2(b8,v3,v4);
           
    pHgImg2[i*2+0] = v2;
    pHgImg2[i*2+1] = v1;
    pHgImg2[i*2+2] = v4;
    pHgImg2[i*2+3] = v3;
      
  }
}

inline void enlargeHG_v2(byte *pHgImg,byte *pHgImg2) {
  byte a8,b8;

  for (int i = 0; i < 32 ; i += 2) {

    a8 = pHgImg[i];
    b8 = pHgImg[i+1]; 
       
    pHgImg2[i*2+0] = a8;
    pHgImg2[i*2+1] = b8;
    pHgImg2[i*2+2] = a8;
    pHgImg2[i*2+3] = b8;
  }
}

inline void enlargeHG_x4(byte *pHgImg,byte *pHgImg2) {
  byte a8,b8,v1,v2,v3,v4;

  for (int i = 0; i < 32 ; i += 2) {
    a8 = pHgImg[i];
    b8 = pHgImg[i+1];   
        
    expandX2(a8,v1,v2);
    expandX2(b8,v3,v4);
           
    pHgImg2[i*4+0] = v2;
    pHgImg2[i*4+1] = v1;
    pHgImg2[i*4+2] = v4;
    pHgImg2[i*4+3] = v3;
       
    pHgImg2[i*4+4] = v2;
    pHgImg2[i*4+5] = v1;
    pHgImg2[i*4+6] = v4;
    pHgImg2[i*4+7] = v3;
  }
}

void enlargeASC_x4(byte* pAscImg,byte* pAscImg2) {

  byte a8,v1,v2;

  for (int i = 0; i < 16 ; i++) {
    a8 = pAscImg[i];
        
    expandX2(a8,v1,v2);
           
    pAscImg2[i*4+0] = v2;
    pAscImg2[i*4+1] = v1;
    pAscImg2[i*4+2] = v2;
    pAscImg2[i*4+3] = v1;
  }
}

///
void enlargeASC_h2(byte* pAscImg,byte* pAscImg2) {

  byte a8,v1,v2;

  for (int i = 0; i < 16 ; i++) {
    a8 = pAscImg[i];
        
    expandX2(a8,v1,v2);
           
    pAscImg2[i*2+0] = v2;
    pAscImg2[i*2+1] = v1;
  }
}

void enlargeASC_v2(byte* pAscImg,byte* pAscImg2) {

  byte a8,c1,c2;

  for (int i = 0; i < 16 ; i++) {
    a8 = pAscImg[i];

    pAscImg2[i*2+0] = a8;
    pAscImg2[i*2+1] = a8;
  }
}

/*
#define HG_SIZE_NORMAL 0
#define HG_SIZE_H2     2
#define HG_SIZE_V2     3
#define HG_SIZE_X4     4
*/

void AimHangul_base (uint16_t XPOS,uint16_t YPOS,const String& hgStr,uint32_t color,uint16_t hgSize) { 

  uint16_t screenWidth,screenHeight;

  #if defined(_M5STACK_H_) || defined(_M5STICKC_H_)

  if ((M5.Lcd.getRotation() % 2) == 0) {
    screenWidth  = TFT_WIDTH;
    screenHeight = TFT_HEIGHT;
  }
  else {
    screenWidth  = TFT_HEIGHT;
    screenHeight = TFT_WIDTH;
  }

  #elif defined(_TFT_eSPIH_)

  if ((tft.getRotation() % 2) == 0) {
    screenWidth  = TFT_WIDTH;
    screenHeight = TFT_HEIGHT;
  }
  else {
    screenWidth  = TFT_HEIGHT;
    screenHeight = TFT_WIDTH;
  }

  #elif defined(_U8G2LIB_HH)
  //Serial.printf("ScreenSize(w,h): %d,%d\n",u8g2.getDisplayWidth(),u8g2.getDisplayHeight());  
  //if (((int)U8G2_DIR % 2) == 0) {
    screenWidth  =  u8g2.getDisplayWidth(); // u8g2.getCols();
    screenHeight =  u8g2.getDisplayHeight();//
  //}
  //else {
  //  screenWidth  = u8g2.getDisplayHeight();
  //  screenHeight = u8g2.getDisplayWidth();
  //}

  #else 
  #error "Display Lib error"  

  #endif

  byte hgImg[32]   = {0,}; // 최초 완성 이미지 
  byte hgImg2[128] = {0,}; // 가로나 세로로 확대된 이미지 

  byte *pHgImg  = hgImg;
  byte *pHgImg2 = hgImg2;

  //byte ascImg[16] = {0,}; // 최초 이미지 
  //byte ascImg2[64] = {0,}; // 가로나 세로로 확대된 이미지 

  //byte *pAscImg  = ascImg;
  //byte *pAscImg2 = ascImg2;

  int w1,w2,h1,h2; // 1: hangul , 2: ascii
  
  switch(hgSize) {
    case HG_SIZE_NORMAL : w1 = 16; w2 =  8; h1 = 16; h2 = 16; break;
    case HG_SIZE_H2     : w1 = 32; w2 = 16; h1 = 16; h2 = 16; break;
    case HG_SIZE_V2     : w1 = 16; w2 =  8; h1 = 32; h2 = 32; break;
    case HG_SIZE_X4     : w1 = 32; w2 = 16; h1 = 32; h2 = 32; break;
  }

  char hgChar[64];         // String --> char[]
  hgStr.toCharArray(hgChar,sizeof(hgChar));

  char *pChar = hgChar;    // 문자열 연산하기 위해서 pChar사용   

  byte c,c2,c3;  // utf-8 문자 

  while(*pChar){ 
    c = *(byte*)pChar++;

    //---------- 한글 ---------
    if(c >= 0x80) {
      c2 = *(byte*)pChar++;
      c3 = *(byte*)pChar++; 
      getHAN_font(c, c2, c3,pHgImg); 

      bmpToXbmHg(pHgImg);
      
      switch(hgSize) {
        case HG_SIZE_NORMAL : pHgImg2 = pHgImg; break;
        case HG_SIZE_H2     : enlargeHG_h2(pHgImg,pHgImg2); break;
        case HG_SIZE_V2     : enlargeHG_v2(pHgImg,pHgImg2); break;
        case HG_SIZE_X4     : enlargeHG_x4(pHgImg,pHgImg2); break;
      }

      if((XPOS+w1) > screenWidth) {
        XPOS = 0;
        YPOS = YPOS+h1+ 3;
      } 
      
      #if defined(_M5STACK_H_) || defined(_M5STICKC_H_)
      M5.Lcd.drawXBitmap(XPOS, YPOS, pHgImg2, w1, h1, color);
      #elif defined(_TFT_eSPIH_)
      tft.drawXBitmap(XPOS, YPOS, pHgImg2, w1, h1, color);
      #elif defined(_U8G2LIB_HH)
      u8g2.drawXBM(XPOS, YPOS, w1, h1, pHgImg2); 
      #else 
      #error "Display Lib error"      
      #endif 

      XPOS = XPOS + w1;
    }
    //---------- ASCII ---------
    else{

      byte * pFont = (byte*)ASCfontSet + ((byte)c - 0x20) * 16;      
      int i = 16, j = 0;
      #if defined(__AVR__)
      while(i--) hgImg[j++] = pgm_read_byte_near(pFont++);
      #else
      while(i--) hgImg[j++] = pgm_read_byte(pFont++);
      #endif

      bmpToXbmAsc(pHgImg,hgImg);

      switch(hgSize) {
        case HG_SIZE_NORMAL : pHgImg2 = hgImg; break;
        case HG_SIZE_H2     : enlargeASC_h2(hgImg,pHgImg2); break;
        case HG_SIZE_V2     : enlargeASC_v2(hgImg,pHgImg2); break;
        case HG_SIZE_X4     : enlargeASC_x4(hgImg,pHgImg2); break;
      }

      if((XPOS+w2) > screenWidth) {
        XPOS = 0;
        YPOS = YPOS+h2+3;
      } 
      #if defined(_M5STACK_H_) || defined(_M5STICKC_H_)
      M5.Lcd.drawXBitmap(XPOS, YPOS,pHgImg2, w2, h2, color);
      #elif defined(_TFT_eSPIH_)
      tft.drawXBitmap(XPOS, YPOS,pHgImg2, w2, h2, color);
      #elif defined(_U8G2LIB_HH)
      u8g2.drawXBM(XPOS, YPOS, w2, h2, pHgImg2);
      #else 
      #error "Display Lib error"   
      #endif

      XPOS = XPOS+w2;
    }   
  }  
} 

void AimHangul(int XPOS,int YPOS,const String& hgStr,uint32_t color = 0) {
  AimHangul_base(XPOS,YPOS,hgStr,color,HG_SIZE_NORMAL); 
}  

void AimHangul_h2(int XPOS,int YPOS,const String& hgStr,uint32_t color = 0) {
  AimHangul_base(XPOS,YPOS,hgStr,color,HG_SIZE_H2); 
}  

void AimHangul_v2(int XPOS,int YPOS,const String& hgStr,uint32_t color = 0) {
  AimHangul_base(XPOS,YPOS,hgStr,color,HG_SIZE_V2); 
}  

void AimHangul_x4(int XPOS,int YPOS,const String& hgStr,uint32_t color = 0) {
  AimHangul_base(XPOS,YPOS,hgStr,color,HG_SIZE_X4); 
}  

#endif // #ifndef AIM_HANGUL_MS_H