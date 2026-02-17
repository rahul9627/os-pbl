import requests
import sys

try:
    print("Testing OS PBL Backend API...")
    # The backend expects a 'GET' request to trigger the 'D' command
    response = requests.get('http://localhost:5000/api/detect_deadlock')
    
    if response.status_code == 200:
        print("SUCCESS: Connected to Backend Server.")
        print(f"Response JSON: {response.json()}")
    else:
        print(f"FAILURE: Server returned status code {response.status_code}")
        print(f"Response: {response.text}")

except Exception as e:
    print(f"ERROR: Could not connect to OS PBL Backend. {e}")
