# Project Architecture

This document outlines the system architecture, file structure, and interaction patterns of the ESP32 Weather Station.

## System Components

### 1. Hardware Infrastructure
- **Microcontroller:** ESP32 Dev Board (Lolin D32 / DFRobot Firebeetle)
- **Sensors:**
    - Environmental: AHT20 + BMP280 combo (Standard) or legacy BME280 (I2C).
    - Wind/Rain: Analogue vane, anemometer pulse, reed-bucket tipping gauge.
    - Power: INA219 current/voltage monitor.
- **Peripherals:** WS2812 RGB LED status indicator, optional MicroSD module.

### 2. Firmware (C++/Arduino)
The ESP32 code handles real-time data acquisition, internal buffering, and networking services.
- **Web Server:** `AsyncWebServer` handles static resource requests (`/`, `/style.css`, etc.).
- **WebSockets:** `AsyncWebSocket` feeds real-time, non-blocking JSON data streams to clients.
- **Data Pipelines:** Aggregates raw sensor readings every second into 1-minute averages stored in a circular buffer.
- **Persistence:** Uses the LittleFS internal filesystem for storage of config files and raw telemetry history backups (`rawHistory.dat`).

### 3. Client Web Dashboard (SPA)
A browser-based application constructed with pure frontend technologies for maximum responsiveness.
- **Stack:** HTML5 semantic structure, Bootstrap styling framework, Vanilla Modern Javascript logic.
- **Visuals:** Proprietary rendering logic dynamically creates SVG elements for historical visual analysis and telemetry mini-indicators, ensuring CPU-efficiency with low-resource footprint.
- **Layout Management:** Implements a dynamic sidebar toggle for desktop views, using `localStorage` for state persistence and CSS-driven transitions for smooth layout shifts between expanded (text+icons) and collapsed (icons-only) states.

## File Layout & Purpose

- **`weather_station_v8_3_g_web-fixes.ino`**: Root driver orchestrating hardware cycles, data persistence synchronization, connectivity loops, and the Async TCP Server infrastructure.
- **`data/index.html`**: UI markup containing visual slots for weather cards, charts, status tables, and dynamic configuration modals.
- **`data/update.js`**: Frontend logic controller handling WebSocket duplex streams, DOM manipulation based on telemetry payloads, and advanced SVG generation math.
- **`data/style.css`**: UI appearance assets supporting cross-viewport consistency and Dark/Light theme overlays.

## Configuration & State Ecosystem
1. **`config.json`**: High-sensitivity ecosystem credentials including API endpoint routes, keys, SSIDs, and hard constants.
2. **`varVals.json`**: Low-sensitivity runtime persistence tracking component enable/disable states and non-critical UI toggles.
