#ifndef MPU_SENSOR_H
#define MPU_SENSOR_H

#include <Arduino.h>
#include <freertos/semphr.h>

void initMPU6050();
void readMPU6050(SemaphoreHandle_t mutex);
bool getFallStatus();

#endif