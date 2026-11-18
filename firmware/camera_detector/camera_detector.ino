#include <SD.h>
#include <SPI.h>
#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ============================================================
// CONFIGURATION: WIFI & PINS
// ============================================================

const char* ssid = "MilfHouse";         // Your WiFi SSID
const char* password = "shortpant372";  // Your WiFi Password

// GPIO Mapping
const int IRAnalogPin = -1;  // IR Sensor Pin (-1 to disable)
const int RFAnalogPin = -1;  // RF Sensor Pin (-1 to disable)
const int buzz_pin = -1;     // Buzzer Pin (-1 to disable)
const int buttonPin = 28;    // ESP32-C5 BOOT Button Pin

// SD Card SPI Pins (-1 to disable)
const int sck = -1;
const int miso = -1;
const int mosi = -1;
const int cs = -1;

// I2C LCD Configuration (PCF8574T)
#define LCD_SDA 23
#define LCD_SCL 24
#define LCD_ADDRESS 0x27
#define LCD_COLUMNS 16
#define LCD_ROWS 2

LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLUMNS, LCD_ROWS);

// ============================================================
// SYSTEM GLOBALS & STATE VARIABLES
// ============================================================

// Mode Selection (1 = IR, 2 = RF, 3 = WiFi)
int button = 1;
const unsigned long buttonDebounce = 250;
bool lastButtonState = HIGH;
unsigned long lastButtonPress = 0;

// System Flags
bool sdReady = false;
bool wifiConnected = false;
bool wifiMonitoring = false;

// IR Sensor States
int baseline = 0;
int noise = 0;
bool calibrated = false;
unsigned long lastIRcall = 0;
unsigned long lastTrigger = 0;
const int triggerCooldown = 500;
const int readEntries = 100;
const int sampleDelayUS = 50;

// Log File Names
char logFile[32];
char packetLog[32];

// WiFi & Traffic Statistics
uint8_t apMac[6] = { 0 };
uint8_t monitorChannel = 0;

volatile uint64_t downloadBytes = 0;
volatile uint64_t uploadBytes = 0;
volatile uint32_t downloadPackets = 0;
volatile uint32_t uploadPackets = 0;

portMUX_TYPE trafficMux = portMUX_INITIALIZER_UNLOCKED;

uint64_t lastDownloadBytes = 0;
uint64_t lastUploadBytes = 0;
unsigned long lastWifiUpdate = 0;

float downloadMbps = 0;
float uploadMbps = 0;
float totalMbps = 0;

// ============================================================
// FUNCTION DECLARATIONS
// ============================================================

// Display & Interface
void LCD_setup();
void writeToScreen(const char* line1, const char* line2);
void WiFi_display();
void checkModeButton();

// Sensors & Audio
void calibrateIR();
bool IR();
void RF();
void powerOn();
void postCalibrate();

// Storage (SD Card)
void createLogFiles();
void logData(unsigned long time, int value);
void logEvent(const char* event);

// Network & Monitoring
void connectWiFi();
void startWifiMonitor();
void stopWifiMonitor();
void updateWifiTraffic();
void wifiSnifferCallback(void* buf, wifi_promiscuous_pkt_type_t type);
bool macEquals(const uint8_t* a, const uint8_t* b);
void printMac(const uint8_t* mac);

// ============================================================
// SETUP
// ============================================================

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println();
  Serial.println("================================");
  Serial.println("       PRIVASEE STARTING");
  Serial.println("================================");

  // Initialize Subsystems
  LCD_setup();
  delay(1000);

  pinMode(buttonPin, INPUT_PULLUP);
  Serial.println("Onboard BOOT button initialized.");

  if (buzz_pin >= 0) {
    pinMode(buzz_pin, OUTPUT);
    analogWrite(buzz_pin, 0);
    powerOn();
  }

  // Initialize SD Card if pins configured
  if (sck >= 0 && miso >= 0 && mosi >= 0 && cs >= 0) {
    SPI.begin(sck, miso, mosi, cs);
    if (!SD.begin(cs, SPI)) {
      writeToScreen("SD Card Error", "Check Wiring");
      delay(1500);
    } else {
      if (SD.cardType() == CARD_NONE) {
        writeToScreen("No SD Card", "Check Card");
      } else {
        sdReady = true;
        createLogFiles();
      }
    }
  }

  // Connect to Wi-Fi with timeout
  connectWiFi();

  // Calibrate IR Sensor if configured
  //if (IRAnalogPin >= 0) {
    writeToScreen("IR Calibration", "Keep Area Clear");
    delay(2000);
    calibrateIR();
    writeToScreen("IR DONE", "");
    delay(2000);
    if (buzz_pin >= 0) {
      postCalibrate();
    }
  //}

  // Ready State
  button = 0;
  writeToScreen("PRIVASEE READY", "PRESS BUTTON");
  Serial.println("Ready. Press BOOT to change modes.");
}

