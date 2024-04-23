# ESP32-Weather-Station
Comprehensive open source ESP32 based weather station software and hardware with built in web server that can publish weather data to both Windy.com and Wunderground.com 


## Full documentation and hardware build guide in progress
- Custom PCB design
- STL 3D printing files for
  - Custom helical radiation screen
  - Solar panel mounts

## Features
- Runs on most ESP32 based boards
- Sensor compatibility
  - Supports popular Bosch BME280 sensor for reading temperature, relative humidity and air pressue
  - Analogue wind vanes and anemometers
  - Reed-switch rain tipping bucket
  - INA219 based voltage and current measurement
- Upload to various weather sites such as Windy.com and Wunderground.com as well as Thingspeak for data storage
  - Requires free accounts and API keys for each
  - Supports all required API variables
  - Additional can be easily added
- WS2182 RGB LED status light with various error and status colour patterns
- Full featured web dashboard built on Bootstrap
  - Local CSS and JS files
  - App-like Dashboard style UI
  - Daily weather records
  - Weather history charts linked to Thingspeak cloud storage
  - Sensor, API and system controls
  - Built-in web serial terminal (read only) with extra debug prints
  - Log levels for easy ready
  - CSV logging to LittleFS or SD card (based on your hardware)
  - Dark and light mode
  - API and other system stats
  - Supports PWA but requires HTTPS and SSL certificate capabale web server (not implemented yet)
 
## Hardware requirements
- ESP32 dev board
  - Tested on Lolin D32, DFRobot Firebeetle
- Temperature, relative humidity and air pressue
  - BME280 sensor breakout board
- Analogue wind vanes and anemometers
  - Recommended from Sparkfunhttps://www.sparkfun.com/products/15901
- Voltage and current measurement
  - INA219 current/voltage sensor
- LED status light
  -  WS2182 RGB LED
-  MicroSD breakout board


![mobile-UI-top-half](https://github.com/cerevisis/ESP32-Weather-Station/assets/66214741/be5743fe-d321-40f5-a2d3-4645243ab7ec)
![desktop-UI-top-half](https://github.com/cerevisis/ESP32-Weather-Station/assets/66214741/a5ab6faf-d83d-4a5c-a5b2-fa1006c1890f)
![desktop-UI-bottom-half](https://github.com/cerevisis/ESP32-Weather-Station/assets/66214741/8954c201-d2b7-4794-ac47-8a2b4c2748c4)
