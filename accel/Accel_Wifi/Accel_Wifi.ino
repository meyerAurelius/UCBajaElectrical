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

const char* WIFI_SSID = "BAJA AP";
const char* WIFI_PASS = "password123";

const char* SERVER_IP = "192.168.0.104"; // <-- replace with your laptop's LAN IP
const uint16_t SERVER_PORT = 5055;

WiFiClient client;

uint32_t intervalMs = 200;  // default 1s
uint32_t lastSend = 0;
uint32_t counter = 0;

int filenumber = 0;
File myFile;
bool isWriting = false;

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

void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  Serial.printf("[wifi] Connecting to \"%s\"...\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(250);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("[wifi] Connected. IP: ");
    Serial.println(WiFi.localIP());
    // Optional: sometimes helps in noisy environments
    // WiFi.setSleep(false);
  } else {
    Serial.println("[wifi] Failed to connect.");
  }
}

bool connectServer() {
  if (client.connected()) return true;

  Serial.printf("[tcp] Connecting to %s:%u ...\n", SERVER_IP, SERVER_PORT);
  if (client.connect(SERVER_IP, SERVER_PORT)) {
    client.setTimeout(1000);
    Serial.println("[tcp] Connected to server.");
    // Say hello
    client.print("HELLO ESP32\n");
    return true;
  }
  Serial.println("[tcp] Connect failed.");
  return false;
}

void sendSample(double accel_x, double accel_y, double accel_z){
  uint32_t now = millis();

  client.printf("{\"uptime\":%d,\"accel_x\":%f,\"accel_y\":%f,\"accel_z\":%f}\n", now, accel_x, accel_y, accel_z);
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
  delay(200);
  pinMode(SWITCH_PIN, INPUT);

  connectWiFi();
  connectServer();


  if (WiFi.status() == WL_CONNECTED) {
    
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    struct tm timeinfo;

    for(int i = 0; i < 5; i++){
      if(getLocalTime(&timeinfo)){
        Serial.println("NTP time synced.");

        time(&baseEpoch);
        startMillis = millis();
        lastMillisSync = millis();
        break;
      }
      else{
        Serial.println("NTP sync FAILED!");
      }

      delay(200);
    }
  } else {
    Serial.println("Failed to connect to wifi — continuing without NTP.");
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

  

  if (now - lastSend >= intervalMs) {
    lastSend = now;
    if (WiFi.status() == WL_CONNECTED){
    if (client.connected()) {
      
        sendSample(aevent.acceleration.x, aevent.acceleration.y, aevent.acceleration.z);
      
    }
    }
  }
}
