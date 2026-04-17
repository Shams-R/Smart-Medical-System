import sqlite3
from datetime import datetime
import csv
import os

DB_NAME = "medical.db"

def init_db():
    conn = sqlite3.connect(DB_NAME)
    cursor = conn.cursor()
    cursor.execute("""
    CREATE TABLE IF NOT EXISTS vitals (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        patient_id TEXT,
        heart_rate REAL,
        spo2 REAL,
        temperature REAL,
        serious_accident INTEGER,
        timestamp TEXT
    )
    """)
    conn.commit()
    conn.close()

def insert_reading(data):
    conn = sqlite3.connect(DB_NAME)
    cursor = conn.cursor()
    cursor.execute("""
    INSERT INTO vitals (patient_id, heart_rate, spo2, temperature, serious_accident, timestamp)
    VALUES (?, ?, ?, ?, ?, ?)
    """, (
        data["patient_id"], data["heart_rate"], data["spo2"], 
        data["temperature"], data["serious_accident"], 
        datetime.now().strftime("%H:%M:%S") # Storing time for cleaner graphs
    ))
    conn.commit()
    conn.close()

def get_latest(patient_id):
    conn = sqlite3.connect(DB_NAME)
    cursor = conn.cursor()
    cursor.execute("""
    SELECT heart_rate, spo2, temperature, serious_accident, timestamp
    FROM vitals WHERE patient_id = ? ORDER BY id DESC LIMIT 1
    """, (patient_id,))
    row = cursor.fetchone()
    conn.close()
    return row

def get_history(patient_id, limit=50):
    conn = sqlite3.connect(DB_NAME)
    cursor = conn.cursor()
    cursor.execute("""
    SELECT heart_rate, spo2, temperature, timestamp
    FROM vitals WHERE patient_id = ? ORDER BY id ASC LIMIT ?
    """, (patient_id, limit))
    rows = cursor.fetchall()
    conn.close()
    return rows

def check_patient_exists(patient_id):
    conn = sqlite3.connect(DB_NAME)
    cursor = conn.cursor()
    cursor.execute("SELECT 1 FROM vitals WHERE patient_id = ? LIMIT 1", (patient_id,))
    exists = cursor.fetchone() is not None
    conn.close()
    return exists

def delete_patient(patient_id):
    conn = sqlite3.connect(DB_NAME)
    cursor = conn.cursor()
    cursor.execute("DELETE FROM vitals WHERE patient_id = ?", (patient_id,))
    conn.commit()
    conn.close()

def export_to_csv(patient_id):
    conn = sqlite3.connect(DB_NAME)
    cursor = conn.cursor()
    cursor.execute("SELECT * FROM vitals WHERE patient_id = ?", (patient_id,))
    rows = cursor.fetchall()
    
    # Get column names
    headers = [description[0] for description in cursor.description]
    filename = f"{patient_id}_history.csv"
    
    with open(filename, 'w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow(headers)
        writer.writerows(rows)
    conn.close()
    return os.path.abspath(filename)

def get_all_patient_ids():
    """Returns a list of all unique patient IDs currently in the database."""
    conn = sqlite3.connect(DB_NAME)
    cursor = conn.cursor()
    cursor.execute("SELECT DISTINCT patient_id FROM vitals")
    rows = cursor.fetchall()
    conn.close()
    return [row[0] for row in rows]

# Initialize the DB when the module is loaded
init_db()