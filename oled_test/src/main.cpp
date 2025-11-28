#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>

// WeAct default pins (from project device header): SDA=21, SCL=22
#define OLED_SDA 21
#define OLED_SCL 22

// Use hardware I2C constructor for SSD1306 128x64 (common 0.96" OLED)
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

void setup() {
  Wire.begin(OLED_SDA, OLED_SCL);
  u8g2.begin();
  u8g2.setFont(u8g2_font_ncenB08_tr);
}

void loop() {
  char buf[32];
  snprintf(buf, sizeof(buf), "ms: %lu", millis());

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB14_tr);
  u8g2.drawStr(0, 22, "OLED Test");
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(0, 44, buf);
  u8g2.drawFrame(0,0,128,64);
  u8g2.sendBuffer();

  delay(1000);
}
