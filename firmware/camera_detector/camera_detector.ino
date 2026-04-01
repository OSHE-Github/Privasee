#include <SD.h>
#include <SPI.h>
#include "WiFi.h"
#include "esp_wifi.h"

// Configs
const int serialInit = 921600;

const int IRAnalogPin = 4;
const int buzz_pin = 6;

const int readEntries = 100;
const int sampleDelayUS = 50;   // microseconds between samples

// SD card
int sck = 22;
int miso = 21;
int mosi = 20;
int cs = 19;
char logFile[32];
char packetLog[32];

// Globals
int baseline = 0;
int noise = 0;
bool calibrated = false;
unsigned long lastIRcall = 0;

unsigned long lastTrigger = 0;
const int triggerCooldown = 500; // ms

// Wifi Globals/constants
// #define NUM_TRACKED_ADDR 10 unneeded atm
#define MAX_QUEUE_LENGTH 20
#define MIN_PACKET_LENGTH 32
#define PACKET_LED RGB_BUILTIN
#define PACKET_LED_PULSE 100
unsigned long packetLED_state; // time of last enable (ms)

// Filter for any wifi packet
wifi_promiscuous_filter_t filter = {
    .filter_mask = WIFI_PROMIS_FILTER_MASK_DATA | WIFI_PROMIS_FILTER_MASK_MGMT | WIFI_PROMIS_FILTER_MASK_CTRL,
};

typedef struct {
  uint8_t mac_addr[6];
  int32_t rssi;
  uint32_t length;
} PacketInfo;

// Queue for packet processing
QueueHandle_t packet_queue;

void setup() {
  Serial.begin(serialInit);
  pinMode(buzz_pin, OUTPUT);

  // SD card setup
  Serial.println("Initializing SPI...");
  if (SPI.begin(sck, miso, mosi, cs) == 0) {
    Serial.println("SPI init failed!");
  }
  Serial.println("Initializing SD card...");
  if (!SD.begin(cs)) {
    Serial.println("SD card failed!");
    //while (true); // stop here
  }
  Serial.println("SD card initialized.");
  
  uint8_t cardType = SD.cardType();

  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }

  // find unused logfile name & save. 
  // will open/close upon each write
  Serial.println("Finding new IR log file");
  snprintf(logFile, sizeof(logFile), "/log_%02d.txt", 1);
  uint8_t i = 0;
  while (SD.exists(logFile) == 1) {
    i++;
    snprintf(logFile, sizeof(logFile), "/log_%02d.txt", i);
    if (i > 99) {
      Serial.println("LOGS ARE FULL. PLEASE CLEAR");
      snprintf(logFile, sizeof(logFile), "/log_%02d.txt", 0);;
      break;
    }
    // delay(5);
  }
  Serial.println("empty IR log file found");

  // find second log file
  Serial.println("Finding new packet log file");
  snprintf(packetLog, sizeof(packetLog), "/packet_%02d.txt", 1);
  i = 0;
  while (SD.exists(packetLog) == 1) {
    i++;
    snprintf(packetLog, sizeof(packetLog), "/packet_%02d.txt", i);
    if (i > 99) {
      Serial.println("LOGS ARE FULL. PLEASE CLEAR");
      snprintf(packetLog, sizeof(packetLog), "/packet_%02d.txt", 0);;
      break;
    }
    // delay(5);
  }
  Serial.println("Packet log file found");

  // Write Header
  File file = SD.open(logFile, FILE_WRITE);
  if (file) {
    if (!file.println("time,value")) // CSV header
    {
      Serial.println("Failed to write headers");
    }
    file.flush();
  } else {
    Serial.println("Failed to open file.");
  }
  file.close();
  Serial.print(logFile);
  Serial.println(" created and initailized");

  // Calibrate IR
  Serial.println("Starting IR calibration...");
  calibrateIR();

  // Wifi Boilerplate
  WiFi.mode(WIFI_MODE_STA);
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_filter(&filter);
  esp_wifi_set_promiscuous_rx_cb(&sniffer_callback);
  Serial.println(esp_wifi_start());
  
  // Initailize Queue
  packet_queue = xQueueCreate(MAX_QUEUE_LENGTH, sizeof(PacketInfo));
  if (packet_queue == NULL) {
    Serial.println("Error initalizing queue");
    while (true) {};
  }
  packetLED_state = 0;

  //funny stuff
  postCalibrate();
  Serial.println("Ready.");
  // while (true) {}
}

