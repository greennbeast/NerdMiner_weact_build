#define ESP_DRD_USE_SPIFFS true

// Include Libraries
//#include ".h"

#include <WiFi.h>

#include <WiFiManager.h>

#include "wManager.h"
#include "monitor.h"
#include "drivers/displays/display.h"
#include "drivers/storage/SDCard.h"
#include "drivers/storage/nvMemory.h"
#include "drivers/storage/storage.h"
#include "mining.h"
#include "timeconst.h"

#include <ArduinoJson.h>
#include <esp_flash.h>
#include <NTPClient.h>


// Flag for saving data
bool shouldSaveConfig = false;

// Variables to hold data from custom textboxes
TSettings Settings;

// Define WiFiManager Object
WiFiManager wm;
extern monitor_data mMonitor;

nvMemory nvMem;

extern SDCard SDCrd;

String readCustomAPName() {
    Serial.println("DEBUG: Attempting to read custom AP name from flash at 0x3F0000...");
    
    // Leer directamente desde flash
    const size_t DATA_SIZE = 128;
    uint8_t buffer[DATA_SIZE];
    memset(buffer, 0, DATA_SIZE); // Clear buffer
    
    // Leer desde 0x3F0000
    esp_err_t result = esp_flash_read(NULL, buffer, 0x3F0000, DATA_SIZE);
    if (result != ESP_OK) {
        Serial.printf("DEBUG: Flash read error: %s\n", esp_err_to_name(result));
        return "";
    }
    
    Serial.println("DEBUG: Successfully read from flash");
    String data = String((char*)buffer);
    
    // Debug: show raw data read
    Serial.printf("DEBUG: Raw flash data: '%s'\n", data.c_str());
    
    if (data.startsWith("WEBFLASHER_CONFIG:")) {
        Serial.println("DEBUG: Found WEBFLASHER_CONFIG marker");
        String jsonPart = data.substring(18); // Después del marcador "WEBFLASHER_CONFIG:"
        
        Serial.printf("DEBUG: JSON part: '%s'\n", jsonPart.c_str());
        
        DynamicJsonDocument doc(256);
        DeserializationError error = deserializeJson(doc, jsonPart);
        
        if (error == DeserializationError::Ok) {
            Serial.println("DEBUG: JSON parsed successfully");
            
            if (doc.containsKey("apname")) {
                String customAP = doc["apname"].as<String>();
                customAP.trim();
                
                if (customAP.length() > 0 && customAP.length() < 32) {
                    Serial.printf("✅ Custom AP name from webflasher: %s\n", customAP.c_str());
                    return customAP;
                } else {
                    Serial.printf("DEBUG: AP name invalid length: %d\n", customAP.length());
                }
            } else {
                Serial.println("DEBUG: 'apname' key not found in JSON");
            }
        } else {
            Serial.printf("DEBUG: JSON parse error: %s\n", error.c_str());
        }
    } else {
        Serial.println("DEBUG: WEBFLASHER_CONFIG marker not found - no custom config");
    }
    
    Serial.println("DEBUG: Using default AP name");
    return "";
}

void saveConfigCallback()
// Callback notifying us of the need to save configuration
{
    Serial.println("Should save config");
    shouldSaveConfig = true;    
    //wm.setConfigPortalBlocking(false);
}

/* void saveParamsCallback()
// Callback notifying us of the need to save configuration
{
    Serial.println("Should save config");
    shouldSaveConfig = true;
    nvMem.saveConfig(&Settings);
} */

void configModeCallback(WiFiManager* myWiFiManager)
// Called when config mode launched
{
    Serial.println("Entered Configuration Mode");
    drawSetupScreen();
    Serial.print("Config SSID: ");
    Serial.println(myWiFiManager->getConfigPortalSSID());

    Serial.print("Config IP Address: ");
    Serial.println(WiFi.softAPIP());
}

void reset_configuration()
{
    Serial.println("Erasing Config, restarting");
    nvMem.deleteConfig();
    resetStat();
    wm.resetSettings();
    ESP.restart();
}

void startConfigPortal()
{
    Serial.println("Starting config portal on demand...");
    wm.startConfigPortal(DEFAULT_SSID, DEFAULT_WIFIPW);
}

