# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [V8.3.9] - 2026-05-16
### Changed
- **Documentation:** Updated `README.md` and `projectarchitecture.md` to officially reflect the transition from BME280 (deprecated) to the AHT20 + BMP280 combination as the standard hardware build.
- **UI Labeling:** Updated the sensor toggle label in the web interface from "BME280" to "AHT20/BMP280" for better clarity and hardware alignment.

## [V8.3.8] - 2026-05-16

### Added
- **Desktop Sidebar Toggle:** Implemented a new hamburger menu for desktop view that allows collapsing the sidebar to an icon-only list.
- **Responsive Layout:** The main dashboard container now dynamically adjusts its width and margin to remain alongside the sidebar in both expanded and collapsed states.
- **State Persistence:** The sidebar's expanded/collapsed state is now persisted in the browser's `localStorage`.
- **UI Refinement:** Standardized navigation item structure and standardized icon sizes to a consistent 24px.
- **Fixed Branding:** Decoupled the logo and title from the sidebar grid to ensure they remain fixed next to the toggle button during layout shifts.
- **Smooth Animation:** Eliminated vertical and horizontal "jumpiness" in the sidebar toggle by standardizing padding and using flexbox alignment across both expanded and collapsed states.
- **Fixed Icon Positioning:** Locked the horizontal position of icons so they remain at exactly the same coordinates (centered at 36px) in both expanded and collapsed states.
- **Improved Iconography:** Replaced the "Charts" icon with a data-centric bar chart, updated the Config icon to a gear, and transitioned the Dark Mode toggle to a modern Sun/Moon icon toggle.
- **Refined Sizing:** Optimized navigation icons to a sleeker 20px size and adjusted padding to 26px to maintain perfect centering.
- **Unified Navigation Toggle:** Moved the hamburger menu from the header into the sidebar itself, and updated the desktop icon to a descriptive 'sidebar' toggle with dynamic 'Expand/Collapse' text.
- **Mobile Header Refinement:** Removed the border/shadow from the mobile hamburger menu and vertically centered it within the header for a more balanced aesthetic.
- **Semantic HTML Improvement:** Converted the navigation brand from an anchor tag to a proper `<h1>` element for better document structure and SEO.
- **Mobile Branding Adjustment:** Optimized the header branding font size to 16px on mobile devices for better space utilization.
- **New Section Navigation:** Added a 'Controls' navigation entry to the sidebar, linked to the new system overrides section.
- **System Configuration Accordions:** Reorganized the system configuration section into collapsible accordions (Wi-Fi, Timezone, Cloud APIs, MQTT), significantly reducing vertical scroll depth and improving navigation.
- **Controls Section Reorganization:** Restructured the controls interface into semantic groups (Hardware, Connectivity, System) and implemented a full-width diagnostic area for the Terminal, improving both balance and workflow.
- **Light Mode Contrast Fix:** Improved the visual hierarchy in light mode by adding subtle borders and soft shadows to weather cards, and setting a clean #f7f7f7 body background.
- **Status UI Redesign:** Completely overhauled the sidebar status table into a modern card-based layout with improved typography and semantic structure.
- **Connection Indicator:** Implemented a visual connection status dot that glows green with a checkmark when connected, and red with an 'X' when disconnected.

## [V8.3.7] - 2026-05-11

### Fixed
- **Mini-Chart Zoom Sensitivity:** Overhauled dynamic bound constraint algorithm in `update.js` to drastically decrease fixed padding constraints from 5% down to 0.5% of sensor scale. This successfully forces a 10x increase in rendering sensitivity, guaranteeing micro-fluctuations (e.g. 0.1 degrees) occupy large, prominent visual segments of the chart vertical space instead of vanishing into flat pixels.


## [V8.3.6] - 2026-05-11

### Fixed
- **Historical Precision:** Enhanced the `createHistoricalSVGChart` signature with configurable decimal precision and elevated the Battery Voltage historical chart to render at 2 decimal places for finer structural monitoring.

## [V8.3.5] - 2026-05-11

### Fixed
- **Mini-Chart Auto-Scaling:** Resolved critical runtime bug where uninitialized `null` buffers collapsed the chart boundary range to zero via JS `Math.min` cast evaluation, yielding artificially flattened telemetry lines.
- **Dummy Data Intrusion:** Terminated creation of fake 'midpoint' dataset injections in update loop, securing authentic baseline visualization on interface initialization.

## [V8.3.4] - 2026-05-11

### Changed
- **Documentation:** Updated `README.md` to officially list support for the AHT20 + BMP280 sensor combination in the features overview, system architecture, and hardware requirements sections, ensuring parity with the codebase.

## [V8.3.3] - 2026-03-15

### Added
- **MQTT Integration:** Full support for publishing real-time sensor data (Temp, Humidity, Pressure, Wind, Rain, Battery, RSSI) to an MQTT broker.
- **Home Assistant MQTT Discovery:** Implemented automatic registration so the weather station appears as a "Device" with 10 sensors in Home Assistant without manual YAML configuration.
- **MQTT UI Controls:** Added a dedicated setup section for MQTT Broker settings (host, port, topic, authentication) and a global enable/disable toggle in the web dashboard.
- **Improved MQTT Reliability:** Added success/fail status indicators and a non-blocking reconnection logic.

