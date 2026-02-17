# OS Deadlock Simulator

This project simulates process management and resource allocation in an operating system, with support for deadlock avoidance, detection, and prevention.

## Features
- Process creation, forking, and scheduling
- Resource allocation and release
- Deadlock avoidance (Banker's Algorithm)
- Deadlock detection and prevention
- Command-line interface (CLI)
- Beautiful, interactive web frontend (HTML/CSS/JS)

## Building and Running (CLI)

1. **Build the project:**
   ```sh
   make
   ```
   **Windows:**
   ```sh
   build.bat
   ```
2. **Run the simulation:**
   ```sh
   ./sim
   ```

## CLI Commands

- `C` : Create a process (enter priority 0/1/2)
- `F` : Fork the current process
- `K` : Kill a process (enter PID)
- `E` : Exit the current process
- `Q` : Time quantum expires
- `S` : Send a message
- `R` : Receive a message
- `Y` : Reply to a message
- `N` : Create a new semaphore
- `P` : Semaphore P (wait)
- `V` : Semaphore V (signal)
- `I` : Show process info
- `T` : Show all process queues and resource allocation state
- `D` : Detect deadlock (prints deadlock status)
- `W` : Request resources (enter PID and 5 comma-separated values)
- `w` : Release resources (enter PID and 5 comma-separated values)

## Web Frontend

A modern, interactive frontend is provided in the `frontend/` directory. It allows you to:
- Add/remove processes and resources
- Request/release resources
- Visualize allocations and deadlock status

### Running the Frontend

You can use any static file server. For example, with Python:

```sh
cd frontend
python -m http.server 8000
```

Then open [http://localhost:8000](http://localhost:8000) in your browser.

**Note:** The frontend currently uses mock data. To connect it to the backend, you would need to implement a web API wrapper for the C backend (see project issues for ideas).

## Requirements
- GCC or compatible C compiler
- `make`
- Python (for serving the frontend, optional)

## License
MIT 