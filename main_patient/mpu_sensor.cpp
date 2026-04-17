#include "mpu_sensor.h"
#include <Wire.h>

#define MPU6050_ADDR 0x68
const float FALL_THRESHOLD = 28.0f;
const unsigned long FALL_HOLD_TIME = 6000;

static bool mpuValid = false;
static float magnitude = 0.0f;
static bool fallMessageActive = false;
static unsigned long fallTriggerTime = 0;

void initMPU6050() {
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(0x6B);
  Wire.write(0x00);
  mpuValid = (Wire.endTransmission() == 0);
}

void readMPU6050(SemaphoreHandle_t mutex) {
  if (!mpuValid) return;
  if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    Wire.beginTransmission(MPU6050_ADDR);
    Wire.write(0x3B); 
    if (Wire.endTransmission(false) == 0) {
      Wire.requestFrom(MPU6050_ADDR, 6, true);
      if (Wire.available() == 6) {
        int16_t ax = (Wire.read() << 8) | Wire.read();
        int16_t ay = (Wire.read() << 8) | Wire.read();
        int16_t az = (Wire.read() << 8) | Wire.read();
        magnitude = sqrt(pow(ax/16384.0f, 2) + pow(ay/16384.0f, 2) + pow(az/16384.0f, 2)) * 9.8f;
      }
    }
    xSemaphoreGive(mutex);
  }
  
  if (magnitude > FALL_THRESHOLD && !fallMessageActive) {
    fallMessageActive = true;
    fallTriggerTime = millis();
  }
  if (fallMessageActive && (millis() - fallTriggerTime >= FALL_HOLD_TIME)) {
    fallMessageActive = false;
  }
}

bool getFallStatus() { return fallMessageActive; }