### Fixed
- **Configuration Buffer Overflow:** Increased the web server's JSON buffer size (`pageValsDoc`) to 2048 bytes to prevent configuration data from being truncated when more setup fields are added.
- **UI Toggle Sync:** Fixed a synchronization bug in the web frontend where the MQTT toggle state would not correctly reflect the saved system status upon page refresh.

## [V8.3.2] - 2026-01-16

### Added
- **Interactive Chart Tooltips:** Added hovering tooltips to all historical SVG charts, displaying precise values, timestamps, and wind gust data.
- **WebSocket Status Indicator:** Added a "Connection" row to the sidebar status table, showing real-time connection state (Connected/Disconnected).
- **Client-Side Watchdog:** Implemented a 10-second heartbeat mechanism in the web interface to detect MCU power loss or hangs much faster than standard TCP timeouts, enabling quicker auto-reconnection.

### Changed
- **Daily Records UI:** Redesigned the "Daily Weather Records" section with "Min/Max" labels and improved Range Bars.
- **Dynamic Range Scaling:** Range bars for Temperature, Humidity, and Pressure now dynamically scale to showcase daily variations (1.5x span), while Wind and Rain bars retain a fixed zero-start for context.
- **Rain UI:** Consolidated "Rain Rate" and "Daily Rain" into a single "Rainfall" card to save screen space and improve data density.
- **Wind Chart Visuals:** Enhanced the historical wind chart by drawing direction triangles for every second data point (reducing clutter) and implementing aggressive non-linear size scaling to visually highlight strong wind events.

### Fixed
- **Pressure Chart Scaling:** Fixed a bug where the historical pressure chart would force a 0-start on the Y-axis, causing the data (e.g., ~1000hPa) to bunch at the top and the scale to show negative values. It now scales dynamically based on the data range.
- **Duplicate ID Conflicts:** Resolved HTML ID conflicts in the Daily Records section that were preventing data updates.

## [V8.3.1] - 2025-11-25

### Changed
- **Sensor Library:** Replaced the single BME280 sensor logic to support a separate AHT20 for Temperature/Humidity and a BMP280 for Pressure readings. This included updating sensor initialization, detection, and reading functions.
- **Historical Wind Chart:** Differentiated the wind speed average line on the 24-hour chart by styling it as a dashed line, improving visual clarity against wind gust markers and direction indicators.

### Fixed
- **INA219 Sensor Logging:** Resolved an issue that caused the serial monitor to be flooded with "Failed to find INA219 chip" errors when the sensor is not connected. A non-blocking retry mechanism with a 30-second interval has been implemented to prevent log spam.
- **Wind Direction Charting:** Corrected the orientation of wind direction indicators on the 24-hour historical chart, which were previously rendered 180 degrees off.
- **BMP280 Compilation Error:** Fixed a compilation error caused by an incorrect function signature in `bmp.begin()`, which was incompatible with the updated Adafruit BMP280 library.

---

## [V8.1.5] - 2025-11-05

### Added
- **WiFi Roaming:** Added a "Force Scan & Reconnect" button to the web UI. This triggers a scan for the best access point with the same SSID and reconnects to it, improving stability in mesh or multi-AP networks.
- **Automatic WiFi Roaming:** Implemented a new function `autoWifiScanCheck` that periodically checks the WiFi signal strength (RSSI). If the signal is below a user-configurable threshold, it automatically triggers the new scan-and-reconnect logic to switch to a stronger access point. This feature can be enabled/disabled and its interval configured from the UI.

### Changed
- **Captive Portal:** Significantly improved the reliability of the SoftAP configuration portal. It now uses a more robust handler that correctly triggers the "Sign in to network" pop-up on most modern mobile devices (iOS, Android) for a seamless setup experience.
- **WiFi Monitoring:** The `monitorWiFiRSSI` function has been simplified to only handle the regular updating of the RSSI value for the UI, separating it from the new automatic reconnection logic.

### Fixed
- **WiFi Scan Logic:** Resolved a race condition where an asynchronous event handler (`handleAsyncWiFiScan`) was prematurely deleting WiFi scan results before the synchronous `forceWifiScanAndReconnect` function could process them.
- **Web UI:** Fixed a CSS issue where the sticky header would disappear when scrolling down the page by removing a fixed height on the `body` element.

## [V8.1.4] - 2025-10-23

### Fixed
- Changed Windy API request from GET to POST to resolve error 400 status code

## [V8.1.3] - 2024-05-26

### Added
- **Timezone Configuration:** Added a new section in the web UI for setting the station's timezone.
- Includes a dropdown list of all UTC offsets.
- Includes a toggle for enabling/disabling Daylight Saving Time (adds 1 hour).

### Changed
- **Configuration Persistence:** The selected timezone offset and daylight savings setting are now saved to `config.json` under the `location` section.
- **Firmware:** The ESP32 now loads the timezone settings on boot and applies them for correct local time calculation. The WebSocket message handler was updated to process timezone changes.
- **Web Interface:** The UI now sends timezone changes via WebSocket and correctly displays the currently saved settings on page load.

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