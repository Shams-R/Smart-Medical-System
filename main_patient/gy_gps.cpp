#include "gy_gps.h"
#include <TinyGPS++.h>
#include <HardwareSerial.h>

// Create the GPS object and Serial port locally in this file
static TinyGPSPlus gps;
static HardwareSerial gpsSerial(2); // Use ESP32 Hardware UART 2

// Variables for timing
static unsigned long lastDataTime = 0;
static unsigned long lastPrintTime = 0;

void gps_setup(int rx_pin, int tx_pin) {
  // Start the serial connection to the GPS module
  gpsSerial.begin(9600, SERIAL_8N1, rx_pin, tx_pin);
  
  // It's okay to leave these here since they only print once at startup
  Serial.println("===============================");
  Serial.println("   Simple GPS Monitor Started  ");
  Serial.println("===============================");
}

String gps_check() {
  String mapsLink = "";
  
  // 1. Read everything the GPS is sending right now
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
    lastDataTime = millis(); // Save the exact moment we got data
  }

  // 2. Only check updates every 2 seconds
  if (millis() - lastPrintTime > 2000) {
    lastPrintTime = millis(); 

    // CHECK 1: Power & Wiring
    if (millis() - lastDataTime > 2000) {
      return "GPS has no data";
    }
    
    // CHECK 2: Satellites
    else if (!gps.location.isValid()) {
      return "GPS searching for satellites...";
    }
    
    // CHECK 3: Success
    else {
      mapsLink = "https://www.google.com/maps?q=" + String(gps.location.lat(), 6) + "," + String(gps.location.lng(), 6);
      return mapsLink;
    }
  }
  
  return ""; 
}