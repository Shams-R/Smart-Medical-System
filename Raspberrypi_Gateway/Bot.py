import requests
import time

# --- CREDENTIALS ---
TOKEN = "8621635127:AAHkwm4Jz5i8vObZLUHhE-IjRzzwGuBGAhs"
DOCTOR_ID = "8284005169"
HOSPITAL_ID = "955741171"

# --- PATIENT ID MAPPING ---
PATIENT_TELEGRAM_MAP = {
    "ESP32_1": "955741171",
    "ESP32_2": "955741171", 
    "ESP32_3": "955741171"
}

# --- COOLDOWN TRACKER ---
# Dictionary to store the last time an alert was sent for each patient
last_alert_time = {}
# How many seconds to wait before sending another Telegram message for the SAME patient
ALERT_COOLDOWN = 60 

def send_msg(text, target_id):
    if not target_id:
        return 
        
    url = f"https://api.telegram.org/bot{TOKEN}/sendMessage"
    payload = {"chat_id": target_id, "text": text, "parse_mode": "HTML"}
    
    try:
        response = requests.post(url, json=payload, timeout=5) # Added a 5s timeout to prevent infinite hanging
        if response.status_code != 200:
            print(f"❌ ERROR sending to {target_id}: {response.text}")
    except Exception as e:
         print(f"❌ Network ERROR sending to {target_id}: {e}")

def monitor_system(data):
    p_id = data.get("patient_id")
    hr = data.get("heart_rate")
    spo2 = data.get("spo2")
    temp = data.get("temperature")
    accident = data.get("serious_accident")
    location = data.get("location")
    
    patient_tg_id = PATIENT_TELEGRAM_MAP.get(p_id)
    current_time = time.time()

    print(f"\n--- MONITORING [{p_id}]: HR={hr}, SpO2={spo2}%, Temp={temp}°C, Accident={accident} ---")

    if hr == 0 and spo2 == 0 and accident == 0:
        if temp == 0 or temp >= 30 and temp <= 37:
            print("STATUS: WAITING FOR SENSOR")
            return {"level": "waiting", "label": "WAITING FOR SENSOR", "critical": False}
        else :
            print("STATUS: CRITICAL")
            return {"level": "critical", "label": "CRITICAL", "critical": False}           


    # CRITICAL STATE
    #elif accident == 1 or (0 < spo2 < 90) or hr >= 120 or (0 < hr < 50) or temp >= 39 or (0 < temp < 35):
    elif accident == 1 or (0 < spo2 < 90) or hr >= 120 or (0 < hr < 50) or temp >= 39 or (0 < temp < 30):
        label = "ACCIDENT" if accident == 1 else "VITALS CRITICAL"
        
        # Check if the patient is already in the dictionary AND if the cooldown has passed
        if p_id not in last_alert_time or (current_time - last_alert_time[p_id]) > ALERT_COOLDOWN:
            print(f"STATUS: CRITICAL ({label}) - FIRING ALERTS!")
            
            # Send Alerts
            send_msg(f"Patient {p_id} is CRITICAL! ({label})", DOCTOR_ID)
            send_msg(f"Dispatch Emergency Team to Patient {p_id}!\nLocation: {location}", HOSPITAL_ID)
            send_msg("Critical alert triggered. Help is on the way. Please stay still.", patient_tg_id)
            
            # Update the timestamp for this patient
            last_alert_time[p_id] = current_time
            
        else:
            time_left = int(ALERT_COOLDOWN - (current_time - last_alert_time[p_id]))
            print(f"STATUS: CRITICAL ({label}) - Alerts muted for {time_left}s (Cooldown active)")

        print("FEEDBACK COMMAND: [BUZZER_ON]")
        return {"level": "critical", "label": label, "critical": True}

    # WARNING STATE
    #elif (spo2 < 94 or spo2 > 100) or (hr < 50 or hr > 105) or (temp < 36 or temp >= 37.8):
    elif (spo2 < 94 or spo2 > 100) or (hr < 50 or hr > 105) or (temp < 30 or temp > 37):
  
        print("STATUS: NEEDS ATTENTION")
        
        # We can also apply a separate cooldown here if desired, but for now just sending:
        if p_id not in last_alert_time or (current_time - last_alert_time[p_id]) > ALERT_COOLDOWN:
             send_msg(f"Patient {p_id} needs attention. Vitals are abnormal.", DOCTOR_ID)
             last_alert_time[p_id] = current_time
        
        print("FEEDBACK COMMAND: [YELLOW_LED_ON]")
        return {"level": "warning", "label": "NEEDS ATTENTION", "critical": False}

    # STABLE STATE
    else:
        print("STATUS: STABLE")
        return {"level": "stable", "label": "STABLE", "critical": False}

def isCritical(data):
    hr = data.get("heart_rate")
    spo2 = data.get("spo2")
    temp = data.get("temperature")
    accident = data.get("serious_accident")

    if accident == 1 or (0 < spo2 < 90) or hr >= 120 or (0 < hr < 50) or temp >= 39 or (0 < temp < 30) or (hr == 0 and spo2 == 0 and accident== 0  and (temp < 30 or temp > 37)) :
        return True
    else:
        return False