void init_WifiManager()
{
#ifdef MONITOR_SPEED
    Serial.begin(MONITOR_SPEED);
#else
    Serial.begin(115200);
#endif //MONITOR_SPEED
    //Serial.setTxTimeoutMs(10);
    
    // Improve AP reliability on some clients
    WiFi.setSleep(false);
    
    // Check for custom AP name from flasher config, otherwise use default
    String customAPName = readCustomAPName();
    const char* apName = customAPName.length() > 0 ? customAPName.c_str() : DEFAULT_SSID;

    //Init pin 15 to eneble 5V external power (LilyGo bug)
#ifdef PIN_ENABLE5V
    pinMode(PIN_ENABLE5V, OUTPUT);
    digitalWrite(PIN_ENABLE5V, HIGH);
#endif

    // Change to true when testing to force configuration every time we run
    bool forceConfig = false;

#if defined(PIN_BUTTON_2)
    // Check if button2 is pressed to enter configMode with actual configuration
    if (!digitalRead(PIN_BUTTON_2)) {
        Serial.println(F("Button pressed to force start config mode"));
        forceConfig = true;
        wm.setBreakAfterConfig(true); //Set to detect config edition and save
    }
#endif
    // Explicitly set WiFi mode to STA+AP (station + access point)
    WiFi.mode(WIFI_AP_STA);

    if (!nvMem.loadConfig(&Settings))
    {
        //No config file on internal flash.
        if (SDCrd.loadConfigFile(&Settings))
        {
            //Config file on SD card.
            SDCrd.SD2nvMemory(&nvMem, &Settings); // reboot on success.          
        }
        else
        {
            //No config file on SD card. Starting wifi config server.
            forceConfig = true;
        }
    };
    
    // Free the memory from SDCard class 
    SDCrd.terminate();
    
    // Reset settings (only for development)
    //wm.resetSettings();

    //Set dark theme
    //wm.setClass("invert"); // dark theme

    // Set config save notify callback
    wm.setSaveConfigCallback(saveConfigCallback);
    wm.setSaveParamsCallback(saveConfigCallback);

    // Set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
    wm.setAPCallback(configModeCallback);    

    //Advanced settings
    wm.setConfigPortalBlocking(false); //Hacemos que el portal no bloquee el firmware
    wm.setConnectTimeout(40); // how long to try to connect for before continuing
    wm.setConfigPortalTimeout(0); // keep portal accessible (0 = no timeout)
    // wm.setCaptivePortalEnable(false); // disable captive portal redirection
    // wm.setAPClientCheck(true); // avoid timeout if client connected to softap
    //wm.setTimeout(120);
    //wm.setConfigPortalTimeout(120); //seconds

    // Custom elements

    // Pool presets dropdown with JavaScript to auto-fill URL and port
    const char* poolPresetsHTML = 
        "<br/><label for='poolPreset'>Quick Pool Select:</label>"
        "<select id='poolPreset' onchange='setPool(this.value)'>"
        "<option value=''>-- Select a pool preset --</option>"
        "<option value='public-pool.io:21496'>Public Pool (public-pool.io:21496)</option>"
        "<option value='solo.ckpool.org:3333'>CKPool Solo (solo.ckpool.org:3333)</option>"
        "<option value='pool.vkbit.com:3333'>VKBit (pool.vkbit.com:3333)</option>"
        "<option value='stratum.braiins.com:3333'>Braiins Pool (stratum.braiins.com:3333)</option>"
        "<option value='us1.btcpaypool.com:3333'>BTC Pay Pool (us1.btcpaypool.com:3333)</option>"
        "<option value='tazmining.ch:33333'>Taz Mining (tazmining.ch:33333)</option>"
        "</select>"
        "<script>"
        "function setPool(val){"
        "if(!val)return;"
        "var parts=val.split(':');"
        "document.getElementById('Poolurl').value=parts[0];"
        "document.getElementById('Poolport').value=parts[1]||'';"
        "}"
        "</script><br/><br/>";
    
    WiFiManagerParameter pool_presets(poolPresetsHTML);

    // Text box (String) - 80 characters maximum
    WiFiManagerParameter pool_text_box("Poolurl", "Pool url", Settings.PoolAddress.c_str(), 80);

    // Need to convert numerical input to string to display the default value.
    char convertedValue[6];
    sprintf(convertedValue, "%d", Settings.PoolPort);

    // Text box (Number) - 7 characters maximum
    WiFiManagerParameter port_text_box_num("Poolport", "Pool port", convertedValue, 7);

    // Text box (String) - 80 characters maximum
    //WiFiManagerParameter password_text_box("Poolpassword", "Pool password (Optional)", Settings.PoolPassword, 80);

    // Text box (String) - 80 characters maximum
    WiFiManagerParameter addr_text_box("btcAddress", "Your BTC address", Settings.BtcWallet, 80);

  // Text box (Number) - 2 characters maximum
  char charZone[6];
  sprintf(charZone, "%d", Settings.Timezone);
  WiFiManagerParameter time_text_box_num("TimeZone", "TimeZone fromUTC (-12/+12)", charZone, 3);

  WiFiManagerParameter features_html("<hr><br><label style=\"font-weight: bold;margin-bottom: 25px;display: inline-block;\">Features</label>");

  char checkboxParams[24] = "type=\"checkbox\"";
  if (Settings.saveStats)
  {
    strcat(checkboxParams, " checked");
  }
  WiFiManagerParameter save_stats_to_nvs("SaveStatsToNVS", "Save mining statistics to flash memory.", "T", 2, checkboxParams, WFM_LABEL_AFTER);
  // Text box (String) - 80 characters maximum
  WiFiManagerParameter password_text_box("Poolpassword - Optional", "Pool password", Settings.PoolPassword, 80);

  // Add all defined parameters
  wm.addParameter(&pool_presets);
  wm.addParameter(&pool_text_box);
  wm.addParameter(&port_text_box_num);
  wm.addParameter(&password_text_box);
  wm.addParameter(&addr_text_box);
  wm.addParameter(&time_text_box_num);
  wm.addParameter(&features_html);
  wm.addParameter(&save_stats_to_nvs);
  #if defined(ESP32_2432S028R) || defined(ESP32_2432S028_2USB)
  char checkboxParams2[24] = "type=\"checkbox\"";
  if (Settings.invertColors)
  {
    strcat(checkboxParams2, " checked");
  }
  WiFiManagerParameter invertColors("inverColors", "Invert Display Colors (if the colors looks weird)", "T", 2, checkboxParams2, WFM_LABEL_AFTER);
  wm.addParameter(&invertColors);
  #endif
  #if defined(ESP32_2432S028R) || defined(ESP32_2432S028_2USB)
    char brightnessConvValue[2];
    sprintf(brightnessConvValue, "%d", Settings.Brightness);
    // Text box (Number) - 3 characters maximum
    WiFiManagerParameter brightness_text_box_num("Brightness", "Screen backlight Duty Cycle (0-255)", brightnessConvValue, 3);
    wm.addParameter(&brightness_text_box_num);
  #endif

    Serial.println("AllDone: ");
    if (forceConfig)    
    {
        // Run if we need a configuration
        //No configuramos timeout al modulo
        wm.setConfigPortalBlocking(true); //Hacemos que el portal SI bloquee el firmware
        drawSetupScreen();
        mMonitor.NerdStatus = NM_Connecting;
        wm.startConfigPortal(apName, DEFAULT_WIFIPW);

        if (shouldSaveConfig)
        {
            //Could be break forced after edditing, so save new config
            Serial.println("failed to connect and hit timeout");
            Settings.PoolAddress = pool_text_box.getValue();
            Settings.PoolPort = atoi(port_text_box_num.getValue());
            strncpy(Settings.PoolPassword, password_text_box.getValue(), sizeof(Settings.PoolPassword));
            strncpy(Settings.BtcWallet, addr_text_box.getValue(), sizeof(Settings.BtcWallet));
            Settings.Timezone = atoi(time_text_box_num.getValue());
            //Serial.println(save_stats_to_nvs.getValue());
            Settings.saveStats = (strncmp(save_stats_to_nvs.getValue(), "T", 1) == 0);
            #if defined(ESP32_2432S028R) || defined(ESP32_2432S028_2USB)
                Settings.invertColors = (strncmp(invertColors.getValue(), "T", 1) == 0);
            #endif
            #if defined(ESP32_2432S028R) || defined(ESP32_2432S028_2USB)
                Settings.Brightness = atoi(brightness_text_box_num.getValue());
            #endif
            nvMem.saveConfig(&Settings);
            delay(3*SECOND_MS);
            //reset and try again, or maybe put it to deep sleep
            ESP.restart();            
        };
    }
    else
    {
        //Tratamos de conectar con la configuración inicial ya almacenada
        mMonitor.NerdStatus = NM_Connecting;
        // disable captive portal redirection
        wm.setCaptivePortalEnable(true); 
        wm.setConfigPortalBlocking(true);
        wm.setEnableConfigPortal(true);
        // if (!wm.autoConnect(Settings.WifiSSID.c_str(), Settings.WifiPW.c_str()))
        if (!wm.autoConnect(apName, DEFAULT_WIFIPW))
        {
            Serial.println("Failed to connect to configured WIFI, and hit timeout");
            if (shouldSaveConfig) {
                // Save new config            
                Settings.PoolAddress = pool_text_box.getValue();
                Settings.PoolPort = atoi(port_text_box_num.getValue());
                strncpy(Settings.PoolPassword, password_text_box.getValue(), sizeof(Settings.PoolPassword));
                strncpy(Settings.BtcWallet, addr_text_box.getValue(), sizeof(Settings.BtcWallet));
                Settings.Timezone = atoi(time_text_box_num.getValue());
                // Serial.println(save_stats_to_nvs.getValue());
                Settings.saveStats = (strncmp(save_stats_to_nvs.getValue(), "T", 1) == 0);
                #if defined(ESP32_2432S028R) || defined(ESP32_2432S028_2USB)
                Settings.invertColors = (strncmp(invertColors.getValue(), "T", 1) == 0);
                #endif
                #if defined(ESP32_2432S028R) || defined(ESP32_2432S028_2USB)
                Settings.Brightness = atoi(brightness_text_box_num.getValue());
                #endif
                nvMem.saveConfig(&Settings);
                vTaskDelay(2000 / portTICK_PERIOD_MS);      
            }        
            ESP.restart();                            
        } 
    }
    
    //Conectado a la red Wifi
    if (WiFi.status() == WL_CONNECTED) {
        //tft.pushImage(0, 0, MinerWidth, MinerHeight, MinerScreen);
        Serial.println("");
        Serial.println("WiFi connected");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        
        // Start AP alongside STA connection with explicit network config
        IPAddress apIP(192, 168, 4, 1);
        IPAddress gateway(192, 168, 4, 1);
        IPAddress subnet(255, 255, 255, 0);
        if (!WiFi.softAPConfig(apIP, gateway, subnet)) {
            Serial.println("[AP] softAPConfig failed (using defaults)");
        }
        // Use current STA channel for AP to avoid mismatch
        int currentCh = WiFi.channel();
        if (currentCh <= 0) currentCh = 1;
        // hidden=0, max_connection=8
        bool apOk = WiFi.softAP(apName, DEFAULT_WIFIPW, currentCh, 0, 8);
        if (!apOk) {
            Serial.println("[AP] Failed to start SoftAP");
        }
        Serial.print("Access Point started: ");
        Serial.println(apName);
        Serial.print("AP password: ");
        Serial.println(DEFAULT_WIFIPW);
        Serial.print("AP IP address: ");
        Serial.println(WiFi.softAPIP());
        Serial.print("STA IP address: ");
        Serial.println(WiFi.localIP());
        Serial.print("WiFi channel (STA/AP): ");
        Serial.println(currentCh);
        Serial.println("Portal accessible at http://192.168.4.1 or via STA IP");

        // Ensure WiFiManager's web portal is running on both STA and AP interfaces
        if (!wm.getWebPortalActive()) {
            Serial.println("[AP] Starting WiFiManager web portal");
            wm.startWebPortal();
        } else {
            Serial.println("[AP] WiFiManager web portal already active");
        }
        
        // Setup custom web pages for settings
        setupCustomWebPages();
        
        Serial.println("[SETTINGS] Pool settings page: http://192.168.0.185/settings");
        Serial.println("[SETTINGS] Or via AP: http://192.168.4.1/settings");


        // Lets deal with the user config values

        // Copy the string value
        Settings.PoolAddress = pool_text_box.getValue();
        //strncpy(Settings.PoolAddress, pool_text_box.getValue(), sizeof(Settings.PoolAddress));
        Serial.print("PoolString: ");
        Serial.println(Settings.PoolAddress);

        //Convert the number value
        Settings.PoolPort = atoi(port_text_box_num.getValue());
        Serial.print("portNumber: ");
        Serial.println(Settings.PoolPort);

        // Copy the string value
        strncpy(Settings.PoolPassword, password_text_box.getValue(), sizeof(Settings.PoolPassword));
        Serial.print("poolPassword: ");
        Serial.println(Settings.PoolPassword);

        // Copy the string value
        strncpy(Settings.BtcWallet, addr_text_box.getValue(), sizeof(Settings.BtcWallet));
        Serial.print("btcString: ");
        Serial.println(Settings.BtcWallet);

        //Convert the number value
        Settings.Timezone = atoi(time_text_box_num.getValue());
        Serial.print("TimeZone fromUTC: ");
        Serial.println(Settings.Timezone);

        #if defined(ESP32_2432S028R) || defined(ESP32_2432S028_2USB)
        Settings.invertColors = (strncmp(invertColors.getValue(), "T", 1) == 0);
        Serial.print("Invert Colors: ");
        Serial.println(Settings.invertColors);        
        #endif

        #if defined(ESP32_2432S028R) || defined(ESP32_2432S028_2USB)
        Settings.Brightness = atoi(brightness_text_box_num.getValue());
        Serial.print("Brightness: ");
        Serial.println(Settings.Brightness);
        #endif

    }

    // Lets deal with the user config values

    // Copy the string value
    Settings.PoolAddress = pool_text_box.getValue();
    //strncpy(Settings.PoolAddress, pool_text_box.getValue(), sizeof(Settings.PoolAddress));
    Serial.print("PoolString: ");
    Serial.println(Settings.PoolAddress);

    //Convert the number value
    Settings.PoolPort = atoi(port_text_box_num.getValue());
    Serial.print("portNumber: ");
    Serial.println(Settings.PoolPort);

    // Copy the string value
    strncpy(Settings.PoolPassword, password_text_box.getValue(), sizeof(Settings.PoolPassword));
    Serial.print("poolPassword: ");
    Serial.println(Settings.PoolPassword);

    // Copy the string value
    strncpy(Settings.BtcWallet, addr_text_box.getValue(), sizeof(Settings.BtcWallet));
    Serial.print("btcString: ");
    Serial.println(Settings.BtcWallet);

    //Convert the number value
    Settings.Timezone = atoi(time_text_box_num.getValue());
    Serial.print("TimeZone fromUTC: ");
    Serial.println(Settings.Timezone);

    #ifdef ESP32_2432S028R
    Settings.invertColors = (strncmp(invertColors.getValue(), "T", 1) == 0);
    Serial.print("Invert Colors: ");
    Serial.println(Settings.invertColors);
    #endif

    // Save the custom parameters to FS
    if (shouldSaveConfig)
    {
        nvMem.saveConfig(&Settings);
        #if defined(ESP32_2432S028R) || defined(ESP32_2432S028_2USB)
         if (Settings.invertColors) ESP.restart();                
        #endif
        #if defined(ESP32_2432S028R) || defined(ESP32_2432S028_2USB)
        if (Settings.Brightness != 250) ESP.restart();
        #endif
    }
}

