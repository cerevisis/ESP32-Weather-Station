# ESP32-Weather-Station
This project is an advanced ESP32-based weather station with a full-featured web interface and integration with multiple public weather services like Windy.com, Wunderground.com, and ThingSpeak. It reads data from environmental sensors, processes it, serves a real-time web dashboard, and uploads data to various weather APIs. Configuration is managed via the web UI and stored persistently on the device's filesystem.

**Hardware Note:** The project has transitioned from the BME280 sensor to a more robust AHT20 + BMP280 combination for environmental readings; the BME280 is now considered deprecated for new builds.


## Full documentation and hardware build guide in progress
- Custom PCB design
- STL 3D printing files for
  - Custom helical radiation screen
  - Solar panel mounts

## Key Features

*   **Platform:** Runs on most ESP32-based boards (tested on Lolin D32, DFRobot Firebeetle).
*   **Sensor Integration:**
    *   AHT20 + BMP280 (current standard) or legacy Bosch BME280, for temperature, relative humidity, and air pressure.
    *   Analogue wind vanes and anemometers (e.g., SparkFun Weather Meters).
    *   Reed-switch based rain tipping buckets.
    *   INA219 for onboard voltage and current measurement.
*   **API Uploads:**
    *   Natively pushes data to Windy.com, Wunderground.com, ThingSpeak, and any MQTT broker.
    *   **Home Assistant Integration:** Built-in MQTT Auto-Discovery support registers the station as a full device with all sensors in Home Assistant automatically.
    *   The structure is easily extensible to support additional services.
*   **Web Dashboard:**
    *   A full-featured, responsive, app-like UI built on Bootstrap, served directly from the device.
    *   Displays real-time sensor readings and daily weather records.
    *   Renders 24-hour historical data charts.
    *   Renders historical data charts with configurable time windows (e.g., 1h, 24h). Charts feature interactive tooltips for data inspection, and the wind chart features dynamic indicators for speed and direction.
    *   **Robust Connectivity:** Includes a WebSocket connection status indicator and a client-side watchdog timer to detect disconnections and auto-reconnect within seconds.
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
*   **Sensors:** AHT20 + BMP280 combination (standard) or BME280 (deprecated) for temperature, humidity, and pressure; INA219 for power monitoring; and analogue/pulse-based sensors for wind speed, wind direction, and rainfall.
*   **Connectivity:** Connects to a local WiFi network. If the connection fails, it automatically starts a SoftAP configuration portal for setup.
*   **Firmware (C++/Arduino):** The ESP32 firmware manages sensor readings, maintains a 24-hour historical data buffer, handles API uploads, and runs an `AsyncWebServer` with `AsyncWebSocket` support for non-blocking communication with the web client. It includes intelligent WiFi roaming to automatically connect to the strongest access point.
*   **Web Interface (SPA):** A responsive single-page application served from the ESP32's LittleFS. It's built with HTML, Bootstrap for styling, and vanilla JavaScript. It provides real-time data visualization through custom-built SVG charts and allows for system configuration.
*   **Data Persistence:** Configuration for WiFi and API keys (`config.json`), persistent UI state variables (`varVals.json`), and system logs (`systemLog.csv`) are stored on the ESP32's LittleFS filesystem.
 
*   **Raw History Persistence:** The raw, 1-minute resolution historical data is saved to `rawHistory.dat`, allowing the station to retain its full history across reboots.
## Hardware requirements

*   **ESP32 Dev Board:**
    *   Tested on Lolin D32, DFRobot Firebeetle, but most ESP32 boards are compatible.
*   **Environmental Sensors:**
    *   **Temp/Humidity/Pressure:** AHT20 + BMP280 combination (recommended) or legacy BME280 sensor breakout board.
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
- Your device should automatically prompt you to "Sign in to network," which will open the configuration page. If not, open a browser and navigate to http://192.168.4.1.
- You will be presented with a captive portal where you can enter your WiFi details.
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

### MQTT / Home Assistant
- **Broker Configuration:** Enter your MQTT broker's IP or hostname, port (default 1883), and authentication credentials (if required) in the web dashboard.
- **Home Assistant Auto-Discovery:** When MQTT is enabled, the station automatically publishes discovery payloads. In Home Assistant, navigate to **Settings > Devices & Services > MQTT**, and you should see your station appear as a new device with all its sensors ready to use.

## LED patterns and their meanings:

1) Booting / Initializing
- Pattern: Slow Blue Pulse
- Use Case: Shown once when the device first powers on and starts its setup sequence.
- Success / OK

2) Pattern: Quick Green Double-Flash
- Use Case: Indicates any successful operation, such as completing setup, connecting to WiFi, syncing time, or a successful API data upload.
- In Progress / Working

3) Pattern: Pulsing Cyan
- Use Case: Shows the device is actively working on a task, like attempting to connect to a WiFi network.
- Warning / Recoverable Error

4) Pattern: Slow Orange Pulse
- Use Case: Signals a non-critical error where the system can recover or retry, such as a failed API call, a temporary sensor read error, or a periodic NTP sync failure.
- Critical Failure / Action Required

5) Pattern: Fast Red Flashing
- Use Case: Alerts you to a major failure that requires attention or will lead to a system action, like failing to connect to WiFi after all retries (which triggers the config portal) or an impending system restart.


## Known Issues
1) Be Careful of using the "Debug" toggle with Terminal enabled, as you will flood the websocket with messages and this can cause and ESP32 crash
2) The Status LED code is not very performant and can block some tasks (it works by default at boot time only). Use it for debugging only.
3) Only use the error logging if you have sufficient memory on your ESP32 such as the 8MB or 16MB variants

## Web App Screenshots
![mobile-UI](https://raw.githubusercontent.com/cerevisis/ESP32-Weather-Station/refs/heads/main/esp32-weather-station-web-app-mobile.png)
![desktop-UI](https://raw.githubusercontent.com/cerevisis/ESP32-Weather-Station/refs/heads/main/esp32-weather-station-web-app-desktop.png)
