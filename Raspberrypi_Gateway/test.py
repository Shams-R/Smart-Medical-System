
import json
import Bot
import medical_database as db



# Test Case 1: Sensor disconnected / booting up
data_waiting = {
    "heart_rate": 0,
    "spo2": 0,
    "temperature": 0,
    "serious_accident": 0,
    "location":"https://www.google.com/maps?q=29.987212,31.439972"
}

# Test Case 2: Perfectly healthy patient
data_stable = {
    "heart_rate": 75,
    "spo2": 98,
    "temperature": 37.0,
    "serious_accident": 0,
    "location":"https://www.google.com/maps?q=29.987212,31.439972"
}

# Test Case 3: Warning - High Temp (Fever)
data_warning = {
    "heart_rate": 85,
    "spo2": 96,
    "temperature": 38.2, 
    "serious_accident": 0,
    "location":"https://www.google.com/maps?q=29.987212,31.439972"
}

# Test Case 4: Critical - Hypoxia & Tachycardia
data_critical_vitals = {
    "heart_rate": 130, 
    "spo2": 85,       
    "temperature": 37.1,
    "serious_accident": 0,
    "location":"https://www.google.com/maps?q=29.987212,31.439972"
}

# Test Case 5: Critical - Fall/Accident detected
data_accident = {
    "heart_rate": 90,
    "spo2": 97,
    "temperature": 36.8,
    "serious_accident": 1 ,
    "location":"https://www.google.com/maps?q=29.987212,31.439972"
}


# Add the patient_id (Logic based on topic)

data_critical_vitals["patient_id"] = "ESP32_1"
  # send data to Bot
Bot.monitor_system(data_critical_vitals)

# Remove the location link
data_critical_vitals.pop("location", None)

#send data to medical_datbase
db.insert_reading(data_critical_vitals)