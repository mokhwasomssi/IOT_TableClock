# widget_tableclock
실용성있는 위젯을 추가한 IOT 탁상시계

## ESP32 dev board
- [ESP32-C3-DevKitM-1](https://docs.espressif.com/projects/esp-idf/en/latest/esp32c3/hw-reference/esp32c3/user-guide-devkitm-1.html)
- [ESP32­-C3­-MINI-­1](https://www.espressif.com/sites/default/files/documentation/esp32-c3-mini-1_datasheet_en.pdf)
- [ESP32-C3FN4](https://www.espressif.com/sites/default/files/documentation/esp32-c3_datasheet_en.pdf)
  

## 설명서

- [시스템도](https://whimsical.com/wiget-tableclock-EQHfxn7x5JfxGt9CBQZhHa)

## 개발환경
- Arduino IDE 2.0.4
    - TFT_eSPI 라이브러리 설치
      - `User_Setup.h` 헤더파일은 `문서\Arduino\libraries\TFT_eSPI`로 붙여넣기
    - CST816S 라이브러리
      - 헤더, 소스파일 직접 포함 [fbiego/CST816S](https://github.com/fbiego/CST816S)
      - `uint8_t i2c_read(uint16_t addr, uint8_t reg_addr, uint8_t * reg_data, size_t length)`로 수정함
    - [tinyxml2](https://github.com/leethomason/tinyxml2) : XML parser
- ESP32-C3-DEVKITM-1_V1  
  - GPIO 8, 9 사용 X  
- 1.28 inch round Touch Display  
    - LCD controller : GC9A01  
    - Touch controller : CST816S  
- 핀연결
    ```c
  /* GC9A01 display */
  #define TFT_MOSI 3     
  // #define TFT_MISO     // NC  
  #define TFT_SCLK 0  
  #define TFT_CS   1     
  #define TFT_DC   10     // Data Command control pin
  #define TFT_RST  19  
  // #define TFT_BL       // 3.3V

  /* CST816S Touch */
  #define TOUCH_SDA 4
  #define TOUCH_SCL 5
  #define TOUCH_INT 6
  #define TOUCH_RST 7
  ```
  - API
    - [Weather API](https://openweathermap.org/)


## 참고자료
- 원형 디스플레이
  - https://github.com/Makerfabs/ESP32-S3-Round-SPI-TFT-with-Touch-1.28
- 현재 날짜 및 시각
  - https://randomnerdtutorials.com/esp32-date-time-ntp-client-server-arduino/
- 


## 예상질문
- 스마트워치하고 차이점
- 스마트미러하고 차이점