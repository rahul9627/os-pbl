from flask import Flask, request, jsonify, send_from_directory
import subprocess
import os

app = Flask(__name__, static_folder='frontend')

# Serve the frontend
@app.route('/')
def index():
    return send_from_directory('frontend', 'index.html')

@app.route('/<path:path>')
def static_proxy(path):
    return send_from_directory('frontend', path)

# Example API endpoint: detect deadlock
@app.route('/api/detect_deadlock', methods=['GET'])
def detect_deadlock():
    # Run the C backend with a command to detect deadlock
    # Adapted for Windows
    proc = subprocess.Popen(['sim.exe'], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    # Send the 'D' command to detect deadlock, then 'E' to exit simulation
    # 'E' kills the current process (Init), which terminates the sim if it's the only one.
    out, err = proc.communicate(input='D\nE\n')
    # Parse output for deadlock status
    if 'Deadlock detected' in out:
        status = 'deadlock'
    else:
        status = 'safe'
    return jsonify({'status': status, 'output': out})

# TODO: Add more endpoints for process/resource management

if __name__ == '__main__':
    app.run(debug=False, host='0.0.0.0', port=5000)