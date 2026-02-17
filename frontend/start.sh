#!/bin/bash

# Go to project root and start backend in background
cd "$(dirname "$0")/.."
make
./sim < /dev/null &

# Go to frontend and start the web server
cd frontend
python3 -m http.server 8000 &

# Open browser automatically (Linux/WSL)
sleep 2
xdg-open http://localhost:8000 