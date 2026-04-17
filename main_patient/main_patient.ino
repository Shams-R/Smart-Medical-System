#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "mpu_sensor.h"
#include "temp_sensor.h"
#include "vitals_sensor.h"
#include "gy_gps.h"

// WiFi & MQTT
const char* ssid = "Toto3010";
const char* password = "L@rt3010";
const char* mqtt_server = "broker.hivemq.com";
const char* topic = "esp32_1/real_patient";
const char* alert_topic = "ESP32_1/alert"; // The topic to listen for alerts

// --- Hardware Setup ---
const int ALERT_PIN_1 = 32;
const int ALERT_PIN_2 = 33; 

WiFiClient espClient;
PubSubClient client(espClient);
SemaphoreHandle_t i2cMutex;

// GPS Mutex and Global Variable
SemaphoreHandle_t gpsMutex; 
String currentLocation = "GPS searching for satellites";

// FUNCTION PROTOTYPES
void fallDetectionTask(void * parameter);
void sensorTickTask(void * parameter);
void gpsTask(void * parameter); // [NEW] Prototype for the GPS task

const unsigned long displayInterval = 1000;

// readings
struct VitalsRecord {
  int heartRate;           
  int spo2;                
  float temperature;       
  bool accident;           
};

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  Wire.setClock(400000); // 400 kHz I2C
  
  i2cMutex = xSemaphoreCreateMutex();
  gpsMutex = xSemaphoreCreateMutex(); // [NEW] Initialize GPS Mutex

  initMPU6050();
  initDS18B20();
  initMAX30102();

  // Initialize GPS on RX:16, TX:17
  gps_setup(16, 17);

  setup_wifi();
  client.setServer(mqtt_server, 1883);

  // Core 0 Tasks (I2C)
  xTaskCreatePinnedToCore(fallDetectionTask, "FallTask",   10000, NULL, 3, NULL, 0);
  xTaskCreatePinnedToCore(sensorTickTask,    "SensorTick", 10000, NULL, 2, NULL, 0);
  
  // Core 1 Task (GPS - kept off Core 0 so it doesn't interrupt I2C sensors)
  xTaskCreatePinnedToCore(gpsTask, "GPSTask", 4096, NULL, 1, NULL, 1);

  pinMode(ALERT_PIN_1, OUTPUT); 
  pinMode(ALERT_PIN_2, OUTPUT);
}

//  Callback function that runs automatically when raspberrypi send an alert
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Alert received on topic: ");
  Serial.println(topic);

  StaticJsonDocument<200> doc;
  deserializeJson(doc, payload, length);

  const char* status = doc["status"]; 
  
  if (String(status) == "CRITICAL") {
    Serial.println("!!! CRITICAL ALERT FROM PI !!!");
    // Blink LED to notify
    for(int i=0; i<1; i++) {
      digitalWrite(ALERT_PIN_1, HIGH);
      delay(100);
      digitalWrite(ALERT_PIN_1, LOW);
      delay(100);
      digitalWrite(ALERT_PIN_2, HIGH);
      delay(100);
      digitalWrite(ALERT_PIN_2, LOW);
      delay(100);
    }
  }
}

void setup_wifi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { 
    vTaskDelay(pdMS_TO_TICKS(500)); 
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");

  //  Tell the client which function to use for messages ---
  client.setCallback(callback);
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0, 0xffff), HEX);
    
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");

      // Subscribe to the alert topic after connecting ---
      client.subscribe(alert_topic);
      Serial.println("Subscribed to alert topic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 2 seconds");
      delay(2000);
    }
  }
}

// Dedicated task to constantly empty the UART buffer and check GPS
void gpsTask(void *pv) {
  for(;;) {
    String status = gps_check();
    
    // If the GPS gives us a new update (happens every 2 seconds)
    if (status != "") {
      xSemaphoreTake(gpsMutex, portMAX_DELAY); // Lock variable
      
      // If it's a valid Maps link, ALWAYS update.
      if (status.startsWith("http")) {
        currentLocation = status; 
      } 
      // If it's just an error/searching status, only update if we don't already have a valid location
      else if (!currentLocation.startsWith("http")) {
        currentLocation = status;
      }
      
      xSemaphoreGive(gpsMutex); // Unlock variable
    }
    
    // Check the serial buffer every 50ms so it never overflows
    vTaskDelay(pdMS_TO_TICKS(50)); 
  }
}

void fallDetectionTask(void *pv) {
  for(;;) { 
    readMPU6050(i2cMutex); 
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void sensorTickTask(void *pv) {
  requestDS18B20(); // kick off first conversion
  TickType_t conversionStart = xTaskGetTickCount();

  for(;;) {
    tickMAX30102(i2cMutex); // one sample per tick into circular buffer
    if ((xTaskGetTickCount() - conversionStart) >= pdMS_TO_TICKS(800)) {
      fetchDS18B20(); // read completed conversion
      requestDS18B20(); // start next one
      conversionStart = xTaskGetTickCount();
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

VitalsRecord getReadings() {
  VitalsRecord readings;
  Serial.println("\n--- Monitor Report ---"); // Added \n for extra clean spacing

  // Fall Status
  readings.accident = getFallStatus();
  if (readings.accident) Serial.println("[!] Accident Detected");
  else Serial.println("[+] Status: Stable");

  // Temperature
  readings.temperature = getTemperature();
  Serial.print("Temperature: ");
  Serial.print(readings.temperature);
  Serial.println(" °C");

  // Heart Rate and SpO2 with Finger Detection
  if (isMaxValid() && getIRValue() > 50000) {
    readings.heartRate = getSmoothedHR();
    readings.spo2 = getSmoothedSPO2();
    Serial.print("HR: "); Serial.println(readings.heartRate);
    Serial.print("SPO2: "); Serial.print(readings.spo2); Serial.println("%");
  } else {
    Serial.println(">>> Place your finger on HR Sensor...");
    readings.heartRate = 0;
    readings.spo2 = 0;
  }

  // grab the GPS location and print it in the report
  String displayLocation = "";
  xSemaphoreTake(gpsMutex, portMAX_DELAY); // Lock the variable so we don't read while it's updating
  displayLocation = currentLocation;
  xSemaphoreGive(gpsMutex);                // Unlock it
  
  Serial.print("Location: ");
  Serial.println(displayLocation);
  Serial.println("----------------------");

  return readings;
}

void loop() {
  computeMAX30102(); // run algorithm over buffered samples (no I2C)
  VitalsRecord currentData = getReadings();

  // Publish to MQTT
  if (!client.connected()) reconnect();
  client.loop();

  StaticJsonDocument<256> doc;
  doc["heart_rate"] = currentData.heartRate;
  doc["spo2"] = currentData.spo2;
  doc["temperature"] = currentData.temperature;
  doc["serious_accident"] = currentData.accident;

  // [NEW] Safely grab the latest location string
  xSemaphoreTake(gpsMutex, portMAX_DELAY);
  doc["location"] = currentLocation; 
  xSemaphoreGive(gpsMutex);

  char jsonBuffer[256];
  serializeJson(doc, jsonBuffer);
  client.publish(topic, jsonBuffer);

  vTaskDelay(pdMS_TO_TICKS(displayInterval));
}