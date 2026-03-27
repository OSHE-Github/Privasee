#include <SD.h>
#include <SPI.h>

// Configs
const int serialInit = 921600;

// pins only used IR
const int IRAnalogPin = 4;
const int buzz_pin = 6;

const int readEntries = 100;
const int sampleDelayUS = 50;   // microseconds between samples

// SD card
const int chipSelect = 5;       // Change to your SD card CS pin
File logFile;

// Globals
int baseline = 0;
int noise = 0;
bool calibrated = false;

unsigned long lastTrigger = 0;
const int triggerCooldown = 500; // ms

void setup() {
  Serial.begin(serialInit);
  pinMode(buzz_pin, OUTPUT);

  // SD card setup
  Serial.println("Initializing SD card...");
  if (!SD.begin(chipSelect)) {
    Serial.println("SD card failed!");
    //while (true); // stop here
  }
  Serial.println("SD card initialized.");

  logFile = SD.open("/log.csv", FILE_WRITE);
  if (logFile) {
    logFile.println("time,value"); // CSV header
    logFile.flush();
  }

  // Calibrate IR
  Serial.println("Starting calibration...");
  calibrateIR();

  //funny stuff
  postCalibrate();
  Serial.println("Ready.");
}

void loop() {
  //if (!calibrated) return;

  IR();
  delay(50);  // small loop delay
  //Serial.println("LOOP");
  //Serial.println();
}

// Calibration function
void calibrateIR() {
  
  //calibration buz

  analogWrite(buzz_pin, 255);

  long sum = 0;
  int minVal = 4095;
  int maxVal = 0;

  for (int i = 0; i < 1000; i++) {
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
  
  if(noise > 100){
    calibrateIR();
  }

  analogWrite(buzz_pin, 0);
  delay(350);

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
    logEvent("DETECTED");
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

  if (logFile) {
    logFile.print(time);
    logFile.print(",");
    logFile.println(value);
    logFile.flush();
  }
}

// Logging events
void logEvent(const char* event) {
  if (logFile) {
    logFile.print(millis());
    logFile.print(",EVENT:");
    logFile.println(event);
    logFile.flush();
  }
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
