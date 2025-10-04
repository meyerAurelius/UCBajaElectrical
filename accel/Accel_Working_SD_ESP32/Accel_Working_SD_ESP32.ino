#include <Adafruit_FXOS8700.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"

// Define SD card SPI pins for ESP32
#define SD_MOSI     23  // safe HSPI MOSI
#define SD_MISO     19  // safe HSPI MISO
#define SD_SCLK     18  // safe HSPI SCLK
#define SD_CS       5   // safe CS pin

#define SWITCH_PIN 34   // any safe input pin

int filenumber = 0;
File myFile;
bool isWriting = false;

/* Assign a unique ID to this sensor at the same time */
Adafruit_FXOS8700 accelmag = Adafruit_FXOS8700(0x8700A, 0x8700B);

void displaySensorDetails(void) {
  sensor_t accel, mag;
  accelmag.getSensor(&accel, &mag);
  Serial.println("------------------------------------");
  Serial.println("ACCELEROMETER");
  Serial.println("------------------------------------");
  Serial.println("Sensor:       " + String(accel.name));
  Serial.println("Driver Ver:   " + String(accel.version));
  Serial.print("Unique ID:    0x");
  Serial.println(accel.sensor_id, HEX);

  Serial.println("Min Delay:    " + String(accel.min_delay) + " s");

  Serial.print("Max Value:    ");
  Serial.print(accel.max_value, 4);
  Serial.println(" m/s^2");

  Serial.print("Min Value:    ");
  Serial.print(accel.min_value, 4);
  Serial.println(" m/s^2");

  Serial.print("Resolution:   ");
  Serial.print(accel.resolution, 8);
  Serial.println(" m/s^2");

  delay(500);
  Serial.println("");
}

void setup(void) {
  Serial.begin(115200);

  pinMode(SWITCH_PIN, INPUT);

  while (!Serial) { delay(1); }

  Serial.println("Setup start");

  // Initialize SPI with custom pins for ESP32
  SPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);

  if (!SD.begin(SD_CS, SPI)) {
    Serial.println("SD Card MOUNT FAIL");
  } else {
    Serial.println("SD Card MOUNT SUCCESS");
    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.println("SDCard Size: " + String(cardSize) + "MB");

    Serial.println("FXOS8700 Test");

    if (!accelmag.begin()) {
      Serial.println("Ooops, no FXOS8700 detected ... Check your wiring!");
      while (1);
    }

    accelmag.setAccelRange(ACCEL_RANGE_4G); // optional
    accelmag.setSensorMode(ACCEL_ONLY_MODE);
    displaySensorDetails();
  }
}

void loop(void) {
  int switchstate = digitalRead(SWITCH_PIN);

  if (switchstate) {
    if (!isWriting) {
      String fileName = "/data" + String(filenumber) + ".csv";
      myFile = SD.open(fileName, FILE_WRITE);
      if (myFile) {
        Serial.println("Started writing to " + fileName);
        isWriting = true;
        delay(1000);
      } else {
        Serial.println("Error opening " + fileName);
      }
    }
  }

  sensors_event_t aevent;

  if (isWriting) {
    accelmag.getEvent(&aevent);

    Serial.print(aevent.acceleration.x, 4);
    Serial.print(",");
    myFile.print(aevent.acceleration.x, 4);
    myFile.print(",");

    Serial.print(aevent.acceleration.y, 4);
    Serial.print(",");
    myFile.print(aevent.acceleration.y, 4);
    myFile.print(",");

    Serial.print(aevent.acceleration.z, 4);
    Serial.print("  ");
    myFile.print(aevent.acceleration.z, 4);
    myFile.print("  ");

    Serial.println("");
    myFile.println("");

    delay(50);
    isWriting = true;
  } 
  if (!switchstate & isWriting){
      myFile.close();
      Serial.println("File saved and closed.");
      filenumber++;
      isWriting = false;
    
  }
}