void loop() {
  //if (!calibrated) return;
  if ((millis() - packetLED_state) >= PACKET_LED_PULSE) {
    digitalWrite(PACKET_LED, LOW); 
  }

  if ((millis() - lastIRcall) > 50) {
    IR(); // TODO: Run every 50 ms
    Serial.println();
    lastIRcall = millis();
  }

  int packets_available = uxQueueMessagesWaiting(packet_queue);
  if (packets_available > 0) {
    // Read Packet
    PacketInfo pkt_info;
    if (xQueueReceive(packet_queue, &pkt_info, 0) == errQUEUE_EMPTY) {
      Serial.println("NOTHINH IN QUEUE. THIS SHOULDNT RUN. EVER.");
      return;
    }
    logPacket(pkt_info);
    printPacket(pkt_info);
  }
  
  //Serial.println("LOOP");
  // Serial.println();
}

void logPacket(PacketInfo pkt_info) {
  File file = SD.open(packetLog, FILE_APPEND);
  if (packetLog) {
    file.print("Packet | ");
    for (int i = 0; i < 6; i++) {
      file.print(pkt_info.mac_addr[i], HEX);
      file.print(" ");
    }
    file.print("| rssi: "); file.print(pkt_info.rssi); file.print(" length: "); file.print(pkt_info.length);
    file.println();
    file.flush();
  }
  file.close();
}

void printPacket(PacketInfo pkt_info) {
    // Display Individual Packets
    Serial.print("Packet | ");
    for (int i = 0; i < 6; i++) {
      Serial.print(pkt_info.mac_addr[i], HEX);
      Serial.print(" ");
    }
    Serial.print("| rssi: "); Serial.print(pkt_info.rssi); Serial.print(" length: "); Serial.print(pkt_info.length);
    Serial.println();
}

// Calibration function
void calibrateIR() {
  long sum = 0;
  int minVal = 4095;
  int maxVal = 0;

  for (int i = 0; i < 500; i++) {
    int val = analogRead(IRAnalogPin);
    sum += val;
    if (val < minVal) minVal = val;
    if (val > maxVal) maxVal = val;
    delay(2);
  }

  baseline = sum / 500;
  noise = maxVal - minVal;

  Serial.println("Calibration complete:");
  Serial.print("Baseline: "); 
  Serial.println(baseline);
  Serial.print("Noise: "); 
  Serial.println(noise);
  
  calibrated = true;
}   

