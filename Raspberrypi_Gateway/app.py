from flask import Flask, render_template, jsonify, request, send_file
import medical_database as db
import os

app = Flask(__name__)

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/api/patients', methods=['GET'])
def get_patients():
    # AUTO-DISCOVERY: Fetch all unique patients directly from the database
    patient_ids = db.get_all_patient_ids()
    data = []
    
    for pid in patient_ids:
        latest = db.get_latest(pid)
        if latest:
            hr, spo2, temp, accident, timestamp = latest
            data.append({
                'patient_id': pid,
                'heart_rate': hr,
                'spo2': spo2,
                'temperature': temp,
                'accident': accident,
                'timestamp': timestamp
            })
    return jsonify(data)

@app.route('/api/patients/add', methods=['POST'])
def add_patient():
    pid = request.json.get('patient_id')
    
    if db.check_patient_exists(pid):
        return jsonify({'status': 'error', 'message': 'Patient is already on the dashboard.'})
    
    # Inject a placeholder record so they appear instantly, waiting for ESP32 data
    placeholder_data = {
        "patient_id": pid,
        "heart_rate": 0,
        "spo2": 0,
        "temperature": 0.0,
        "serious_accident": 0
    }
    db.insert_reading(placeholder_data)
    
    return jsonify({'status': 'success'})

@app.route('/api/patients/delete', methods=['POST'])
def delete_patient():
    pid = request.json.get('patient_id')
    db.delete_patient(pid) # Wipes them from the database so they disappear
    return jsonify({'status': 'success'})

@app.route('/api/patients/<pid>/history')
def get_history(pid):
    history = db.get_history(pid)
    if not history:
        return jsonify({'error': 'No data'})
        
    return jsonify({
        'hrs': [row[0] for row in history],
        'spo2s': [row[1] for row in history],
        'temps': [row[2] for row in history],
        'times': [row[3] for row in history]
    })

@app.route('/api/patients/<pid>/csv')
def get_csv(pid):
    filepath = db.export_to_csv(pid)
    return send_file(filepath, as_attachment=True)

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)