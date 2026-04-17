import time
import random
import medical_database as db

# The simulated patient nodes we want to transmit data for
SIMULATED_PATIENTS = ["ESP32_1", "ESP32_2", "ESP32_3"]

def generate_normal_vitals():
    """Generates a dictionary of healthy, normal vital signs."""
    return {
        "heart_rate": random.randint(65, 85),        # Normal resting HR
        "spo2": random.randint(95, 100),             # Normal oxygen
        "temperature": round(random.uniform(36.5, 37.2), 1), # Normal temp in C
        "serious_accident": 0                        # No accident
    }

def generate_critical_vitals():
    """Generates a dictionary of critical vital signs to test the emergency loop."""
    # Randomly choose what kind of emergency is happening
    emergency_type = random.choice(["hypoxia", "tachycardia", "fever", "accident"])
    
    vitals = generate_normal_vitals() # Start with normal, then override
    
    if emergency_type == "hypoxia":
        vitals["spo2"] = random.randint(80, 88) # Dangerously low oxygen
    elif emergency_type == "tachycardia":
        vitals["heart_rate"] = random.randint(125, 150) # Dangerously high HR
    elif emergency_type == "fever":
        vitals["temperature"] = round(random.uniform(38.5, 40.0), 1) # High fever
    elif emergency_type == "accident":
        vitals["serious_accident"] = 1 # Fall or impact detected
        
    return vitals

print("💉 Starting ESP32 Simulation...")
print("Press Ctrl+C to stop transmitting.")

try:
    while True:
        # Loop through each simulated patient and send a data packet
        for patient_id in SIMULATED_PATIENTS:
            
            # 90% of the time, the patient is fine. 10% of the time, trigger an emergency.
            if random.random() < 0.90:
                data = generate_normal_vitals()
            else:
                data = generate_critical_vitals()
                print(f"⚠️ Injecting emergency data for {patient_id}!")
            
            # Add the patient ID to the data packet
            data["patient_id"] = patient_id
            
            # Insert the simulated reading into the database
            db.insert_reading(data)
            
            print(f"Transmitted for {patient_id}: HR={data['heart_rate']}, SpO2={data['spo2']}%, Temp={data['temperature']}°C, Accident={data['serious_accident']}")
            
        # Wait 1.5 seconds before the next transmission cycle (adjust as needed)
        time.sleep(1.5)

except KeyboardInterrupt:
    print("\n🛑 Simulation stopped by user.")