#ifndef GY_GPS_H
#define GY_GPS_H

#include <Arduino.h>

// Initializes the GPS module on the specified ESP32 pins
void gps_setup(int rx_pin, int tx_pin);

// Reads GPS data and returns either a status message or a Google Maps link
String gps_check();

#endif // GY_GPS_H