// ============================================================
// MAIN LOOP
// ============================================================

void loop() {
  checkModeButton();

  // Mode 1: IR Motion Detection
  if (button == 1) {
    if (millis() - lastIRcall >= 50) {
      IR();
      lastIRcall = millis();
    }
  }
  // Mode 2: RF Signal Detection
  else if (button == 2) {
    RF();
  }
  // Mode 3: WiFi Traffic Monitor
  else if (button == 3) {
    if (wifiConnected) {
      updateWifiTraffic();
    }
  }
}

// ============================================================
// LCD & DISPLAY FUNCTIONS
// ============================================================

void LCD_setup() {
  Wire.begin(LCD_SDA, LCD_SCL);
  Wire.setClock(100000);

  lcd.init();
  lcd.backlight();
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("PRIVASEE");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");
}

void writeToScreen(const char* line1, const char* line2) {
  lcd.setCursor(0, 0);
  lcd.print("                ");
  lcd.setCursor(0, 0);
  lcd.print(line1);

  lcd.setCursor(0, 1);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print(line2);
}

void WiFi_display() {
  if (!wifiConnected) {
    writeToScreen("WiFi Offline", "No Connection");
    return;
  }

  char line1[17];
  char line2[17];

  snprintf(line1, sizeof(line1), "DOWN:%5.2f Mbps", downloadMbps);
  snprintf(line2, sizeof(line2), "UP:  %5.2f Mbps", uploadMbps);

  writeToScreen(line1, line2);
}

// ============================================================
// USER INTERFACE & BUTTON CONTROL
// ============================================================

void checkModeButton() {
  bool currentButtonState = digitalRead(buttonPin);

  if (currentButtonState == LOW && lastButtonState == HIGH) {
    if (millis() - lastButtonPress > buttonDebounce) {
      lastButtonPress = millis();
      button++;

      if (button > 3) {
        button = 1;
      }

      Serial.println("\n==============================");

      switch (button) {
        case 1:
          stopWifiMonitor();
          writeToScreen("PRIVASEE", "Mode: IR");
          Serial.println("MODE 1: IR");
          break;
        case 2:
          stopWifiMonitor();
          writeToScreen("PRIVASEE", "Mode: RF");
          Serial.println("MODE 2: RF");
          break;
        case 3:
          if (wifiConnected) {
            startWifiMonitor();
            writeToScreen("WiFi Monitor", "Traffic Active");
            Serial.println("MODE 3: WIFI");
          } else {
            writeToScreen("WiFi Offline", "Check Network");
            Serial.println("WiFi unavailable.");
          }
          break;
      }
      Serial.println("==============================");
    }
  }
  lastButtonState = currentButtonState;
}

// ============================================================
// WIFI CONNECTION & MONITORING
// ============================================================

void connectWiFi() {
  Serial.println("Connecting to WiFi...");

  if (strlen(ssid) == 0) {
    wifiConnected = false;
    writeToScreen("WiFi Not Set", "Enter Credentials");
    return;
  }

  WiFi.mode(WIFI_STA);
  writeToScreen("Connecting WiFi", "Please Wait");
  WiFi.begin(ssid, password);

  unsigned long startTime = millis();
  unsigned long timeoutMillis = 10000;  // 10-second timeout

  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");

    if (millis() - startTime >= timeoutMillis) {
      Serial.println("\nWiFi connection timed out!");
      wifiConnected = false;
      WiFi.disconnect(true);
      writeToScreen("WiFi Failed", "Timeout / Offline");
      delay(2000);
      return;
    }
  }

  wifiConnected = true;
  Serial.println("\nWiFi connected.");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  uint8_t* bssid = WiFi.BSSID();
  if (bssid != nullptr) {
    memcpy(apMac, bssid, 6);
  }

  monitorChannel = WiFi.channel();
  esp_wifi_set_promiscuous(false);
  esp_wifi_set_promiscuous_rx_cb(&wifiSnifferCallback);

  writeToScreen("WiFi Connected", "Ready");
  delay(1000);
}

