#include <Adafruit_FXOS8700.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <WiFi.h>
#include <time.h>

// --- SD card SPI pins ---
#define SD_MOSI 23
#define SD_MISO 19
#define SD_SCLK 18
#define SD_CS   5

#define SWITCH_PIN 34

int filenumber = 0;
File myFile;
bool isWriting = false;

// --- WiFi credentials ---
const char* ssid     = "DaddysIphone";
const char* password = "babygirlgojo";

// --- NTP config ---
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -21600;   // Calgary UTC-6
const int   daylightOffset_sec = 3600; // DST

// --- FXOS8700 sensor ---
Adafruit_FXOS8700 accelmag = Adafruit_FXOS8700(0x8700A, 0x8700B);

// --- Sampling rate ---
const int samplingIntervalMs = 50; // 20 Hz
unsigned long lastSampleTime = 0;

// --- Time tracking ---
unsigned long startMillis = 0;
unsigned long lastMillisSync = 0;
time_t baseEpoch = 0;

// --- Get human-readable, strictly increasing timestamp ---
String getHighPrecisionTimestamp() {
  unsigned long elapsedMs = millis() - startMillis;
  time_t currentEpoch = baseEpoch + (elapsedMs / 1000);
  unsigned long ms = elapsedMs % 1000;

  struct tm timeinfo;
  localtime_r(&currentEpoch, &timeinfo);

  char buffer[20];
  sprintf(buffer, "%02d:%02d:%02d.%03lu",
          timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, ms);
  return String(buffer);
}

void displaySensorDetails() {
  sensor_t accel, mag;
  accelmag.getSensor(&accel, &mag);
  Serial.println("------------------------------------");
  Serial.println("ACCELEROMETER");
  Serial.println("------------------------------------");
  Serial.println("Sensor:       " + String(accel.name));
  Serial.println("Driver Ver:   " + String(accel.version));
  Serial.print("Unique ID:    0x"); Serial.println(accel.sensor_id, HEX);
  Serial.print("Max Value:    "); Serial.print(accel.max_value, 4); Serial.println(" m/s^2");
  Serial.print("Min Value:    "); Serial.print(accel.min_value, 4); Serial.println(" m/s^2");
  Serial.print("Resolution:   "); Serial.print(accel.resolution, 8); Serial.println(" m/s^2");
  Serial.println("");
}

void setup() {
  Serial.begin(115200);
  pinMode(SWITCH_PIN, INPUT);

  // --- Connect WiFi & NTP ---
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 15000) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected.");
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    struct tm timeinfo;
    while (!getLocalTime(&timeinfo)) {
      delay(100);
      Serial.print(".");
    }
    Serial.println("NTP time synced.");

    // record base epoch for monotonic timestamps
    time(&baseEpoch);
    startMillis = millis();
    lastMillisSync = millis();
  } else {
    Serial.println("Failed to sync time — continuing without NTP.");
    time(&baseEpoch);
    startMillis = millis();
  }

  // --- Initialize SD card ---
  SPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS, SPI)) {
    Serial.println("SD Card mount failed!");
    while (1);
  }
  Serial.println("SD Card initialized.");

  // --- Initialize accelerometer ---
  if (!accelmag.begin()) {
    Serial.println("Ooops, no FXOS8700 detected ... Check wiring!");
    while (1);
  }
  accelmag.setAccelRange(ACCEL_RANGE_4G);
  accelmag.setSensorMode(ACCEL_ONLY_MODE);
  displaySensorDetails();
}

void loop() {
  int switchstate = digitalRead(SWITCH_PIN);
  sensors_event_t aevent;
  accelmag.getEvent(&aevent);

  unsigned long now = millis();

  if (switchstate) {
    if (!isWriting) {
      String fileName = "/data" + String(filenumber) + ".txt";
      myFile = SD.open(fileName, FILE_WRITE);
      if (myFile) {
        Serial.println("Started writing to " + fileName);
        isWriting = true;
        delay(500);
      } else {
        Serial.println("Error opening " + fileName);
      }
    }

    // --- Write data at set sampling rate ---
    if (isWriting && now - lastSampleTime >= samplingIntervalMs) {
      lastSampleTime = now;

      String dataString = getHighPrecisionTimestamp() + ",";
      dataString += String(aevent.acceleration.x, 4) + ",";
      dataString += String(aevent.acceleration.y, 4) + ",";
      dataString += String(aevent.acceleration.z, 4);

      Serial.println(dataString);
      if (myFile) myFile.println(dataString);
    }

  } else if (isWriting) {
    if (myFile) myFile.close();
    Serial.println("File saved and closed.");
    filenumber++;
    isWriting = false;
    delay(500);
  } else {
    if (now - lastSampleTime >= samplingIntervalMs) {
      lastSampleTime = now;
      String dataString = getHighPrecisionTimestamp() + ",";
      dataString += String(aevent.acceleration.x, 4) + ",";
      dataString += String(aevent.acceleration.y, 4) + ",";
      dataString += String(aevent.acceleration.z, 4);
      Serial.println(dataString);
    }
  }
}