//----------------- MAIN PROCESS WIFI MANAGER --------------
int oldStatus = 0;

void wifiManagerProcess() {

    wm.process(); // avoid delays() in loop when non-blocking and other long running code

    int newStatus = WiFi.status();
    if (newStatus != oldStatus) {
        if (newStatus == WL_CONNECTED) {
            Serial.println("CONNECTED - Current ip: " + WiFi.localIP().toString());
        } else {
            Serial.print("[Error] - current status: ");
            Serial.println(newStatus);
        }
        oldStatus = newStatus;
    }
}

//----------------- TRIGGER CONFIG PORTAL --------------
void triggerConfigPortal() {
    Serial.println("[CONFIG] Restarting into configuration mode...");
    ESP.restart();
}

//----------------- CUSTOM WEB PAGES --------------
void setupCustomWebPages() {
    // Stats API endpoint for live data
    // Preflight CORS handler for browsers
    wm.server->on("/api/stats", HTTP_OPTIONS, [](){
        wm.server->sendHeader("Access-Control-Allow-Origin", "*");
        wm.server->sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        wm.server->sendHeader("Access-Control-Allow-Headers", "*");
        wm.server->send(200);
    });
    wm.server->on("/api/stats", [](){
        extern uint32_t elapsedKHs;
        extern uint32_t shares;
        extern uint32_t valids;
        extern uint32_t templates;
        extern uint32_t Mhashes;
        extern uint64_t upTime;
        extern double best_diff;
        extern unsigned int bitcoin_price;
        extern String current_block;
        extern global_data gData;
        extern NTPClient timeClient;
        extern TSettings Settings;
        
        // Update global data (BTC price, block height, network stats)
        String btcPrice = getBTCprice();
        String blockHeight = getBlockHeight();
        updateGlobalData();
        
        // Check if internet is reachable (any of the globals populated)
        bool netOk = (bitcoin_price > 0 || current_block.length() > 0 || gData.globalHash.length() > 0);
        
        // Get chip ID
        uint64_t chipid = ESP.getEfuseMac();
        String chipID = String((uint32_t)(chipid >> 32), HEX) + String((uint32_t)chipid, HEX);
        chipID.toUpperCase();
        
        // Get temperature
        float temp = temperatureRead();
        
        // Get worker hash rates
        float workerRates[MINER_WORKER_COUNT] = {0};
        getWorkerHashRates(workerRates, MINER_WORKER_COUNT);
        double totalWorkerRate = 0.0;
        for (int i = 0; i < MINER_WORKER_COUNT; ++i) {
            totalWorkerRate += workerRates[i];
        }
        
        // Get 30-second rolling average hash rate
        float rollingAvgHashRate = getAvgHashRate();
        
        // Get total uptime average hash rate
        float totalAvgHashRate = getTotalAvgHashRate();
        
        static unsigned long lastStatsTime = 0;
        unsigned long now = millis();
        unsigned long elapsed = now - lastStatsTime;
        if (elapsed == 0) elapsed = 1;
        
        // Use instantaneous total from individual workers
        double currentHashRate = totalWorkerRate;
        (void)elapsed; // keep variable to avoid warnings
        lastStatsTime = now;
        
        // Update time
        timeClient.update();
        String currentTime = timeClient.getFormattedTime();
        
        // Format uptime as d:hh:mm:ss
        uint64_t totalSeconds = upTime;
        uint32_t days = totalSeconds / 86400;
        uint32_t hours = (totalSeconds % 86400) / 3600;
        uint32_t minutes = (totalSeconds % 3600) / 60;
        uint32_t seconds = totalSeconds % 60;
        String uptime = String(days) + "d " + 
                       (hours < 10 ? "0" : "") + String(hours) + ":" +
                       (minutes < 10 ? "0" : "") + String(minutes) + ":" +
                       (seconds < 10 ? "0" : "") + String(seconds);
        
        // Format best difficulty with suffix
        String bestDiffStr;
        if (best_diff >= 1e12) {
            bestDiffStr = String(best_diff / 1e12, 3) + "T";
        } else if (best_diff >= 1e9) {
            bestDiffStr = String(best_diff / 1e9, 3) + "B";
        } else if (best_diff >= 1e6) {
            bestDiffStr = String(best_diff / 1e6, 3) + "M";
        } else if (best_diff >= 1e3) {
            bestDiffStr = String(best_diff / 1e3, 3) + "K";
        } else {
            bestDiffStr = String(best_diff, 3);
        }
        
        // Calculate halving blocks (next halving at block 1,050,000)
        uint32_t currentBlockNum = current_block.toInt();
        uint32_t nextHalving = 1050000;
        uint32_t blocksToHalving = (currentBlockNum < nextHalving) ? (nextHalving - currentBlockNum) : 0;
        
        // Build worker rates JSON array (numbers, not strings)
        String workerRatesJson = "[";
        for (int i = 0; i < MINER_WORKER_COUNT; i++) {
            if (i > 0) workerRatesJson += ",";
            // Ensure numeric with two decimals
            workerRatesJson += String(workerRates[i], 2);
        }
        workerRatesJson += "]";
        
        // Safe defaults for missing globals
        unsigned int safe_btcprice = bitcoin_price; // default 0 if not set
        String safe_current_block = current_block.length() ? current_block : String("");
        String safe_globalhash = gData.globalHash.length() ? (gData.globalHash + " EH/s") : String("");
        String safe_difficulty = gData.difficulty.length() ? gData.difficulty : String("");
        unsigned int safe_fee = gData.halfHourFee;
        uint32_t safe_blocksToHalving = blocksToHalving;
        float safe_temp = isfinite(temp) ? temp : 0.0f;

        String poolName = Settings.PoolAddress.length() ? Settings.PoolAddress : String("");
        if (poolName.length() && Settings.PoolPort > 0) {
            poolName += ":" + String(Settings.PoolPort);
        }
        String json = "{\"hashrate\":" + String(currentHashRate, 2) + 
                      ",\"avghashrate\":" + String(rollingAvgHashRate, 2) +
                      ",\"totalavghashrate\":" + String(totalAvgHashRate, 2) +
                      ",\"workers\":" + workerRatesJson +
                  ",\"shares\":" + String(shares) + 
                  ",\"valids\":" + String(valids) +
                  ",\"templates\":" + String(templates) +
                  ",\"mhashes\":" + String(Mhashes) +
                      ",\"uptime\":\"" + uptime + "\"" +
                      ",\"bestdiff\":\"" + bestDiffStr + "\"" +
                      ",\"blocks\":0" +
                  ",\"btcprice\":" + String(safe_btcprice) +
                  ",\"currentblock\":\"" + safe_current_block + "\"" +
                      ",\"time\":\"" + currentTime + "\"" +
                  ",\"globalhash\":\"" + safe_globalhash + "\"" +
                  ",\"difficulty\":\"" + safe_difficulty + "\"" +
                  ",\"mediumfee\":" + String(safe_fee) +
                      ",\"halving\":" + String(safe_blocksToHalving) +
                      ",\"chipid\":\"" + chipID + "\"" +
                      ",\"temp\":" + String(safe_temp, 1) +
                      ",\"pool\":\"" + poolName + "\"" +
                      ",\"netok\":" + String(netOk ? 1 : 0) + "}";
        wm.server->sendHeader("Access-Control-Allow-Origin", "*");
        wm.server->sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        wm.server->sendHeader("Access-Control-Allow-Headers", "*");
        wm.server->send(200, "application/json", json);
    });
    
    // Settings page with pool configuration
    wm.server->on("/settings", [](){
        // Get unique ESP32 chip ID
        uint64_t chipid = ESP.getEfuseMac();
        String chipID = String((uint32_t)(chipid >> 32), HEX) + String((uint32_t)chipid, HEX);
        chipID.toUpperCase();
        
        String html = "<!DOCTYPE html><html><head>"
            "<meta charset='UTF-8'>"
            "<meta name='viewport' content='width=device-width,initial-scale=1'>"
            "<title>NerdMiner Dashboard</title>"
            "<style>"
            "*{margin:0;padding:0;box-sizing:border-box}"
            "body{font-family:'Courier New',monospace;background:#000;color:#0f0;padding:10px}"
            ".container{max-width:800px;margin:0 auto}"
            ".header{text-align:center;padding:20px;border:3px solid #0f0;margin-bottom:15px;background:#001100}"
            ".header h1{font-size:28px;margin-bottom:5px;letter-spacing:2px}"
            ".header .chipid{font-size:12px;color:#0a0;letter-spacing:1px}"
            ".dashboard{display:grid;grid-template-columns:1fr 1fr;gap:15px;margin-bottom:15px}"
            ".stat-box{border:2px solid #0f0;padding:15px;background:#001a00;text-align:center}"
            ".stat-box.large{grid-column:1/-1}"
            ".stat-label{font-size:11px;color:#0a0;margin-bottom:5px;text-transform:uppercase}"
            ".stat-value{font-size:36px;font-weight:bold;color:#0f0;font-family:monospace}"
            ".stat-value.mega{font-size:48px}"
            ".stat-small{font-size:20px}"
            ".settings-panel{border:3px solid #0f0;background:#001100;padding:20px}"
            ".settings-panel h2{text-align:center;margin-bottom:20px;font-size:20px;border-bottom:2px solid #0f0;padding-bottom:10px}"
            ".form-group{margin-bottom:15px}"
            "label{display:block;font-size:12px;margin-bottom:5px;color:#0f0;text-transform:uppercase}"
            "input,select{width:100%;padding:10px;background:#000;color:#0f0;border:2px solid #0f0;font-family:'Courier New',monospace;font-size:14px}"
            "input:focus,select:focus{outline:none;border-color:#0ff;background:#001a1a}"
            ".btn{width:100%;padding:15px;background:#0f0;color:#000;border:none;font-size:16px;font-weight:bold;"
            "cursor:pointer;font-family:'Courier New',monospace;text-transform:uppercase;letter-spacing:2px;margin-top:10px}"
            ".btn:hover{background:#0ff;box-shadow:0 0 10px #0f0}"
            ".checkbox-inline{display:inline-block;width:auto;margin-left:10px}"
            "@media(max-width:600px){.dashboard{grid-template-columns:1fr}.stat-box.large{grid-column:1}}"
            "</style></head><body>"
            "<div class='container'>"
            "<div class='header'>"
            "<h1>NERD MINER</h1>"
            "<div class='chipid'>CHIP ID: " + chipID + "</div>"
            "</div>"
            "<div class='dashboard'>"
            "<div class='stat-box'>"
            "<div class='stat-label'>Block Templates</div>"
            "<div class='stat-value stat-small' id='templates'>0</div>"
            "</div>"
            "<div class='stat-box'>"
            "<div class='stat-label'>Best Difficulty</div>"
            "<div class='stat-value stat-small' id='bestdiff'>0</div>"
            "</div>"
            "<div class='stat-box'>"
            "<div class='stat-label'>32 Bit Shares</div>"
            "<div class='stat-value stat-small' id='shares'>0</div>"
            "</div>"
            "<div class='stat-box'>"
            "<div class='stat-label'>Valid Blocks</div>"
            "<div class='stat-value stat-small' id='valids'>0</div>"
            "</div>"
            "<div class='stat-box'>"
            "<div class='stat-label'>Blocks Mined</div>"
            "<div class='stat-value stat-small' id='blocks'>0</div>"
            "</div>"
            "<div class='stat-box'>"
            "<div class='stat-label'>Device Uptime</div>"
            "<div class='stat-value stat-small' id='uptime'>0d 00:00:00</div>"
            "</div>"
            "<div class='stat-box large'>"
            "<div class='stat-label'>Current Hash Rate (KH/s)</div>"
            "<div class='stat-value mega' id='hashrate'>---</div>"
            "</div>"
            "<div class='stat-box'>"
            "<div class='stat-label'>Total Hashes (M)</div>"
            "<div class='stat-value stat-small' id='mhashes'>0</div>"
            "</div>"
            "<div class='stat-box'>"
            "<div class='stat-label'>Current Pool</div>"
            "<div class='stat-value stat-small' id='pool'>POOL</div>"
            "</div>"
            "</div>"
            "<div class='settings-panel'>"
            "<h2>⚙ CONFIGURATION ⚙</h2>"
            "<form action='/savesettings' method='POST'>"
            "<div class='form-group'>"
            "<label for='pool_preset'>⚡ Quick Pool Select</label>"
            "<select id='pool_preset' onchange='setPool()'>"
            "<option value=''>-- Select Pool --</option>"
            "<option value='public-pool.io:21496'>Public Pool (public-pool.io:21496)</option>"
            "<option value='solo.ckpool.org:3333'>CKPool Solo (solo.ckpool.org:3333)</option>"
            "<option value='pool.vkbit.com:3333'>VKBit (pool.vkbit.com:3333)</option>"
            "<option value='stratum.braiins.com:3333'>Braiins Pool (stratum.braiins.com:3333)</option>"
            "<option value='us1.btcpaypool.com:3333'>BTC Pay Pool (us1.btcpaypool.com:3333)</option>"
            "<option value='pool.tazmining.ch:33333'>Taz Mining (pool.tazmining.ch:33333)</option>"
            "</select></div>"
            "<div class='form-group'>"
            "<label for='ssid'>📡 WiFi SSID</label>"
            "<input type='text' id='ssid' name='ssid' value='IoT' required>"
            "</div>"
            "<div class='form-group'>"
            "<label for='wifipass'>🔑 WiFi Password<input type='checkbox' onclick='togglePass()' class='checkbox-inline'> Show</label>"
            "<input type='password' id='wifipass' name='wifipass' value='Elatedsun502' required>"
            "</div>"
            "<div class='form-group'>"
            "<label for='poolurl'>🌐 Pool URL</label>"
            "<input type='text' id='poolurl' name='poolurl' value='";
        html += Settings.PoolAddress;
        html += "' required></div>"
            "<div class='form-group'>"
            "<label for='poolport'>🔌 Pool Port</label>"
            "<input type='number' id='poolport' name='poolport' value='";
        html += String(Settings.PoolPort);
        html += "' required></div>"
            "<div class='form-group'>"
            "<label for='poolpass'>🔐 Pool Password</label>"
            "<input type='text' id='poolpass' name='poolpass' value='";
        html += String(Settings.PoolPassword);
        html += "'></div>"
            "<div class='form-group'>"
            "<label for='btcwallet'>₿ BTC Wallet Address</label>"
            "<input type='text' id='btcwallet' name='btcwallet' value='";
        html += String(Settings.BtcWallet);
        html += "' required></div>"
            "<div class='form-group'>"
            "<label for='timezone'>🕐 Timezone (UTC offset)</label>"
            "<input type='number' id='timezone' name='timezone' value='";
        html += (Settings.Timezone != 0) ? String(Settings.Timezone) : "2";
        html += "'></div>"
            "<button type='submit' class='btn'>💾 Save &amp; Restart</button>"
            "</form></div></div>"
            "<script>"
            "function setPool(){"
            "var p=document.getElementById('pool_preset').value;"
            "if(p){"
            "var s=p.split(':');"
            "document.getElementById('poolurl').value=s[0];"
            "document.getElementById('poolport').value=s[1];"
            "}"
            "}"
            "function togglePass(){"
            "var x=document.getElementById('wifipass');"
            "x.type=x.type==='password'?'text':'password';"
            "}"
            "function updateStats(){"
            "fetch('/api/stats').then(r=>r.json()).then(d=>{"
            "document.getElementById('hashrate').textContent=d.hashrate.toFixed(2);"
            "document.getElementById('shares').textContent=d.shares;"
            "document.getElementById('valids').textContent=d.valids;"
            "document.getElementById('templates').textContent=d.templates;"
            "document.getElementById('mhashes').textContent=d.mhashes;"
            "document.getElementById('uptime').textContent=d.uptime;"
            "document.getElementById('bestdiff').textContent=d.bestdiff;"
            "document.getElementById('blocks').textContent=d.blocks;"
            "}).catch(e=>console.log(e));"
            "}"
            "function updatePool(){"
            "var url=document.getElementById('poolurl').value;"
            "var port=document.getElementById('poolport').value;"
            "var display=url.split('.')[0].toUpperCase();"
            "document.getElementById('pool').textContent=display;"
            "}"
            "updateStats();"
            "setInterval(updateStats,2000);"
            "updatePool();"
            "document.getElementById('poolurl').addEventListener('input',updatePool);"
            "</script>"
            "</body></html>";
        wm.server->send(200, "text/html; charset=UTF-8", html);
    });
    
    // Save settings handler
    wm.server->on("/savesettings", HTTP_POST, [](){
        // Add CORS headers
        wm.server->sendHeader("Access-Control-Allow-Origin", "*");
        wm.server->sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        wm.server->sendHeader("Access-Control-Allow-Headers", "Content-Type");
        
        if(wm.server->hasArg("poolurl")) {
            // Save WiFi credentials
            Settings.WifiSSID = wm.server->arg("ssid");
            Settings.WifiPW = wm.server->arg("wifipass");
            
            // Save pool settings
            Settings.PoolAddress = wm.server->arg("poolurl");
            Settings.PoolPort = wm.server->arg("poolport").toInt();
            strncpy(Settings.PoolPassword, wm.server->arg("poolpass").c_str(), sizeof(Settings.PoolPassword));
            strncpy(Settings.BtcWallet, wm.server->arg("btcwallet").c_str(), sizeof(Settings.BtcWallet));
            Settings.Timezone = wm.server->arg("timezone").toInt();
            
            nvMem.saveConfig(&Settings);
            
            wm.server->send(200, "text/html; charset=UTF-8", 
                "<html><head><meta charset='UTF-8'></head>"
                "<body style='background:#1a1a1a;color:#fff;font-family:Arial;text-align:center;padding:50px'>"
                "<h1 style='color:#00ff00'>Settings Saved!</h1>"
                "<p>Device will restart in 3 seconds...</p>"
                "<p>Mining will resume with new pool settings.</p>"
                "</body></html>"
            );
            delay(3000);
            ESP.restart();
        } else {
            wm.server->send(400, "text/html; charset=UTF-8", 
                "<html><head><meta charset='UTF-8'></head>"
                "<body style='background:#1a1a1a;color:#f00;font-family:Arial;text-align:center;padding:50px'>"
                "<h1>Error: Missing Parameters</h1>"
                "<p><a href='/settings' style='color:#00ff00'>Go Back</a></p>"
                "</body></html>");
        }
    });
}