void startWifiMonitor() {
  if (!wifiConnected) return;
  wifiMonitoring = true;
  esp_wifi_set_promiscuous(true);
  Serial.println("Wi-Fi promiscuous monitor started.");
}

void stopWifiMonitor() {
  if (!wifiMonitoring) return;
  wifiMonitoring = false;
  esp_wifi_set_promiscuous(false);
  Serial.println("Wi-Fi promiscuous monitor stopped.");
}

void updateWifiTraffic() {
  if (!wifiMonitoring) return;

  portENTER_CRITICAL(&trafficMux);
  uint64_t curDownBytes = downloadBytes;
  uint64_t curUpBytes = uploadBytes;
  portEXIT_CRITICAL(&trafficMux);

  unsigned long currentMillis = millis();
  if (currentMillis - lastWifiUpdate >= 1000) {
    unsigned long elapsed = currentMillis - lastWifiUpdate;
    lastWifiUpdate = currentMillis;

    uint64_t diffDown = curDownBytes - lastDownloadBytes;
    uint64_t diffUp = curUpBytes - lastUploadBytes;

    lastDownloadBytes = curDownBytes;
    lastUploadBytes = curUpBytes;

    downloadMbps = (diffDown * 8.0) / (elapsed / 1000.0) / 1000000.0;
    uploadMbps = (diffUp * 8.0) / (elapsed / 1000.0) / 1000000.0;
    totalMbps = downloadMbps + uploadMbps;

    WiFi_display();
  }
}

void wifiSnifferCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
  if (!wifiMonitoring) return;
  wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
  uint32_t pktLen = pkt->rx_ctrl.sig_len;

  portENTER_CRITICAL(&trafficMux);
  downloadBytes += pktLen;
  downloadPackets++;
  portEXIT_CRITICAL(&trafficMux);
}

// ============================================================
// SENSORS & PERIPHERALS (IR, RF, AUDIO, SD)
// ============================================================

// ============================================================
// UPGRADED IR CALIBRATION
// ============================================================

void calibrateIR() {
  if (IRAnalogPin < 0) {
    Serial.println("IR sensor not configured. Skipping calibration.");
    return;
  }

  long sum = 0;
  int minVal = 4095;
  int maxVal = 0;

  Serial.println("Starting IR calibration...");
  writeToScreen("IR Calibration", "Keep Area Clear");

  // Sample over 1 second (500 samples * 2ms) to filter out AC lighting flicker
  for (int i = 0; i < 500; i++) {
    int val = analogRead(IRAnalogPin);
    sum += val;

    if (val < minVal) {
      minVal = val;
    }
    if (val > maxVal) {
      maxVal = val;
    }

    delay(2);
  }

  baseline = sum / 500;

  // Calculate raw environmental noise spread
  noise = maxVal - minVal;

  // SAFETY FLOOR: Ensure noise is never too low (prevents hyper-sensitivity in dead-quiet rooms)
  if (noise < 10) {
    noise = 10;
  }

  calibrated = true;

  Serial.println("Calibration complete:");
  Serial.print("Baseline: ");
  Serial.println(baseline);
  Serial.print("Noise Floor: ");
  Serial.println(noise);
}


// ============================================================
// UPGRADED IR DETECTION
// ============================================================

