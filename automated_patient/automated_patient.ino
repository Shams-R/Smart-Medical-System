#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// --- WiFi Credentials ---
const char* ssid = "Toto3010";
const char* password = "L@rt3010";

// --- MQTT Setup ---
const char* mqtt_server = "broker.hivemq.com"; 
const char* topic = "esp32_2/virtual_patient_1"; // Change for 2nd ESP32
const char* alert_topic = "ESP32_2/alert"; // The topic to listen for alerts

// --- Hardware Setup ---
const int ALERT_PIN_1 = 4; // For external LED  (GPIO 18)
const int ALERT_PIN_2 = 2; // Buzzer pin  (GPIO 19)

const int stable=32;
const int warning=33;
const int critical=25;

WiFiClient espClient;
PubSubClient client(espClient);

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
    for(int i=0; i<5; i++) {
      Serial.print("iteration: ");
      Serial.println(i);
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
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
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
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  // Initialize alert pins
  pinMode(ALERT_PIN_1, OUTPUT); 
  pinMode(ALERT_PIN_2, OUTPUT);

  pinMode(stable, INPUT); 
  pinMode(critical, INPUT);
  pinMode(warning, INPUT); 

  setup_wifi();
  client.setServer(mqtt_server, 1883);

  //  Tell the client which function to use for messages ---
  client.setCallback(callback);
}


void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // --- Create JSON Payload ---
  StaticJsonDocument<256> doc;

  // Replace these hardcoded values with your actual sensor readings
  if (digitalRead(stable)) {
    doc["heart_rate"] = 95;
    doc["spo2"] = 98;
    doc["temperature"] = 36.5;
    doc["serious_accident"] = 0; 
    doc["location"] = "https://maps.google.com/?q=30.0444,31.2357";
  }
  else if(digitalRead(warning)) {
    doc["heart_rate"] = 92;
    doc["spo2"] = 110;
    doc["temperature"] = 38.2;
    doc["serious_accident"] = 0; 
    doc["location"] = "https://maps.google.com/?q=30.0444,31.2357";
  }
  else if (digitalRead(critical)){
    doc["heart_rate"] = 75;
    doc["spo2"] = 98;
    doc["temperature"] = 36.5;
    doc["serious_accident"] = 1; 
    doc["location"] = "https://maps.google.com/?q=30.0444,31.2357";
  }


  char jsonBuffer[256];
  serializeJson(doc, jsonBuffer);

  // --- Publish to Broker ---
  Serial.print("Publishing data to topic: ");
  Serial.println(topic);
  Serial.println(jsonBuffer);
  
  client.publish(topic, jsonBuffer);
  
  // Wait 5 seconds before sending the next reading
  delay(5000); 
}

