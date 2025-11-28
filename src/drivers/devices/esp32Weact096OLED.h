#ifndef _ESP32_WEACT_096_OLED_H
#define _ESP32_WEACT_096_OLED_H

// Default SDA/SCL for many ESP32 devboards; change if your board uses different pins
#ifndef SDA_PIN
#define SDA_PIN 21
#endif

#ifndef SCL_PIN
#define SCL_PIN 22
#endif

// Enable the 0.96" SSD1306 128x64 driver
#define OLED_096_DISPLAY

#endif
