import paho.mqtt.publish as publish
import json

#  using same Broker
ALERT_BROKER = "broker.hivemq.com"
ALERT_PORT = 1883

def send_alert(patient_id):
    """
    Connects to the broker, publishes a critical alert, and disconnects.
    """
    # Create a specific topic for this patient's ESP32 to listen to,example "ESP32_1/alert"
    topic = f"{patient_id}/alert"
    
    # The message payload to send
    payload_dict = {
        "status": "CRITICAL",
        "instruction": "Trigger Alarm"
    }
    payload_json = json.dumps(payload_dict)

    try:
        # publish.single handles connecting, sending, and disconnecting automatically
        publish.single(topic, payload=payload_json, hostname=ALERT_BROKER, port=ALERT_PORT)
        print(f"[ALERT SYSTEM] Successfully published critical alert to {topic}")
    except Exception as e:
        print(f"[ALERT SYSTEM] Failed to send alert to {topic}. Error: {e}")