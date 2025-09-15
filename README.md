# Rover Firmware ESP32

This repository contains the firmware for the **ESP32 microcontroller** of the Rover Project, developed as part of the **Project Workshop 1** course at the **National University of La Plata**.  

The ESP32 is responsible for **receiving commands from an external web server, communicating with the CIAA microcontroller via UART**, and providing **real-time video streaming** for remote monitoring. This firmware is implemented with **ESP-IDF** and FreeRTOS to ensure robustness, multitasking, and efficient handling of concurrent operations.

## Key Responsibilities

- **Web Communication**
  - Connects to an external Wi-Fi network.
  - Maintains a persistent connection with the backend server via HTTP or WebSockets.
  - Parses incoming JSON commands and prepares them for UART transmission.

- **UART Communication**
  - Sends parsed commands to the **CIAA microcontroller** for execution.
  - Handles buffering and prioritization to avoid data loss.
  - Implements a simple, robust communication protocol for rover control.

- **Camera Streaming**
  - Captures live video frames and streams them to the web interface.
  - Supports MJPEG streaming for browser compatibility.
  - Runs concurrently with other tasks using FreeRTOS.

- **Task Management (FreeRTOS)**
  - Separates Wi-Fi, UART, and camera tasks into concurrent threads.
  - Implements priority management to ensure real-time performance.
  - Provides mechanisms for reconnection, error handling, and task synchronization.

## Architecture Overview

The ESP32 firmware operates as an intermediary between the **web server** and the **CIAA microcontroller**, while managing real-time video streaming.  

## License

This repository is licensed under the MIT License. See the LICENSE file for details.

---

Note: This firmware is part of the larger Rover Project, which includes other components such as the CIAA microcontroller firmware, server backend, and web interface. This repository focuses solely on the ESP32 responsibilities.
