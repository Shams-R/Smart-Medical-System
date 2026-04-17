#ifndef VITALS_SENSOR_H
#define VITALS_SENSOR_H
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <stdint.h>

void initMAX30102();
void tickMAX30102(SemaphoreHandle_t mutex);   // call every ~10ms to fill sample buffer
void computeMAX30102();                        // call once per second to run algorithm
int getSmoothedHR();
int getSmoothedSPO2();
bool isMaxValid();
uint32_t getIRValue();

#endif
