#include <TinyGPSPlus.h>
#include <SoftwareSerial.h>
#include <SD.h>
#include <SPI.h>

// Create an instance of TinyGPS++ to handle GPS data
TinyGPSPlus gps;

// Define RX and TX pins for the SoftwareSerial
SoftwareSerial gpsSerial(3, 4); // RX, TX
SoftwareSerial bridge(6, 7);
// Define chip select pin for the SD card module
const int chipSelect = 10;

void setup() {
  Serial.begin(9600);         // Start Serial Monitor
  gpsSerial.begin(9600);      // Start GPS communication
  Serial.println("GPS and SD Logging Test");

  // Initialize SD card
  if (!SD.begin(chipSelect)) {
    Serial.println("SD card initialization failed!");
    while (true);  // Halt if SD card is not initialized
  }
  Serial.println("SD card initialized.");

  // Create or open a file on the SD card
  File dataFile = SD.open("gpslog.txt", FILE_WRITE);
  if (dataFile) {
    dataFile.println("Date, Time, Latitude, Longitude, Altitude(m), Satellites");
    dataFile.close();
  } else {
    Serial.println("Error opening gpslog.txt");
  }
}

void loop() {
  // Continuously read GPS data if available
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());

    // Check if we have a valid GPS location
    if (gps.location.isUpdated()) {
      // Skip writing data if the date is invalid (0/0/2000)
      if (gps.date.month() == 0 || gps.date.day() == 0 || gps.date.year() == 2000) {
        return; // Skip this iteration if the date is not valid
      }

      // Prepare GPS data
      String dataString = "";
      dataString += gps.date.day();
      dataString += "/";
      dataString += gps.date.month();
      dataString += "/";
      dataString += gps.date.year();
      dataString += ", ";
      
      dataString += gps.time.hour() - 7;
      dataString += ":";
      dataString += gps.time.minute();
      dataString += ":";
      dataString += gps.time.second();
      dataString += ", ";
      
      dataString += String(gps.location.lat(), 6); // Latitude
      dataString += ", ";
      dataString += String(gps.location.lng(), 6); // Longitude
      dataString += ", ";
      dataString += String(gps.altitude.meters()); // Altitude
      dataString += ", ";
      dataString += String(gps.satellites.value()); // Satellite count

      // Print data to Serial Monitor
      Serial.println(dataString);

      Serial.write("Test string");

      // Save data to SD card
      File dataFile = SD.open("gpslog.txt", FILE_WRITE);
      if (dataFile) {
        dataFile.println(dataString);
        dataFile.close();
        Serial.println("Data written to SD card.");
      } else {
        Serial.println("Error opening gpslog.txt");
      }

      // Short delay before the next reading
      delay(1000);
    }
  }
}