// IR detection
bool IR() {
  int maxVal = 0;
  int minVal = 4095;

  for (int i = 0; i < readEntries; i++) {
    int val = analogRead(IRAnalogPin);

    if (val > maxVal) maxVal = val;
    if (val < minVal) minVal = val;

    //logData(micros(), val); // Fixed function call

    delayMicroseconds(sampleDelayUS);
  }

  int peak = maxVal - baseline;
  int dip = baseline - minVal;
  int threshold = noise * 5 + 100; // sensitivity
  /*Serial.print("Baseline: ");
  Serial.print(baseline);
  Serial.print(" | noise: ");
  Serial.print(noise);
  Serial.print(" | Peak: "); 
  Serial.print(peak);
  Serial.print(" | Dip: "); 
  Serial.print(dip);
  Serial.print(" | Threshold: "); 
  Serial.println(threshold);  
  */
  // Cooldown check (optional)
  // if (millis() - lastTrigger < triggerCooldown) return false;

  if (peak > threshold || dip > threshold) {
    Serial.println("DETECTED");

    Serial.print(baseline);
    Serial.print(" | noise: ");
    Serial.print(noise);
    Serial.print(" | Peak: "); 
    Serial.print(peak);
    Serial.print(" | Dip: "); 
    Serial.print(dip);
    Serial.print(" | Threshold: "); 
    Serial.println(threshold);  
    
    int buzzVal = (((double)baseline/maxVal)*256+10);
    if(dip * -1 > peak){
      buzzVal = ((double)minVal/baseline)*256+10;
    }
    if(buzzVal > 256){
      buzzVal = 256 ;
    }
    analogWrite(buzz_pin, buzzVal);

    Serial.print("Buzz: "); 
    Serial.println(buzzVal);  

    lastTrigger = millis();

    //name requested by Jordan Chopskie
    char Harold[1024];
    sprintf(Harold, " DETECTED IR \t max : %d | min : %d | base : %d", maxVal, minVal, baseline);

    logEvent(Harold);
    delay(25);
    return true;
  } else {
    analogWrite(buzz_pin, 0);
    return false;
  }
}

// Logging sensor values
void logData(unsigned long time, int value) {
  Serial.print(time);
  Serial.print(",");
  Serial.println(value);
  File file = SD.open(logFile, FILE_APPEND);
  if (logFile) {
    file.print(time);
    file.print(",");
    file.println(value);
    file.flush();
  }
  file.close();
}

// Logging events
void logEvent(const char* event) {
  File file = SD.open(logFile, FILE_APPEND);
  if (logFile) {
    file.print(millis());
    file.print(",EVENT:");
    file.println(event);
    file.flush();
  }
  file.close();
}

void sniffer_callback(void* buf, wifi_promiscuous_pkt_type_t type) {
  // This check is technically unneccicary due to filter config, but ima leave it in for now.
  if (type == WIFI_PKT_MISC) {
    Serial.println("MISC PACKET");
    return;
  }
  const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buf;
  // Make sure we wont go out of bounds
  int pkt_len = ppkt->rx_ctrl.sig_len;
  if (pkt_len <= MIN_PACKET_LENGTH) {
    return;
  }
  // Save info to tmp variable
  PacketInfo pkt_info;
  for (int i = 0; i < 6; i++) {
    // 10 is the offset of the source mac address in a MAC header
    pkt_info.mac_addr[i] = ppkt->payload[10+i];
  }
  pkt_info.rssi = ppkt->rx_ctrl.rssi;
  pkt_info.length = ppkt->rx_ctrl.sig_len;
  // push to queue
  xQueueSendToBackFromISR(packet_queue, &pkt_info, NULL);
  // toggle LED
  digitalWrite(PACKET_LED, HIGH); 
  packetLED_state = millis();
  logEvent("Packet Recieved");

  // Serial.print("Packet | ");
  // for (int i = 0; i < 6; i++) {
  //   if (10+i >= ppkt->rx_ctrl.sig_len)
  //   {
  //     Serial.print("XX ");
  //     continue;
  //   }
  //   Serial.print(ppkt->payload[10+i], HEX);
  //   Serial.print(" ");
  // }
  // Serial.print("| rssi: "); Serial.print(ppkt->rx_ctrl.rssi); Serial.print(" length: "); Serial.print(ppkt->rx_ctrl.sig_len);
  // Serial.println();
}


//ignore
void postCalibrate(){
  analogWrite(buzz_pin, 255);
  delay(350);
  analogWrite(buzz_pin, 0); // stop
  delay(100);
  analogWrite(buzz_pin, 40);
  delay(350);
  analogWrite(buzz_pin, 0); // stop
  delay(100);
  analogWrite(buzz_pin, 100);
  delay(350);
  analogWrite(buzz_pin, 0); // stop
}
