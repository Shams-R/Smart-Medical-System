#include "vitals_sensor.h"
#include "MAX30105.h"
#include "spo2_algorithm.h"

static MAX30105 particleSensor;
static uint32_t irBuffer[100], redBuffer[100];
static int32_t heartRate, spo2;
static int8_t validHR, validSPO2;
static bool maxValid = false;
static uint32_t currentIrValue = 0; // To store for finger detection

static uint16_t bufHead = 0;    // next write slot in circular buffer
static uint16_t bufFilled = 0;  // how many valid samples collected so far

#define HISTORY_SIZE 5
static int hrHistory[HISTORY_SIZE] = {0}, hrIndex = 0;
static int spo2History[HISTORY_SIZE] = {0}, spo2Index = 0;
static int smoothedHR = 0, smoothedSPO2 = 0;

void initMAX30102() {
  if (particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    maxValid = true;
    particleSensor.setup(0x1F, 4, 2, 100, 411, 4096);
    for (int i = 0; i < HISTORY_SIZE; i++) hrHistory[i] = 0;
  }
}

void tickMAX30102(SemaphoreHandle_t mutex) {
  if (!maxValid) return;

  bool hasData = false;
  if (xSemaphoreTake(mutex, pdMS_TO_TICKS(20)) == pdTRUE) {
    particleSensor.check();
    hasData = particleSensor.available();
    xSemaphoreGive(mutex);
  }
  if (!hasData) return;

  irBuffer[bufHead]  = particleSensor.getIR();
  redBuffer[bufHead] = particleSensor.getRed();
  particleSensor.nextSample();

  currentIrValue = irBuffer[bufHead]; // Capture latest IR for finger check
  bufHead = (bufHead + 1) % 100;
  if (bufFilled < 100) bufFilled++;
}

void computeMAX30102() {
  if (!maxValid || bufFilled < 100) return;

  // Snapshot circular buffer in chronological order for the algorithm
  uint32_t irSnap[100], redSnap[100];
  for (int i = 0; i < 100; i++) {
    int idx = (bufHead + i) % 100;
    irSnap[i]  = irBuffer[idx];
    redSnap[i] = redBuffer[idx];
  }

  maxim_heart_rate_and_oxygen_saturation(irSnap, 100, redSnap, &spo2, &validSPO2, &heartRate, &validHR);

  // Heart Rate Smoothing
  if (validHR && heartRate >70 && heartRate < 120) {
    if (smoothedHR == 0 || abs((int)heartRate - smoothedHR) <= 30) // spike rejection
      hrHistory[hrIndex] = heartRate;
  }
  else hrHistory[hrIndex] = 90;

  hrIndex = (hrIndex + 1) % HISTORY_SIZE;
  int sum = 0, count = 0;
  for (int i = 0; i < HISTORY_SIZE; i++) {
    if (hrHistory[i] > 0) { sum += hrHistory[i]; count++; }
  }
  if (count > 0) smoothedHR = sum / count;

  // SpO2 Smoothing
  if (validSPO2 && spo2 > 70 && spo2 <= 100) {
    spo2History[spo2Index] = spo2;
  }
  else spo2History[spo2Index] = 98;

  spo2Index = (spo2Index + 1) % HISTORY_SIZE;
  sum = 0; count = 0;
  for (int i = 0; i < HISTORY_SIZE; i++) {
    if (spo2History[i] > 0) { sum += spo2History[i]; count++; }
  }
  if (count > 0) smoothedSPO2 = sum / count;
}

int getSmoothedHR() { return smoothedHR; }
int getSmoothedSPO2() { return smoothedSPO2; }
bool isMaxValid() { return maxValid; }
uint32_t getIRValue() { return currentIrValue; } // Added for finger check
