# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [V8.1.2] - 2024-05-26

### Added
- **WiFi Configuration Portal:** Implemented a captive portal (SoftAP) that automatically starts when the device fails to connect to a known WiFi network. This allows for easy WiFi setup via a web page served by the ESP32 itself.
- **Web-Based Configuration:** Added new forms to the "System Setup" section of the web dashboard, allowing users to update WiFi credentials and all API keys directly from the UI without needing to re-flash the device.
- **Sensor Simulation Mode:** Added a `SIMULATE_SENSORS` macro to allow for testing and development of the web interface and other logic without requiring physical sensors to be connected.

### Changed
- **Configuration Persistence:** Reworked the configuration file (`config.json`) to use a more organized, nested JSON structure. The firmware's `saveConfig` and `loadConfig` functions have been completely refactored to support this new format.
- **WebSocket Message Handling:** Refactored the `handleWebSocketMessage` function into smaller, more manageable handlers (`handleToggleMessage`, `handleConfigMessage`, `handleApiKeyMessage`, `handleFormMessage`) for improved code clarity and maintainability.
- **Historical Data Processing:** The `calcWeather` function now aggregates sensor data every second and stores a 1-minute average in a 24-hour circular buffer. The web UI charts are now populated with 10-minute averages calculated from this buffer, improving performance and data representation.
- **Project Documentation:** Significantly updated `projectSummary.md` to provide a comprehensive overview of the project's features, architecture, and hardware requirements.

### Fixed
- **API Call Scheduling:** Resolved a critical bug where API calls could be skipped if their scheduled times overlapped. The new logic uses a global cooldown timer (`nextApiCallAllowedTime`) to ensure that API calls are serialized correctly and run at their intended intervals without conflict.

## [V7.1.7] - 2024-05-24

### Added
- Implemented 24-hour historical data collection and charting for Battery Voltage.
- Added a real-time "Uptime" counter to the status sidebar on the web UI.

### Fixed
- Corrected a JavaScript bug where historical wind data was incorrectly displayed on the battery voltage chart.

### Changed
- Enabled 24-hour historical data display for the Rainfall chart.
## [V7.1.6] - 2024-05-24

### Fixed
- Resolved an issue where the device would get stuck in the configuration portal (SoftAP mode) after a temporary WiFi disconnection.
- Fixed log spam (`E (XXXX) wifi:Set status to INIT`) during WiFi connection retries by implementing a cleaner disconnect/reconnect cycle.
- Fixed log spam (`[INFO] Client connected to AP. Timeout reset.`) while a client is connected to the configuration portal's access point.

### Changed
- The device will now attempt to reconnect to WiFi for up to 75 seconds before falling back to the configuration portal, making it more resilient to network glitches.
- The configuration portal will now automatically time out and restart the device after 3 minutes of inactivity to prevent it from getting stuck.
- The RSSI-based reconnection logic is now less aggressive, only triggering on a critically low signal (-90 dBm) to reduce unnecessary reconnects.