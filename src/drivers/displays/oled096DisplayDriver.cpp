// SSD1306 128x64 OLED (0.96") display driver using U8g2 HW I2C

#include "displayDriver.h"

#ifdef OLED_096_DISPLAY

#include <U8g2lib.h>
#include <Wire.h>
#include "monitor.h"
#include "mining.h"
#include <WiFi.h>

// Use the common SSD1306 128x64 noname constructor for HW I2C
static U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

static void clearScreen(void) {
    u8g2.clearBuffer();
    u8g2.sendBuffer();
}

static void serialPrint128(unsigned long mElapsed) {
  mining_data data = getMiningData(mElapsed);
  Serial.printf(">>> shares:%s  valids:%s  Khashes:%s  avg:%s KH/s\n",
                data.completedShares.c_str(), data.valids.c_str(), data.totalKHashes.c_str(), data.currentHashRate.c_str());

  // Also print pool & wifi info for debugging
  pool_data pdata = getPoolData();
  Serial.printf("Pool workers:%d  poolHash:%s\n", pdata.workersCount, pdata.workersHash.c_str());
  Serial.printf("WiFi: %s %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
}

static void oled096Display_Init(void)
{
  Serial.println("OLED 0.96 128x64 display driver initialized");
  // SDA/SCL pins should be defined by device headers as SDA_PIN / SCL_PIN
  Wire.begin(SDA_PIN, SCL_PIN);
  Serial.printf("I2C init: SDA=%d, SCL=%d\n", SDA_PIN, SCL_PIN);

  // Quick I2C scan to help debug wiring/address issues
  Serial.println("Scanning I2C bus...");
  bool found = false;
  for (uint8_t addr = 1; addr < 127; ++addr) {
    Wire.beginTransmission(addr);
    uint8_t err = Wire.endTransmission();
    if (err == 0) {
      Serial.printf("  Found device at 0x%02X\n", addr);
      found = true;
    }
  }
  if (!found) Serial.println("  No I2C devices found. Check wiring, power, and pull-ups.");

  u8g2.begin();
  u8g2.clear();
  clearScreen();
}

static void oled096Display_AlternateScreenState(void)
{
  Serial.println("Switching display state (96)");
}

static void oled096Display_AlternateRotation(void)
{
}

static void oled096Display_Screen1(unsigned long mElapsed)
{
  mining_data data = getMiningData(mElapsed);

  u8g2.clearBuffer();
  
  // Get individual worker hash rates
  float workerRates[MINER_WORKER_COUNT];
  memset(workerRates, 0, sizeof(workerRates));
  getWorkerHashRates(workerRates, MINER_WORKER_COUNT);
  
  // Title and CPU freq
  u8g2.setFont(u8g2_font_4x6_tf); // extra small font to ensure 4 lines fit
  u8g2.drawStr(0, 7, "Workers KH/s");
  
  // CPU frequency on top right
  char cpuFreq[16];
  snprintf(cpuFreq, sizeof(cpuFreq), "%dMHz", getCpuFrequencyMhz());
  u8g2.drawStr(128 - u8g2.getStrWidth(cpuFreq), 7, cpuFreq);
  
  // Show each worker's hash rate
  const int maxLines = 4;
  int y = 15;
  char line[28];
  for (int i = 0; i < MINER_WORKER_COUNT && i < maxLines; ++i) {
    // Cap display to one decimal to keep line short
    snprintf(line, sizeof(line), "W%d: %.1f", i, workerRates[i]);
    u8g2.drawStr(0, y, line);
    y += 10; // condensed spacing for 4 lines within 64px height
  }
  
  // Shares / Valids on bottom
  u8g2.setFont(u8g2_font_6x10_tf);
  String shareLine = "S:" + data.completedShares + " V:" + data.valids;
  u8g2.drawStr(0, 64, shareLine.c_str());

  u8g2.sendBuffer();

  serialPrint128(mElapsed);
}

static void oled096Display_Screen2(unsigned long mElapsed)
{
  mining_data data = getMiningData(mElapsed);
  char temp[8];
  sprintf(temp, "%s°C", data.temp.c_str());

  u8g2.clearBuffer();

  // Temperature large
  u8g2.setFont(u8g2_font_helvB18_tf);
  u8g2.drawUTF8(0, 20, temp);

  // Best difficulty and templates
  u8g2.setFont(u8g2_font_helvB08_tf);
  String diffLine = "Best:" + data.bestDiff + " Tpl:" + data.templates;
  // truncate if too long
  if (diffLine.length() > 24) diffLine = diffLine.substring(0,24);
  u8g2.drawStr(0, 40, diffLine.c_str());

  // Pool and WiFi info (small font)
  pool_data pdata = getPoolData();
  u8g2.setFont(u8g2_font_6x10_tf);
  char poolBuf[32];
  snprintf(poolBuf, sizeof(poolBuf), "W:%d H:%s", pdata.workersCount, pdata.workersHash.c_str());
  u8g2.drawStr(0, 52, poolBuf);

  // (WiFi moved to Screen 3 to keep this view focused)

  u8g2.sendBuffer();

  serialPrint128(mElapsed);
}

static void oled096Display_Screen3(unsigned long mElapsed)
{
  u8g2.clearBuffer();

  // Show WiFi SSID (left) and IP (right) on a dedicated small-font screen
  u8g2.setFont(u8g2_font_6x10_tf);
  String ss = WiFi.SSID();
  String ip = WiFi.localIP().toString();

  String ssLine = ss.length() ? ss : "NoSSID";
  if (ssLine.length() > 20) ssLine = ssLine.substring(0,20);
  u8g2.drawStr(0, 24, ssLine.c_str());

  String ipLine = ip.length() ? ip : "0.0.0.0";
  u8g2.drawStr(0, 44, ipLine.c_str());

  u8g2.sendBuffer();

  // Also print WiFi on serial for debugging
  Serial.printf("OLED Screen3 WiFi: %s %s\n", ss.c_str(), ip.c_str());
}

static void oled096Display_LoadingScreen(void)
{
  Serial.println("Initializing SSD1306 128x64...");
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_helvB12_tf);
  u8g2.drawStr(0, 24, "Initializing...");
  u8g2.sendBuffer();
}

static void oled096Display_SetupScreen(void)
{
  Serial.println("Setup 128x64...");
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_helvB08_tf);
  u8g2.drawUTF8(0, 24, "Setup");
  u8g2.sendBuffer();
}

static void oled096Display_DoLedStuff(unsigned long frame)
{
}

static void oled096Display_AnimateCurrentScreen(unsigned long frame)
{
}
CyclicScreenFunction oled096DisplayCyclicScreens[] = {oled096Display_Screen1, oled096Display_Screen2};

DisplayDriver oled096DisplayDriver = {
    oled096Display_Init,
    oled096Display_AlternateScreenState,
    oled096Display_AlternateRotation,
    oled096Display_LoadingScreen,
    oled096Display_SetupScreen,
    oled096DisplayCyclicScreens,
    oled096Display_AnimateCurrentScreen,
    oled096Display_DoLedStuff,
    SCREENS_ARRAY_SIZE(oled096DisplayCyclicScreens),
    0,
    128,
    64,
};

#endif
