#ifndef TEMP_SENSOR_H
#define TEMP_SENSOR_H

void initDS18B20();
void requestDS18B20();  // Phase 1: start conversion (non-blocking)
void fetchDS18B20();    // Phase 2: read result after 800ms
float getTemperature();
bool isTempValid();

#endif
