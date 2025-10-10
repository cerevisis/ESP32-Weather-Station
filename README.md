# ESP32-Weather-Station
This project is an advanced ESP32-based weather station with a full-featured web interface and integration with multiple public weather services like Windy.com, Wunderground.com, and ThingSpeak. It reads data from environmental sensors, processes it, serves a real-time web dashboard, and uploads data to various weather APIs. Configuration is managed via the web UI and stored persistently on the device's filesystem.


## Full documentation and hardware build guide in progress
- Custom PCB design
- STL 3D printing files for
  - Custom helical radiation screen
  - Solar panel mounts

## Key Features

*   **Platform:** Runs on most ESP32-based boards (tested on Lolin D32, DFRobot Firebeetle).
*   **Sensor Integration:**
    *   Bosch BME280 for temperature, relative humidity, and air pressure.
    *   Analogue wind vanes and anemometers (e.g., SparkFun Weather Meters).
    *   Reed-switch based rain tipping buckets.
    *   INA219 for onboard voltage and current measurement.
*   **API Uploads:**
    *   Natively pushes data to Windy.com, Wunderground.com, and ThingSpeak.
    *   The structure is easily extensible to support additional services.
*   **Web Dashboard:**
    *   A full-featured, responsive, app-like UI built on Bootstrap, served directly from the device.
    *   Displays real-time sensor readings and daily weather records.
    *   Renders 24-hour historical data charts.
    *   Provides system, sensor, and API control toggles.
    *   Includes a built-in web serial terminal with selectable log levels for easy debugging.
    *   Supports both dark and light modes.
*   **Data Logging:**
    *   Logs system events and errors to a CSV file on the LittleFS filesystem.
    *   Can be adapted for SD card logging with appropriate hardware.
*   **Status Indicator:** Uses a WS2812 RGB LED to provide visual feedback on system status and errors.
*   **PWA Ready:** Includes a manifest for Progressive Web App installation (requires an HTTPS/SSL proxy, which is not implemented in the project itself).

## System Architecture

*   **Hardware:** ESP32 microcontroller.
*   **Sensors:** BME280 (temperature, humidity, pressure), INA219 (power monitoring), and analogue/pulse-based sensors for wind speed, wind direction, and rainfall.
*   **Connectivity:** Connects to a local WiFi network. If the connection fails, it automatically starts a SoftAP configuration portal for setup.
*   **Firmware (C++/Arduino):** The ESP32 firmware manages sensor readings, maintains a 24-hour historical data buffer, handles API uploads, and runs an `AsyncWebServer` with `AsyncWebSocket` support for non-blocking communication with the web client.
*   **Web Interface (SPA):** A responsive single-page application served from the ESP32's LittleFS. It's built with HTML, Bootstrap for styling, and vanilla JavaScript. It provides real-time data visualization through custom-built SVG charts and allows for system configuration.
*   **Data Persistence:** Configuration for WiFi and API keys (`config.json`), persistent UI state variables (`varVals.json`), and system logs (`systemLog.csv`) are stored on the ESP32's LittleFS filesystem.
 
## Hardware requirements

*   **ESP32 Dev Board:**
    *   Tested on Lolin D32, DFRobot Firebeetle, but most ESP32 boards are compatible.
*   **Environmental Sensors:**
    *   **Temp/Humidity/Pressure:** BME280 sensor breakout board.
    *   **Wind:** Analogue wind vane and anemometer (e.g., SparkFun Weather Meter Kit).
    *   **Rain:** Tipping bucket rain gauge with a reed switch.
*   **Power Monitoring:**
    *   INA219 current/voltage sensor breakout board.
*   **Status Light (Optional):**
    *   A single WS2812 (NeoPixel) RGB LED.
*   **Storage (Optional):**
    *   A MicroSD breakout board can be used for expanded logging.

## File Breakdown

*   **`weather_station_v8_1.ino`**: The main firmware file that orchestrates all device operations. It handles hardware initialization, WiFi management, the web server, WebSocket communication, sensor data processing, historical data aggregation, and API update logic.

*   **`data/index.html`**: The primary HTML file for the web dashboard. Defines the layout for sensor data cards, charts, and configuration forms.

*   **`data/update.js`**: The core client-side JavaScript. It establishes and maintains the WebSocket connection, parses incoming JSON data to update UI elements and charts in real-time, and sends user-submitted configuration changes back to the ESP32. It contains the logic for rendering both the live mini-charts and the detailed 24-hour historical SVG charts.

*   **`data/style.css`**: Provides custom styling for the web dashboard, including responsive design and a dark mode theme.

*   **`data/varVals.json`**: Stores persistent state for UI toggles and other system variables that are not core credentials.

*   **`utilities.h`**: A C++ header file containing utility functions (`readFile`, `writeFile`) for interacting with the LittleFS filesystem, used for loading and saving configuration and logs.

*   **`changelog.md`**: A markdown file documenting the version history and changes made to the project.

## Setup
- On first boot, the ESP32 will attempt to connect to WiFi but since no config is stored it will switch to SoftAP mode after about 75 seconds
- On your device WiFi, look for the SSID "WeatherStation-Setup" and connect to it
- Open a browser and navigate to http://192.168.4.1
- You will be presented with a WiFi config portal where you can enter your WiFi details
  - NB if you enter incorrect details, the ESP32 will attempt to connect 15 times (approximately 75 seconds)
- Once connected, please input your Windy.com, ThingSpeak and Wunderground API details under the setup section

## API Setup
- You will need to setup your own accounts for Windy.com, ThingSpeak and Wunderground.com

### Windy.com
- Open https://stations.windy.com/stations and follow the prompts to complete your station setup and get your API key
- Enter these details under the Windy API setup section
- Enter the Station ID and API Key details under the Windy API setup section
  - NB Station ID is currently removed from the Windy API GET URL as it is only required for multiple stations

### Wunderground.com
- Login or create an account here https://www.wunderground.com/login?action=member-devices
- Click add new device and setup your station
- Enter these details under the Wunderground API setup section
- Enter the Station ID and Station Password details under the Wunderground API setup section

### ThingSpeak
- Login or create an account here https://thingspeak.mathworks.com/login?skipSSOCheck=true
- Follow the steps to create a public channel
  - Setup the Thingspeak fields exactly as per below
    - Field 1: Temperature
    - Field 2: Pressure
    - Field 3: Humidity
    - Field 4: Dew Point
    - Field 5: Wind Direction
    - Field 6: Wind Speed
    - Field 7: Voltage
    - Field 8: Rainfall
  - Access the "API Keys" tab and set up a "Write API Key"
 - Enter the channel ID and API key details under the ThingSpeak API setup section

## TODO
- PWS Weather API is currently commented out across the various files as I need to fix it
- Fix Daily Weather Records table
- Add wind direction indicators to the 24 Wind Speed Graph
- re-add support for SD card storage to write logs to SD card

## Known Issues
1) Be Careful of using the "Debug" toggle with Terminal enabled, as you will flood the websocket with messages and this can cause and ESP32 crash
2) The Status LED code is not very performant and can block some tasks (it works by default at boot time only). Use it for debugging only.
3) Only use the error logging if you have sufficient memory on your ESP32 such as the 8MB or 16MB variants

## Web App Screenshots
![mobile-UI](https://raw.githubusercontent.com/cerevisis/ESP32-Weather-Station/refs/heads/main/esp32-weather-station-web-app-mobile.png)
![desktop-UI](https://raw.githubusercontent.com/cerevisis/ESP32-Weather-Station/refs/heads/main/esp32-weather-station-web-app-desktop.png)
