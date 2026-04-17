#include "temp_sensor.h"
#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 4
static OneWire oneWire(ONE_WIRE_BUS);
static DallasTemperature tempSensors(&oneWire);
static float temperatureC = 36.0f;
static bool tempValid = false;

void initDS18B20() {
  tempSensors.begin();
  if (tempSensors.getDeviceCount() > 0) {
    tempSensors.setResolution(12);
    tempSensors.setWaitForConversion(false);
    tempValid = true;
    tempSensors.requestTemperatures(); // start first conversion immediately
  }
}

void requestDS18B20() {
  if (!tempValid) return;
  tempSensors.requestTemperatures(); // returns instantly (WaitForConversion=false)
}

void fetchDS18B20() {
  if (!tempValid) return;
  float temp = tempSensors.getTempCByIndex(0);
  if (temp != DEVICE_DISCONNECTED_C && temp > -127) temperatureC = temp;
}

float getTemperature() {
  return temperatureC;
}

bool isTempValid() {
  return tempValid;
}