bool IR() {
  if (!calibrated || IRAnalogPin < 0) {
    if (buzz_pin >= 0) analogWrite(buzz_pin, 0);
    return false;
  }

  // Enforce cooldown between triggers
  if (millis() - lastTrigger < triggerCooldown) {
    if (buzz_pin >= 0) analogWrite(buzz_pin, 0);
    return false;
  }

  int maxVal = 0;
  int minVal = 4095;

  // Fast sampling window
  for (int i = 0; i < readEntries; i++) {
    int val = analogRead(IRAnalogPin);

    if (val > maxVal) maxVal = val;
    if (val < minVal) minVal = val;

    delayMicroseconds(sampleDelayUS);
  }

  int peak = maxVal - baseline;
  int dip = baseline - minVal;

  // Optimized adaptive threshold formula
  int threshold = (noise * 4) + 50;

  if (peak > threshold || dip > threshold) {
    Serial.println("DETECTED IR");
    Serial.print("Base: ");
    Serial.print(baseline);
    Serial.print(" | Noise: ");
    Serial.print(noise);
    Serial.print(" | Peak: ");
    Serial.print(peak);
    Serial.print(" | Dip: ");
    Serial.print(dip);
    Serial.print(" | Thresh: ");
    Serial.println(threshold);

    // Calculate dynamic buzzer intensity
    int buzzVal = 0;
    if (maxVal > 0) {
      buzzVal = (int)(((double)baseline / maxVal) * 256 + 10);
    }
    if (dip > peak && baseline > 0) {
      buzzVal = (int)(((double)minVal / baseline) * 256 + 10);
    }
    buzzVal = constrain(buzzVal, 0, 255);

    if (buzz_pin >= 0) {
      analogWrite(buzz_pin, buzzVal);
    }

    lastTrigger = millis();

    // Log event to SD card if ready
    char eventText[128];
    snprintf(
      eventText,
      sizeof(eventText),
      "DETECTED IR max:%d min:%d base:%d peak:%d dip:%d",
      maxVal,
      minVal,
      baseline,
      peak,
      dip);

    logEvent(eventText);
    writeToScreen("IR DETECTED!", "Check Area");

    delay(25);

    if (buzz_pin >= 0) {
      analogWrite(buzz_pin, 0);
    }

    return true;
  }

  // Clear buzzer if no trigger
  if (buzz_pin >= 0) {
    analogWrite(buzz_pin, 0);
  }

  return false;
}

void RF() {
  if (RFAnalogPin < 0) return;
  int rfVal = analogRead(RFAnalogPin);

  if (rfVal > 200) {
    if (millis() - lastTrigger > triggerCooldown) {
      lastTrigger = millis();
      writeToScreen("RF Alert!", "Signal Detected");

      if (buzz_pin >= 0) tone(buzz_pin, 1500, 200);
      if (sdReady) logEvent("RF Signal Detected");
    }
  }
}

void createLogFiles() {
  if (!sdReady) return;
  strcpy(logFile, "/syslog.txt");
  strcpy(packetLog, "/pktlog.txt");

  if (!SD.exists(logFile)) {
    File f = SD.open(logFile, FILE_WRITE);
    if (f) {
      f.println("System Log Started");
      f.close();
    }
  }
}

void logData(unsigned long time, int value) {
  if (!sdReady) return;
  File f = SD.open(logFile, FILE_APPEND);
  if (f) {
    f.print(time);
    f.print(",");
    f.println(value);
    f.close();
  }
}

void logEvent(const char* event) {
  if (!sdReady) return;
  File f = SD.open(logFile, FILE_APPEND);
  if (f) {
    f.print(millis());
    f.print(": ");
    f.println(event);
    f.close();
  }
}

void powerOn() {
  if (buzz_pin < 0) return;
  tone(buzz_pin, 800, 150);
  delay(150);
  tone(buzz_pin, 1200, 150);
}

void postCalibrate() {
  if (buzz_pin < 0) return;
  tone(buzz_pin, 1500, 200);
}

bool macEquals(const uint8_t* a, const uint8_t* b) {
  for (int i = 0; i < 6; i++) {
    if (a[i] != b[i]) return false;
  }
  return true;
}

void printMac(const uint8_t* mac) {
  for (int i = 0; i < 6; i++) {
    if (mac[i] < 16) Serial.print("0");
    Serial.print(mac[i], HEX);
    if (i < 5) Serial.print(":");
  }
  Serial.println();
}
