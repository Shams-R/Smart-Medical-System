import paho.mqtt.client as mqtt
import json
import queue
import threading
import alert_esp32
import medical_database as db
import Bot

# --- MQTT Broker Settings ---
MQTT_BROKER = "broker.hivemq.com"
MQTT_PORT = 1883

# Define the topics
TOPIC_ESP1 = "esp32_1/real_patient"
TOPIC_ESP2 = "esp32_2/virtual_patient_1"
TOPIC_ESP3 = "esp32_2/virtual_patient_2"
TOPIC_ESP4 = "esp32_2/virtual_patient_3"

# Initialize the database
db.init_db()

# Create a queue to hold incoming messages
message_queue = queue.Queue()

# --- Background Worker Thread ---
def process_data_worker():
    """This runs in the background and processes messages one by one."""
    while True:
        # This will block until a message is added to the queue
        topic, payload = message_queue.get()
        
        try:
            data = json.loads(payload)

            # Add the patient_id (Logic based on topic)
            if topic == TOPIC_ESP1:
                data["patient_id"] = "ESP32_1"
            elif topic == TOPIC_ESP2:
                data["patient_id"] = "ESP32_2"
            elif topic == TOPIC_ESP3:
                data["patient_id"] = "ESP32_3"
            else:
                data["patient_id"] = "ESP32_4"
            
            # Send data to Bot
            Bot.monitor_system(data)

            # If critical, send alert to ESP32
            if Bot.isCritical(data):
               alert_esp32.send_alert(data["patient_id"])    

            # Remove the location link
            data.pop("location", None)

            # Send data to medical_database
            db.insert_reading(data)

        except json.JSONDecodeError:
            print(f"Failed to decode JSON from {topic}: {payload}")
        except Exception as e:
            print(f"Error processing message: {e}")
        finally:
            # Tell the queue that the processing for this task is done
            message_queue.task_done()

# Callback when the client connects to the broker
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected successfully to broker!")
        topics = [  (TOPIC_ESP1, 0), (TOPIC_ESP2, 0), (TOPIC_ESP3, 0), (TOPIC_ESP4, 0)]

        client.subscribe(topics)

        print("Listening to:")
        for t, _ in topics:
            print(f"- {t}")
    else:
        print(f"Connection failed with code {rc}")

# Callback when a message is received from the broker
def receive_send_data(client, userdata, msg):
    # LIGHTNING FAST: Just decode the payload string and drop it in the queue
    try:
        payload_str = msg.payload.decode('utf-8')
        message_queue.put((msg.topic, payload_str))
    except Exception as e:
        print(f"Error putting message in queue: {e}")

# --- Setup Client and Threads ---

# 1. Start the background worker thread
worker_thread = threading.Thread(target=process_data_worker, daemon=True)
worker_thread.start()

# 2. Setup MQTT Client
client = mqtt.Client()
client.on_connect = on_connect
client.on_message = receive_send_data

print(f"Connecting to {MQTT_BROKER}...")
client.connect(MQTT_BROKER, MQTT_PORT, 60)

# 3. Start listening
client.loop_forever()