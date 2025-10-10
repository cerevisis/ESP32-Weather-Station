// VERSION NUMBER - UPDATE ON SAVE!//
/**
   TODO
   > add first boot config for API
   > add changelog to setup page
   > avg metrics trends on web ui
*/

// Define this macro to enable sensor data simulation for testing without hardware
// Comment it out or undefine it to use actual hardware sensors.
// #define SIMULATE_SENSORS

String versionNR = "V8.1.3"; // Incremented version for this change
String versionName = versionNR + " Weather Station";

#include "FS.h"
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <SD.h>
#include "utilities.h"
#include <HTTPClient.h>

#include <ESPAsyncWebServer.h>

#include <AsyncTCP.h>

#include <WiFiUdp.h>
#include "Wire.h"
#include "extra.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_INA219.h>
#include <WiFiClient.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <esp_wifi.h>
#include <DNSServer.h>
#include <ThingSpeak.h>
#include <TimeLib.h>
#include <ArduinoOTA.h>
#include <SPI.h>
#include <Arduino_JSON.h>

char ssid[128] = "";
char password[64] = ""; // Allocate space for password, initialized to empty

// LIVE CREDENTIALS //
// THINGSPEAK SETUP - LIVE
unsigned long myChannelNumber;
String thingSpkAPIwR;
String thingSpkAPIr;
int thingStatus;

// Windy API Setup
String stationID;
String windyAPIKey;

int windyStatus;
// Wunderground API Setup
String wundStationID;
String wundStationPw;
int wundStatus;

// PWSWeather API
int PWSWxStatus;
String PWSWxStationID;
String PWSWxStationPw;



// Station sensor pin assignment definitions
#define WIND_SPD_PIN 25
#define WIND_DIR_PIN 33
#define RAIN_PIN 32

// web server variables
TaskHandle_t Task1; // Task for webserver core
AsyncWebServer ASyncServer(80);
DNSServer dnsServer;
const byte DNS_PORT = 53;

AsyncWebSocket ws("/ws");

// TaskHandle_t APITask;

// TaskHandle_t wxTask;

// Variables for websocket JSON
JSONVar terminalVals;
JSONVar wxVals;
JSONVar wxVarVals;
JSONVar pageVals;
String message = "";
boolean wsStatus = false;

// Files
File logFile;

File wxVariableValues;
const char *varFilename = "/varVals.json";

File configFile;
const char *configFilename = "/config.json";

// Switches
bool uploadSwitch = 0;
bool errorLogSwitch = 0;
bool terminalSwitch = 0;
bool battCheckSwitch = 1;
bool sleepSwitch = 0;
bool debugSwitch = 0;
bool windyApiSwitch = 1;
bool thingSApiSwitch = 1;
bool wundApiSwitch = 1;
bool pwsWXAPISwitch = 1;
bool IoTCSwitch = 1;
bool bmeSwitch = 1;
bool windDirSwitch = 1;
bool windSpdSwitch = 1;
bool rainSwitch = 1;
bool darkModeSwitch = 0;
bool wifiMultiSwitch = false;
bool ledSwitch = 0;

// BME280 Sensor Variables
Adafruit_BME280 bme;
int bmeAdr;
float outsideTemp, sensPress, sensorHum, prevTemp, prevPress, prevHum;
float pressureMin, pressureMax, pressureAvg;
float tempVariance, pressVariance, humVariance, humidityMin, humidityMax, humidityAvg;
float tempOffset = 1.8;
int pressOffset = 458;
bool bmeSensorDetected = false;
bool validBMEReading = false;
bool firstBMEReading = true;
float tempMax, tempMin, tempAvg;
float prevMinT;
float prevMaxT;
// float realFeel;
float dewPoint;
float tempF, pressIn, dewPF;

// Real feel calculation variables
float c1 = -8.78469;
float c2 = 1.611394;
float c3 = 2.338549;
float c4 = -0.14612;
float c5 = -0.01231;
float c6 = -0.01642;
float c7 = 0.002212;
float c8 = 0.000725;
float c9 = -0.0000036;

// INA219 Variables
Adafruit_INA219 ina219;
float volt;
bool ina219Status = false;
float shuntvoltage = 0;
float busvoltage = 0;
float current_mA = 0;
float loadVoltage = 0;
float power_mW = 0;
float energy = 0;
float batMAH = 0;

// Global Variables
bool initialBoot = true;
volatile bool timeSyncStatus = false;
bool wifiStatus = false;
bool systemRestartStatus = false;
bool nanValue = false;
unsigned long lastSecond = 0; // The millis counter to see when a second rolls by
unsigned long sensorSendTimer = 0;

// Wind Variables.
const String windDir[17] = {"N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE", "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW", "N"};
String windDirCd;
volatile bool windStateChange = 0;
volatile bool lastwindStateChange = 0;
volatile unsigned long timeSinceLastTick = 0;
volatile unsigned long lastTick = 0;
float windSpeed = 0.0, windMin, windMax, windAvg, windGMin, windGMax, windGAvg;
volatile unsigned long lastWindIRQ = 0;
int winddir = 0;
bool windVaneStatus = false;
volatile float currentSpeed = 0;
float windspeedKmH = 0;
float windSpeedMph = 0;
float windgustKmH = 0;
float windGustMph = 0;
int windgustdir = 0;
float windspdKmH_avg2m = 0;
int winddir_avg2m = 0;

// variables used in calculating rainfall
volatile bool rainStateChange = 0;
volatile bool lastrainStateChange = 0;
bool rainFirstRead = true;
float rainHourMM = 0; // [rain inches over the past hour)] -- the accumulated rainfall in the past 60 min
float savedRainHourMM;
float rainIn = 0;
volatile float dailyrainMM = 0; // [rain inches so far today in local time]
float dailyRainIn = 0;
float rainPHMin, rainPHMax, rainPHMAvg; // TODO still to add to dashboard
unsigned long raintime, rainlast, raininterval;
int windRainCutoff = 35; // To prevent false rain readings due to mast oscilation, wind speed in km/h above which rain measurements will be skipped
float windSpd3Sec, windSpdAvg3Sec, windSpd60Sec, windSpdAvg60Sec;
float windSpdAvg1min[60];
unsigned long rainTimer = 25;
volatile unsigned long prevRainTimer = 0;

double rainMMStep = 0.1; // calibrated for 200cm2 / 5 tips per 10ml / 2mL per tip

#define WIND_DIR_AVG_SIZE 120
int winddiravg[WIND_DIR_AVG_SIZE]; // 120 ints to keep track of 2 minute average
float windgust_10m[10];            // 10 floats to keep track of 10 minute max
int windgustdirection_10m[10];     // 10 ints to keep track of 10 minute max
volatile float rainMinHour[60];    // 60 floating numbers to keep track of 60 minutes of rain

// WIFI
WiFiMulti wifiMulti;
WiFiClient client;
float rssi;
int wifiRSSIInterval = 600000;
double minWifiRSSI = -86;
String IPaddress;

// NTP Time Setup
const char *ntpServer1 = "time1.google.com";
const char *ntpServer2 = "time.nist.gov";
unsigned long epochTime;
byte timeAttempts = 0;
int epochYear = 1970;
long yourUTC = 2; // Replace with your actual UTC time zone value in hours
long UTCOffset_sec;
int daylightOffset_sec = 0;
tmElements_t unixTimeStamp;
int weekDayIndex, monthIndex;
bool initialTimeSync = false;
int threeSecCounter = 0;
int sixtySecCounter = 0;

// Timer Variables
unsigned long clockTimer = 0;
unsigned long wsTimer = 0;
unsigned long clockSyncTimer = 0;
unsigned long wifiRSSiTimer = 0;
unsigned long wxDataUpload = 1000;
unsigned long variableTimer = 0;
int deepSleepTime = 600;

// API intervals (minutes)
unsigned int wundAPIIntervalMin = 1;
unsigned int tsAPIIntervalMin = 1;
unsigned int windyAPIIntervalMin = 5;

// Next scheduled times (millis)
unsigned long nextWundAPITime = 0;
unsigned long nextTSAPITime = 0;
unsigned long nextWindyAPITime = 0;

String nextWundAPITimeStr = "Not Set";
String nextTSAPITimeStr = "Not Set";
String nextWindyAPITimeStr = "Not Set";

// API call queue flags
bool wundAPIQueued = false;
bool tsAPIQueued = false;
bool windyAPIQueued = false;
unsigned long nextApiCallAllowedTime = 0; // Global timer to enforce a buffer between any two API calls

// Date variables
String currMin, currHour, currSec, currTime, currSWeekDay, currLWeekDay, currDay, currMonth, currYear, currSDate, currLDate, timeStamp;
const String Weekday[7] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
const String shWeekday[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
const String longMonth[12] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};

// LittleFS variables
int tBytesLFS, uBytesLFS, freeBytesLFS;
bool initLittleFS = true;

// memory variablse
int freeMem;
unsigned long memTimer = 0;

// --- 24-Hour Historical Data Storage ---
const int HISTORICAL_DATA_POINTS = 24 * 60; // 1440 minutes in 24 hours

float temp_history[HISTORICAL_DATA_POINTS];
float humidity_history[HISTORICAL_DATA_POINTS];
float pressure_history[HISTORICAL_DATA_POINTS];
float rain_history[HISTORICAL_DATA_POINTS];      // Rain accumulated in that minute
float windspeed_history[HISTORICAL_DATA_POINTS]; // Average windspeed for that minute
float batt_volt_history[HISTORICAL_DATA_POINTS]; // Average battery voltage for that minute
int history_data_index = 0;                      // Current index for storing historical data

// Accumulators for 1-minute averages (used for temp, pressure, humidity)
float temp_sum_for_minute_avg = 0.0;
float humidity_sum_for_minute_avg = 0.0;
float pressure_sum_for_minute_avg = 0.0;
float batt_volt_sum_for_minute_avg = 0.0;
int samples_for_minute_avg = 0; // Number of valid samples collected in the current minute for BME
// Wind speed sum is handled by windSpd60Sec
// Rain for the minute is taken directly from rainMinHour[currMin.toInt()]
int batt_volt_samples_for_minute_avg = 0; // Number of valid battery voltage samples

#ifdef SIMULATE_SENSORS
// Global variables for simulated sensor data
float simulated_outsideTemp_g;
float simulated_sensPress_g;
float simulated_sensorHum_g;
float simulated_currentSpeed_g;
int simulated_winddir_g;
float simulated_rain_this_second_g; // Rain generated in the current 1-second interval

// Static variables for smooth simulation changes
static float s_sim_outsideTemp = 15.0;
static float s_sim_sensPress = 1012.0;
static float s_sim_sensorHum = 60.0;
static float s_sim_currentSpeed = 5.0;
static int s_sim_winddir = 180;

void generateSimulatedSensorData()
{
  // Temperature: changes by -0.1 to +0.1 C per second
  s_sim_outsideTemp += (random(-10, 11) / 100.0);
  if (s_sim_outsideTemp < -5)
    s_sim_outsideTemp = -5; // Min temp -5C
  if (s_sim_outsideTemp > 35)
    s_sim_outsideTemp = 35; // Max temp 35C
  simulated_outsideTemp_g = s_sim_outsideTemp;

  // Pressure: changes by -0.2 to +0.2 hPa per second
  s_sim_sensPress += (random(-20, 21) / 100.0);
  if (s_sim_sensPress < 980)
    s_sim_sensPress = 980; // Min pressure 980 hPa
  if (s_sim_sensPress > 1030)
    s_sim_sensPress = 1030; // Max pressure 1030 hPa
  simulated_sensPress_g = s_sim_sensPress;

  // Humidity: changes by -0.5 to +0.5 %rH per second
  s_sim_sensorHum += (random(-50, 51) / 100.0);
  if (s_sim_sensorHum < 30)
    s_sim_sensorHum = 30; // Min humidity 30%
  if (s_sim_sensorHum > 95)
    s_sim_sensorHum = 95; // Max humidity 95%
  simulated_sensorHum_g = s_sim_sensorHum;

  // Wind Speed: changes by -0.5 to +0.5 km/h per second
  s_sim_currentSpeed += (random(-50, 51) / 100.0);
  if (s_sim_currentSpeed < 0)
    s_sim_currentSpeed = 0; // Min speed 0 km/h
  if (s_sim_currentSpeed > 40)
    s_sim_currentSpeed = random(5, 15); // Gusts down from max 40km/h
  simulated_currentSpeed_g = s_sim_currentSpeed;

  // Wind Direction: changes by -3 to +3 degrees per second
  s_sim_winddir += random(-3, 4);
  if (s_sim_winddir < 0)
    s_sim_winddir += 360;
  if (s_sim_winddir >= 360)
    s_sim_winddir -= 360;
  simulated_winddir_g = s_sim_winddir;

  // Rain: Small chance of 0.1mm rain this second
  simulated_rain_this_second_g = 0.0;
  int rain_chance = random(0, 3600); // Chance per second (e.g., 1 in 3600 for avg 0.1mm/hr if it rained constantly)
  if (rain_chance < 2)
  {                                     // ~0.05% chance of 0.1mm rain this second
    simulated_rain_this_second_g = 0.1; // 0.1 mm of rain
  }
}
#endif

void setup()
{
  Serial.begin(115200);

  xTaskCreate(webServerCode, "webservercode", 15000, NULL, 5, &Task1);

  delay(250);

  unsigned long now = millis();
  nextWundAPITime = now + wundAPIIntervalMin * 60000UL;
  nextTSAPITime = now + tsAPIIntervalMin * 60000UL;
  nextWindyAPITime = now + windyAPIIntervalMin * 60000UL;

  const uint8_t protocol = WIFI_PROTOCOL_LR;
  esp_wifi_set_protocol(WIFI_IF_STA, protocol);

  Wire.begin();

#ifndef SIMULATE_SENSORS
  // Rain sensor pin
  pinMode(RAIN_PIN, INPUT_PULLUP);
  // Wind anemometer (wind speed) sensor pin
  esp_rom_gpio_pad_select_gpio(GPIO_NUM_25); // Use the standard GPIO driver function
  gpio_set_direction(GPIO_NUM_25, GPIO_MODE_INPUT);
  gpio_pullup_en(GPIO_NUM_25);
#else
  serialTerminal(4, "SENSOR SIMULATION ENABLED - SKIPPING RAIN/WIND PIN & ISR SETUP");
  randomSeed(analogRead(0)); // Seed random number generator
#endif

  configTime(UTCOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);
  LittleFS.begin();
  if (!LittleFS.begin())
  {
    // This is a critical failure, as we can't read/write any files.
    // Loop forever with a serial message.
    serialTerminal(3, "An Error has occurred while mounting LittleFS");
    initLittleFS = false;
  }
  else
  {
    serialTerminal(4, "LittleFS mounted");
    initLittleFS = true;
    calcLFSMemory();
  }

  delay(500);
  readVarVals(varFilename);
  delay(250);

  batteryCalc();
  batteryMonitor();
  ledSetup();
  
  // Use the standard "Booting" pattern
  LedBlinker(true, 500, LEDBlue, 500, LEDBlack, 2); // Slow Blue Pulse

  // leds[0] = CRGB::Red;
  // FastLED.show();

  leds[0] = CRGB::Black;
  FastLED.show();

  detectBME280();

  btStop();

  // Load core configuration right before it's needed for WiFi
  loadConfig(); 

  // Establish WiFi connection first, as it's required for time sync.
  enableWiFi();

  // Only attempt to sync time after a successful WiFi connection.
  if (WiFi.status() == WL_CONNECTED)
  {
    // Calculate initial UTCOffset_sec from loaded config value
    UTCOffset_sec = yourUTC * 3600;
    configTime(UTCOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);
    initialTimeSync = true;
    syncTime(0); // Call once to handle initial sync, including retries.
    initialTimeSync = false;
  }

  // OTA Setup
  ArduinoOTA.setHostname(versionName.c_str());
  ArduinoOTA.begin();
  
  // Register the WiFi event handler for async scan
  WiFi.onEvent(WiFiEvent);

  ASyncServer.begin();
  initWebSocket();
  delay(500);
  // Use the standard "Success" pattern
  LedBlinker(true, 100, LEDGreen, 100, LEDBlack, 2); // Quick Green Double-Flash
  initialBoot = false;

  // Initialize historical data arrays
  for (int i = 0; i < HISTORICAL_DATA_POINTS; ++i)
  {
    temp_history[i] = -999.0f;
    humidity_history[i] = -999.0f;
    pressure_history[i] = -999.0f;
    rain_history[i] = 0.0f; // Rain starts at 0
    windspeed_history[i] = -999.0f;
    batt_volt_history[i] = -999.0f;
  }
  serialTerminal(4, "24-hour historical data arrays initialized.");
}

void getMemory()
{
  if (millis() - memTimer > 10000)
  {
    memTimer = millis();
    freeMem = ESP.getFreeHeap();
    serialTerminal(1, "Free Heap: " + String(freeMem) + "K");
  }
}

void notifyClients(String value)
{
  ws.textAll(value);
}

String wxVarValsJSON(String variableVal, String value)
{
  wxVarVals[variableVal] = String(value);

  String jsonString = JSON.stringify(wxVarVals);
  return jsonString;
}

String wxJSONData(String variableVal, String value)
{
  wxVals[variableVal] = String(value);

  String jsonString = JSON.stringify(wxVals);
  return jsonString;
}

String terminalJSONData(String variableVal, String value)
{
  terminalVals[variableVal] = String(value);

  String jsonString = JSON.stringify(terminalVals);
  return jsonString;
}

String pageJSONData(String variableVal, String value)
{
  pageVals[variableVal] = String(value);

  String jsonString = JSON.stringify(pageVals);
  return jsonString;
}

void terminalWebSocket(String value)
{
  if (wsStatus == true)
  {
    if (terminalSwitch == 1)
    {
      notifyClients(terminalJSONData("terminal", value));
    }
  }
}

void serialTerminal(int logLevel, String value)
{
  String logMessage, terminalMsg;
  switch (logLevel)
  {
  case 0:
    logMessage = currTime + "\t" + "[INFO]\t" + value;
    terminalWebSocket(logMessage);
    if (Serial)
      Serial.println(("\t[INFO]\t" + value));

    break;
  case 1:
    if (debugSwitch == 1)
    {
      logMessage = "[DEBUG]\t" + value;
      logErrors(logMessage);
      terminalWebSocket(currTime + "\t" + logMessage);
      if (Serial)
        Serial.println(("\t[DEBUG]\t" + value));
    }
    break;
  case 2:
    logMessage = "[WARNING]," + value;
    logErrors(logMessage);
    terminalMsg = "[WARNING]\t" + value;
    terminalWebSocket(currTime + "\t" + terminalMsg);
    if (Serial)
      Serial.println(("\t[WARNING]\t" + value));

    break;
  case 3:
    logMessage = "[CRITICAL]," + value;
    logErrors(logMessage);
    terminalMsg = "[CRITICAL]\t" + value;
    terminalWebSocket(currTime + "\t" + terminalMsg);
    if (Serial)
      Serial.println(("\t[CRITICAL]\t" + value));

    break;
  case 4:
    logMessage = "[BOOT]," + value;
    logErrors(logMessage);
    terminalMsg = "[BOOT]\t" + value;
    terminalWebSocket(currTime + "\t" + logMessage);
    if (Serial)
      Serial.println(("\t[BOOT]\t" + value));

    break;
  case 5:
    logMessage = "[STATUS]," + value;
    logErrors(logMessage);
    terminalMsg = "[STATUS]\t" + value;
    terminalWebSocket(currTime + "\t" + terminalMsg);
    if (Serial)
      Serial.println(("\t[STATUS]\t" + value));

    break;
  }
}

void calcLFSMemory()
{
  tBytesLFS = LittleFS.totalBytes() / 1024;
  uBytesLFS = LittleFS.usedBytes() / 1024;
  freeBytesLFS = tBytesLFS - uBytesLFS;
  serialTerminal(1, "LittleFS Free:" + String(freeBytesLFS) + "KB | Total:" + String(tBytesLFS) + "KB | Used:" + String(uBytesLFS) + "KB");
}

void logErrors(String errorMessage)
{

  if ((errorLogSwitch == 1) && (initLittleFS == true))
  {
    calcLFSMemory();
    logFile = LittleFS.open("/systemLog.csv", FILE_APPEND);
    int fileSize1 = logFile.size();
    serialTerminal(1, "logFile size: " + String(logFile.size()));

    if ((fileSize1 > 30000) || (freeBytesLFS > 0 && freeBytesLFS < 500))
    {
      clearLogFile();
      calcLFSMemory();
    }
    else if (logFile)
    {

      errorMessage = currSDate + " " + currTime + "," + errorMessage;

      logFile.println(errorMessage);

      logFile.close(); // close the file
      calcLFSMemory();
      serialTerminal(0, "Log data written to LittleFS at " + String(currTime));
    }
  }
  if (!initLittleFS)
  {
    serialTerminal(0, "LittleFS init failed " + String(currTime));
  }
}

void clearLogFile()
{
  logFile.close();
  LittleFS.remove("/systemLog.csv");
  serialTerminal(0, "logFile deleted");
  logFile = LittleFS.open("/systemLog.csv", FILE_APPEND);
  logFile.println("Date,Level,Error Message");
  logFile.close();
  calcLFSMemory();
  serialTerminal(0, "logFile Template RESET");
  serialTerminal(0, "logFile size: " + String(logFile.size()));
}

void serialDebug(String value)
{
  if (debugSwitch == 1)
  {
    terminalWebSocket("[DEBUG] " + value);
    if (Serial)
      Serial.println((value));
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  switch (type)
  {
  case WS_EVT_CONNECT:
    wsStatus = true;
    syncTime(100);
    // Send the full historical data to the newly connected client
    serialTerminal(5, "WebSocket client " + String(client->id()) + " connected from " + client->remoteIP().toString().c_str());
    serialTerminal(0, "Sending full historical data to new client: " + String(client->id()));
    sendHistoricalDataToClients(); // Send full history on new connection

    break;
  case WS_EVT_DISCONNECT:
    wsStatus = false;
    // setModemSleep();
    serialTerminal(5, "WebSocket client " + String(client->id()) + " disconnected");
    break;
  case WS_EVT_DATA:
    handleWebSocketMessage(arg, data, len);
    break;
  case WS_EVT_PONG:
  case WS_EVT_ERROR:
    serialTerminal(4, "ws [" + String(server->url()) + "] [" + String(client->id()) + "] error(" + String(*((uint16_t *)arg)) + "): " + String((char *)data));
    break;
  }
}

void initWebSocket()
{
  ws.onEvent(onEvent);
  ASyncServer.addHandler(&ws);
  serialTerminal(0, "Websocket Initiated");
}
void handleToggleMessage(const String &msg)
{
  // Format from client: e.g., "0D1", "2S0", "1A1"
  // Char 0: Number
  // Char 1: Type (D, T, A, S)
  // Char 2: Value (0 or 1)
  int number = msg.substring(0, 1).toInt();
  char type = msg.charAt(1);
  bool value = msg.substring(2, 3).toInt() == 1;

  serialTerminal(0, "Toggle state: " + msg);

  switch (type)
  {
  case 'D':
    switch (number)
    {
    case 0:
      uploadSwitch = value;
      serialTerminal(0, "Data upload: " + String(value));
      break;
    case 1:
      debugSwitch = value;
      serialTerminal(0, "debugSwitch: " + String(value));
      break;
    case 2:
      errorLogSwitch = value;
      serialTerminal(0, "Data logging: " + String(value));
      break;
    case 3:
      battCheckSwitch = value;
      serialTerminal(0, "Battery Check: " + String(value));
      break;
    case 4:
      sleepSwitch = value;
      serialTerminal(0, "Sleep Mode: " + String(value));
      break;
    case 5:
      darkModeSwitch = value;
      serialTerminal(0, "darkMode: " + String(value));
      break;
    case 6:
      wifiMultiSwitch = value;
      serialTerminal(0, "wifiMultiSwitch: " + String(value));
      break;
    case 7:
      ledSwitch = value;
      serialTerminal(0, "ledSwitch: " + String(value));
      break;
    }
    break;
  case 'T':
    if (number == 0)
    {
      terminalSwitch = value;
      serialTerminal(0, "terminalSwitch: " + String(value));
    }
    break;
  case 'A':
    switch (number)
    {
    case 0:
      thingSApiSwitch = value;
      serialTerminal(0, "thingSApiSwitch: " + String(value));
      break;
    case 1:
      windyApiSwitch = value;
      serialTerminal(0, "windyApiSwitch: " + String(value));
      break;
    case 2:
      wundApiSwitch = value;
      serialTerminal(0, "wundApiSwitch: " + String(value));
      break;
    case 3:
      pwsWXAPISwitch = value;
      serialTerminal(0, "pwsWXAPISwitch: " + String(value));
      break;
    }
    break;
  case 'S':
    switch (number)
    {
    case 1:
      bmeSwitch = value;
      serialTerminal(0, "bmeSwitch: " + String(value));
      break;
    case 2:
      windSpdSwitch = value;
      serialTerminal(0, "windSpdSwitch: " + String(value));
      break;
    case 3:
      windDirSwitch = value;
      serialTerminal(0, "windDirSwitch: " + String(value));
      break;
    case 4:
      rainSwitch = value;
      serialTerminal(0, "rainSwitch: " + String(value));
      break;
    }
    break;
  }
  // Echo the change back to the client
  notifyClients(wxJSONData(String(type) + "toggle" + String(number), String(value)));
}

void saveConfig() {
    DynamicJsonDocument doc(2048); // Increased size for nested structure

    // Create nested structure
    JsonArray wifiArray = doc.createNestedArray("wifi");
    JsonObject wifi_0 = wifiArray.createNestedObject();
    wifi_0["ssid"] = ssid;
    wifi_0["password"] = password;

    JsonArray thingSpkArray = doc.createNestedArray("thingSpkSetup");
    JsonObject thingSpk_0 = thingSpkArray.createNestedObject();
    thingSpk_0["myChannelNumber"] = myChannelNumber;
    thingSpk_0["thingSpkAPIwR"] = thingSpkAPIwR;

    JsonArray windyApiArray = doc.createNestedArray("windyApiSetup");
    JsonObject windyApi_0 = windyApiArray.createNestedObject();
    windyApi_0["stationID"] = stationID;
    windyApi_0["windyAPIKey"] = windyAPIKey;

    JsonArray wundGApiArray = doc.createNestedArray("wundGApiSetup");
    JsonObject wundGApi_0 = wundGApiArray.createNestedObject();
    wundGApi_0["wundStationID"] = wundStationID;
    wundGApi_0["wundStationPw"] = wundStationPw;

    // Add timezone to location section
    doc["location"][0]["yourUTC"] = yourUTC;
    doc["location"][0]["daylightOffset"] = daylightOffset_sec;

    // Assuming PWS is also intended to be nested, creating a new structure for it.
    JsonArray pwsApiArray = doc.createNestedArray("pwsApiSetup");
    JsonObject pwsApi_0 = pwsApiArray.createNestedObject();
    pwsApi_0["PWSWxStationID"] = PWSWxStationID;
    pwsApi_0["PWSWxStationPw"] = PWSWxStationPw;

    File configFile = LittleFS.open(configFilename, FILE_WRITE);
    if (!configFile) {
        serialTerminal(3, "Failed to open config file for writing");
        return;
    }

    if (serializeJson(doc, configFile) == 0) {
        serialTerminal(3, "Failed to write to config file");
    } else {
        serialTerminal(5, "Configuration saved to " + String(configFilename));
    }
    configFile.close();
}

void loadConfig() {
  if (LittleFS.exists(configFilename)) {
    String configFileContent = readFile(LittleFS, configFilename);
    serialTerminal(0, configFileContent);
    if (configFileContent.length() > 0) {
      DynamicJsonDocument doc(2048); // Increased size for nested structure
      DeserializationError error = deserializeJson(doc, configFileContent);
      if (error) {
        serialTerminal(3, "Failed to parse config file: " + String(error.c_str()));
      } else {
        serialTerminal(5, "Loading configuration from " + String(configFilename));
        
        // Load WiFi credentials
        if (doc.containsKey("wifi") && !doc["wifi"].isNull()) {
            strlcpy(ssid, doc["wifi"][0]["ssid"] | "", sizeof(ssid));
            strlcpy(password, doc["wifi"][0]["password"] | "", sizeof(password));
        }

        // Load ThingSpeak settings
        if (doc.containsKey("thingSpkSetup") && !doc["thingSpkSetup"].isNull()) {
            myChannelNumber = doc["thingSpkSetup"][0]["myChannelNumber"].as<unsigned long>() | 0;
            thingSpkAPIwR = doc["thingSpkSetup"][0]["thingSpkAPIwR"].as<String>();
        }

        // Load Windy API settings
        if (doc.containsKey("windyApiSetup") && !doc["windyApiSetup"].isNull()) {
            stationID = doc["windyApiSetup"][0]["stationID"].as<String>();
            windyAPIKey = doc["windyApiSetup"][0]["windyAPIKey"].as<String>();
        }

        // Load Wunderground API settings
        if (doc.containsKey("wundGApiSetup") && !doc["wundGApiSetup"].isNull()) {
            wundStationID = doc["wundGApiSetup"][0]["wundStationID"].as<String>();
            wundStationPw = doc["wundGApiSetup"][0]["wundStationPw"].as<String>();
        }

        // Load Timezone settings from location section
        if (doc.containsKey("location") && !doc["location"].isNull()) {
            yourUTC = doc["location"][0]["yourUTC"] | 2; // Default to 2 if not present
            daylightOffset_sec = doc["location"][0]["daylightOffset"] | 0; // Default to 0
        }

        // Load PWSWeather API settings (assuming a new "pwsApiSetup" structure)
        if (doc.containsKey("pwsApiSetup") && !doc["pwsApiSetup"].isNull()) {
            PWSWxStationID = doc["pwsApiSetup"][0]["PWSWxStationID"].as<String>();
            PWSWxStationPw = doc["pwsApiSetup"][0]["PWSWxStationPw"].as<String>();
        }

        // Print loaded config to serial for debugging
        serialTerminal(4, "Loaded SSID from config: " + String(ssid));
      }
    } else {
      serialTerminal(3, "Failed to open config file for reading.");
    }
  } else {
    serialTerminal(2, "Config file not found. Using default values.");
    // If the file doesn't exist, the hardcoded default values will be used.
    // We can optionally save a default file here.
    saveConfig();
  }
}

void handleConfigMessage(const String &msg)
{
  // Format: C{"C_WIFI_SSID":"new_ssid","C_WIFI_PW":"new_pw",...}
  String jsonString = msg.substring(1); // Remove the 'C' prefix
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, jsonString);

  if (error)
  {
    serialTerminal(3, "Failed to parse config JSON: " + String(error.c_str()));
    return;
  }

  serialTerminal(0, "Received new configuration data.");
  serialTerminal(0, jsonString);

  // Update variables if they exist in the JSON
  if (doc.containsKey("C_WIFI_SSID")) strlcpy(ssid, doc["C_WIFI_SSID"], sizeof(ssid));
  if (doc.containsKey("C_WIFI_PW")) strlcpy(password, doc["C_WIFI_PW"], sizeof(password));
  if (doc.containsKey("C_TS_CH_ID")) myChannelNumber = doc["C_TS_CH_ID"].as<long>();
  if (doc.containsKey("C_TS_API_WR")) thingSpkAPIwR = doc["C_TS_API_WR"].as<String>();
  if (doc.containsKey("C_WINDY_ST_ID")) stationID = doc["C_WINDY_ST_ID"].as<String>();
  if (doc.containsKey("C_WINDY_API_KEY")) windyAPIKey = doc["C_WINDY_API_KEY"].as<String>();
  if (doc.containsKey("C_WUND_ST_ID")) wundStationID = doc["C_WUND_ST_ID"].as<String>();
  if (doc.containsKey("C_WUND_ST_PW")) wundStationPw = doc["C_WUND_ST_PW"].as<String>();
  if (doc.containsKey("C_PWS_ST_ID")) PWSWxStationID = doc["C_PWS_ST_ID"].as<String>();
  if (doc.containsKey("C_PWS_ST_PW")) PWSWxStationPw = doc["C_PWS_ST_PW"].as<String>();

  serialTerminal(5, "Configuration updated. Saving and restarting...");
  saveConfig(); // Save the new configuration to LittleFS
  systemRestart("Configuration updated via web UI.");
}

void handleApiKeyMessage(const String &msg)
{
  // Format: K{"C_TS_CH_ID":"new_id","C_TS_API_WR":"new_key",...}
  String jsonString = msg.substring(1); // Remove the 'K' prefix
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, jsonString);

  if (error)
  {
    serialTerminal(3, "Failed to parse API key JSON: " + String(error.c_str()));
    return;
  }

  serialTerminal(0, "Received new API key configuration data.");
  serialTerminal(0, jsonString);

  // Update variables if they exist in the JSON
  if (doc.containsKey("C_TS_CH_ID")) myChannelNumber = doc["C_TS_CH_ID"].as<long>();
  if (doc.containsKey("C_TS_API_WR")) thingSpkAPIwR = doc["C_TS_API_WR"].as<String>();
  if (doc.containsKey("C_WINDY_ST_ID")) stationID = doc["C_WINDY_ST_ID"].as<String>();
  if (doc.containsKey("C_WINDY_API_KEY")) windyAPIKey = doc["C_WINDY_API_KEY"].as<String>();
  if (doc.containsKey("C_WUND_ST_ID")) wundStationID = doc["C_WUND_ST_ID"].as<String>();
  if (doc.containsKey("C_WUND_ST_PW")) wundStationPw = doc["C_WUND_ST_PW"].as<String>();
  if (doc.containsKey("C_PWS_ST_ID")) PWSWxStationID = doc["C_PWS_ST_ID"].as<String>();
  if (doc.containsKey("C_PWS_ST_PW")) PWSWxStationPw = doc["C_PWS_ST_PW"].as<String>();

  serialTerminal(5, "API Key configuration updated. Saving...");
  saveConfig(); // Save the new configuration to LittleFS without restarting
}

void handleTimezoneMessage(const String &msg)
{
  // Format: Z{"C_TIMEZONE":"2","C_DAYLIGHT_SAVINGS":true}
  String jsonString = msg.substring(1); // Remove 'Z' prefix
  DynamicJsonDocument doc(256);
  DeserializationError error = deserializeJson(doc, jsonString);

  if (error)
  {
    serialTerminal(3, "Failed to parse timezone JSON: " + String(error.c_str()));
    return;
  }

  serialTerminal(0, "Received new timezone configuration.");
  serialTerminal(0, jsonString);

  if (doc.containsKey("C_TIMEZONE"))
  {
    yourUTC = doc["C_TIMEZONE"].as<long>();
  }
  if (doc.containsKey("C_DAYLIGHT_SAVINGS"))
  {
    daylightOffset_sec = doc["C_DAYLIGHT_SAVINGS"].as<bool>() ? 3600 : 0;
  }

  UTCOffset_sec = yourUTC * 3600;
  configTime(UTCOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);
  saveConfig(); // Save new timezone settings
}

void handleFormMessage(const String &msg)
{
  // Format from client: e.g., "0F1000", "2F-80"
  // Char 0: Number
  // Char 1: Type ('F')
  // Chars 2+: Value
  int number = msg.substring(0, 1).toInt();
  String valueStr = msg.substring(2);

  serialTerminal(0, "Form message: " + msg);

  switch (number)
  {
  case 0: // dataRefresh
    wxDataUpload = valueStr.toFloat() * 1000;
    if (wxDataUpload == 0)
      wxDataUpload = 1000;
    serialTerminal(0, "wxDataUpload set to: " + String(wxDataUpload));
    break;
  case 2: // rssilimit
    minWifiRSSI = valueStr.toDouble();
    serialTerminal(0, "minWifiRSSI set to: " + String(minWifiRSSI));
    break;
  case 3: // wifiinterval
    wifiRSSIInterval = valueStr.toFloat() * 1000;
    serialTerminal(0, "wifiRSSIInterval set to: " + String(wifiRSSIInterval));
    break;
  case 4: // windRainCutoff
    windRainCutoff = valueStr.toInt();
    serialTerminal(0, "windRainCutoff set to: " + String(windRainCutoff));
    break;
  case 5: // deepSleepTime
    deepSleepTime = valueStr.toInt();
    serialTerminal(0, "deepSleepTime set to: " + String(deepSleepTime));
    break;
  }
}

void sendAllValuesToClient()
{
  String payload = pageJSONData("Atoggle0", String(thingSApiSwitch));
  payload = pageJSONData("Atoggle1", String(windyApiSwitch));
  payload = pageJSONData("Atoggle2", String(wundApiSwitch));
  payload = pageJSONData("Atoggle3", String(pwsWXAPISwitch));
  // payload = pageJSONData("Dtoggle0", String(uploadSwitch));
  payload = pageJSONData("Dtoggle2", String(errorLogSwitch));
  payload = pageJSONData("Dtoggle1", String(debugSwitch));
  payload = pageJSONData("Dtoggle3", String(battCheckSwitch));
  payload = pageJSONData("Dtoggle4", String(sleepSwitch));
  payload = pageJSONData("Dtoggle5", String(darkModeSwitch));
  payload = pageJSONData("Dtoggle6", String(wifiMultiSwitch));
  payload = pageJSONData("Dtoggle7", String(ledSwitch));
  payload = pageJSONData("Ttoggle0", String(terminalSwitch));
  payload = pageJSONData("Stoggle1", String(bmeSwitch));
  payload = pageJSONData("Stoggle2", String(windSpdSwitch));
  payload = pageJSONData("Stoggle3", String(windDirSwitch));
  payload = pageJSONData("Stoggle4", String(rainSwitch));
  // Add config values to be sent to the client
  payload = pageJSONData("ssid", String(ssid));
  payload = pageJSONData("myChannelNumber", String(myChannelNumber));
  payload = pageJSONData("thingSpkAPIwR", thingSpkAPIwR);
  payload = pageJSONData("stationID", stationID);
  payload = pageJSONData("windyAPIKey", windyAPIKey);
  payload = pageJSONData("wundStationID", wundStationID);
  payload = pageJSONData("wundStationPw", wundStationPw);
  payload = pageJSONData("PWSWxStationID", PWSWxStationID);
  payload = pageJSONData("PWSWxStationPw", PWSWxStationPw);
  payload = pageJSONData("yourUTC", String(yourUTC));
  payload = pageJSONData("daylightOffset", String(daylightOffset_sec));

  notifyClients(payload);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
  {
    data[len] = 0;
    String message = (char *)data;
    serialTerminal(0, "ws message: " + message);

    if (message == "getValues")
    {
      sendAllValuesToClient();
    }
    else if (message == "requestFullHistory")
    {
      serialTerminal(0, "Client requested full historical data.");
      sendHistoricalDataToClients();
    }
    else if (message.length() > 1)
    {
      char type = message.charAt(1); // Type is the second character
      if (type == 'F')
      {
        handleFormMessage(message);
      }
      else if (String("DTAS").indexOf(type) != -1)
      { // Check if type is one of the valid toggle types
        if (message.length() == 3)
        { // Basic validation for toggle messages
          handleToggleMessage(message);
        }
      }
      else if (message.charAt(0) == 'C')
      {
        handleConfigMessage(message);
      }
      else if (message.charAt(0) == 'K')
      {
        handleApiKeyMessage(message);
      }
      else if (message.charAt(0) == 'Z')
      {
        handleTimezoneMessage(message);
      }
    }
    // Save variables after any potential change
    writeVarVals(varFilename);
  }
}

void webServerCode(void *pvParameters)
{
  // --- Main Page Handlers ---
  // Serve index.html for the root URL
  ASyncServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
                 { request->send(LittleFS, "/index.html", "text/html", false); });
  // Also serve index.html if /index or /index.html is requested
  ASyncServer.serveStatic("/index", LittleFS, "/index.html").setCacheControl("max-age=3600");
  ASyncServer.serveStatic("/index.html", LittleFS, "/index.html").setCacheControl("max-age=3600");

  // --- Other HTML Page Handlers ---
  ASyncServer.serveStatic("/charts", LittleFS, "/charts.html").setCacheControl("max-age=3600");
  ASyncServer.serveStatic("/system", LittleFS, "/system.html").setCacheControl("max-age=3600");

  // --- API-like and Action Endpoints ---
  ASyncServer.on("/connect", HTTP_GET, [](AsyncWebServerRequest *request)
                 {
    sleepSwitch = 0;
    request->send(LittleFS, "/index.html", "text/html", false); });
  ASyncServer.on("/disconnect", HTTP_GET, [](AsyncWebServerRequest *request)
                 {
    sleepSwitch = 1;
    request->send(LittleFS, "/index.html", "text/html", false); });
  ASyncServer.on("/systemLog.csv", HTTP_GET, [](AsyncWebServerRequest *request)
                 { request->send(LittleFS, "/systemLog.csv", String(), false); });
  ASyncServer.on("/varVals.json", HTTP_GET, [](AsyncWebServerRequest *request)
                 { request->send(LittleFS, "/varVals.json", String(), false); });
  ASyncServer.on("/restart", HTTP_GET, [](AsyncWebServerRequest *request)
                 {
    request->redirect("/index.html");
    systemRestart("Restart initiated"); });
  ASyncServer.on("/clearerrorlog", HTTP_GET, [](AsyncWebServerRequest *request)
                 {
    serialTerminal(0, "Clearing log file");
    clearLogFile();
    request->redirect("/index"); });

  // --- Static Asset Handler ---
  // This will serve all other static files from the LittleFS root
  ASyncServer.serveStatic("/", LittleFS, "/")
      .setCacheControl("max-age=3600"); // Cache static assets for 1 hour

  // --- 404 Not Found Handler ---
  ASyncServer.onNotFound(notFound);

  vTaskDelete(NULL);
}

void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Not found");
}

bool isValidRainConditions(float windSpeed3s, float currentWindSpeed)
{
  if (windSpeed3s >= windRainCutoff)
  {
    serialTerminal(2, "High wind speed @ " + String(windSpeed3s) + "Km/h - Skipping rain measurement");
    return false;
  }
  return (windSpeed3s < windRainCutoff) && (currentWindSpeed < windRainCutoff);
}

void processRainTip()
{
  dailyrainMM += rainMMStep;
  dailyRainIn = dailyrainMM / 10 / 2.54;
  rainMinHour[currMin.toInt()] += rainMMStep;
  rainlast = raintime;
}

void rainIRQ()
{
  // Check if enough time has passed since last check
  if (millis() - prevRainTimer < rainTimer)
  {
    return;
  }
  prevRainTimer = millis();

  // Read current rain sensor state
  bool currentRainState = digitalRead(RAIN_PIN);

  // Only process if state has changed
  if (currentRainState == lastrainStateChange)
  {
    return;
  }

  // Handle first reading separately
  if (rainFirstRead)
  {
    dailyrainMM = 0;
    rainFirstRead = false;
    lastrainStateChange = currentRainState;
    return;
  }

  // Only process HIGH state (rising edge)
  if (currentRainState == HIGH)
  {
    if (!rainSwitch)
    {
      serialTerminal(2, "Rain sensor off - Skipping rain measurement");
      lastrainStateChange = currentRainState;
      return;
    }

    raintime = millis();
    raininterval = raintime - rainlast;

    // Process valid rain tip if conditions are met
    if (raininterval > 250 && isValidRainConditions(windSpdAvg3Sec, windSpeed))
    {
      processRainTip();
    }
  }

  lastrainStateChange = currentRainState;
}

int windClicks;
void IRAM_ATTR wspeedIRQ()
{
  windStateChange = gpio_get_level(GPIO_NUM_25);
  // compare the windStateChange to its previous state
  if (windStateChange != lastwindStateChange)
  {
    if (windStateChange == HIGH)
    {
      if (millis() - lastTick > 10) // Ignore switch-bounce glitches less than 10ms (142KmH max reading) after the reed switch closes
      {

        windClicks++;
        timeSinceLastTick = millis() - lastTick;
        lastTick = millis();
        // serialTerminal(1, "timeSinceLastTick: " + String(timeSinceLastTick));
      }
    }
  }
  lastwindStateChange = windStateChange;
}

boolean validReading;
// Returns the instantaneous wind speed
// float circumf = 43.98229715;
// float circumfKMH = 1583.3626974;

float deltaTime;
unsigned long lastWindCheck = 0;

#ifdef SIMULATE_SENSORS
float get_wind_speed()
{
  return simulated_currentSpeed_g;
}
#else
float get_wind_speed()
{
  // float wspeed;
  validReading = true;
  if (timeSinceLastTick != 0)
  {
    deltaTime = millis() - lastWindCheck;
    // serialTerminal(1, "deltaTime: " + String(deltaTime));
    deltaTime /= 1000.0;
    // serialTerminal(1, "windClicks: " + String(windClicks));
    windSpeed = windClicks / deltaTime;
    // serialTerminal(1, "windSpeed: " + String(windSpeed));
    windClicks = 0;
    lastWindCheck = millis();
    // windSpeed = circumfKMH / timeSinceLastTick;
    // windSpeed =  windSpeed * 36;
  }

  if (windSpeed < 0.75)
  {
    windSpeed = 0;
  }
  if (windSpeed > 180)
  {
    validReading = false;
  }
  windSpeed *= 2.397644;

  if (validReading)
  {
    return (windSpeed);
  }
  return 0.0; // Return 0 if not valid or no new tick
}
#endif

void calcWeather()
{
  if (millis() - lastSecond >= 1000)
  {
    lastSecond = millis(); // Mark start of this 1-second interval processing

#ifdef SIMULATE_SENSORS
    generateSimulatedSensorData(); // Generate new simulated data for this second
#endif

    // 1. Get current sensor readings for this second
    // If SIMULATE_SENSORS is defined, bmeSensorRead will use simulated values.
    // currentSpeed is updated in loop() by calling get_wind_speed(), which also uses simulated values if defined.

    // Wind speed (currentSpeed) is updated in loop() before this function is called.
    // BME sensor readings are updated here:
    bmeSensorRead(); // Updates outsideTemp, sensPress, sensorHum

    // 2. Accumulate data for 1-minute averages
    if (bmeSensorDetected && validBMEReading)
    { // Use the fresh BME readings
      temp_sum_for_minute_avg += outsideTemp;
      pressure_sum_for_minute_avg += sensPress;
      humidity_sum_for_minute_avg += sensorHum;
      samples_for_minute_avg++;
    }
    // Accumulate battery voltage for 1-minute average
    if (ina219Status)
    {
      batt_volt_sum_for_minute_avg += loadVoltage;
      batt_volt_samples_for_minute_avg++;
    }
    // currentSpeed is added to windSpd60Sec sum below.

    // 3. Process counters and potentially store 60-second averages
    threeSecCounter++;
    sixtySecCounter++;

    windSpd3Sec += currentSpeed;
    windSpd60Sec += currentSpeed;

    if (threeSecCounter >= 3)
    {
      windSpdAvg3Sec = windSpd3Sec / 3.0f; // Use float division
      serialTerminal(1, "windSpdAvg [3 Sec]: " + String(windSpdAvg3Sec));
      threeSecCounter = 0;
      windSpd3Sec = 0;
    }

    if (sixtySecCounter >= 60) // A minute has passed, time to calculate and store averages
    {
      // --- Store 1-minute averaged data into historical arrays ---

      // Calculate and store 1-minute averages for BME
      if (samples_for_minute_avg > 0)
      {
        temp_history[history_data_index] = temp_sum_for_minute_avg / (float)samples_for_minute_avg;
        pressure_history[history_data_index] = pressure_sum_for_minute_avg / (float)samples_for_minute_avg;
        humidity_history[history_data_index] = humidity_sum_for_minute_avg / (float)samples_for_minute_avg;
      }
      else
      {
        // Handle case where no valid BME readings were taken (e.g., sensor off or error)
        temp_history[history_data_index] = -999.0f;
        pressure_history[history_data_index] = -999.0f;
        humidity_history[history_data_index] = -999.0f;
      }

      // Store average wind speed for the minute
      // windSpd60Sec (sum) / sixtySecCounter (count, which is 60)
      windSpdAvg60Sec = windSpd60Sec / (float)sixtySecCounter; // Calculate the average for this minute
      windspeed_history[history_data_index] = windSpdAvg60Sec;

      // Store rain for the minute that just completed
      // rainMinHour[currMin.toInt()] holds the accumulated rain for that specific minute.
      // Ensure currMin is valid and an integer before using as index
      if (currMin.length() > 0)
      {
        rain_history[history_data_index] = rainMinHour[currMin.toInt()];
      }
      else
      {
        rain_history[history_data_index] = 0.0; // Default if currMin is not yet set
      }

      // Calculate and store 1-minute average for battery voltage
      if (batt_volt_samples_for_minute_avg > 0)
      {
        batt_volt_history[history_data_index] = batt_volt_sum_for_minute_avg / (float)batt_volt_samples_for_minute_avg;
      }
      else
      {
        batt_volt_history[history_data_index] = -999.0f;
      }

      // Log the stored historical data (optional, for debugging)
      String histLog = "Stored Hist. Idx:" + String(history_data_index);
      histLog += " T:" + String(temp_history[history_data_index]);
      histLog += " H:" + String(humidity_history[history_data_index]);
      histLog += " P:" + String(pressure_history[history_data_index]);
      histLog += " R:" + String(rain_history[history_data_index]);
      histLog += " W:" + String(windspeed_history[history_data_index]);
      histLog += " V:" + String(batt_volt_history[history_data_index]);
      serialTerminal(0, histLog);

      // Move to the next storage slot in historical arrays (circular buffer)
      history_data_index = (history_data_index + 1) % HISTORICAL_DATA_POINTS;

      // Reset accumulators for BME for the next minute's average
      temp_sum_for_minute_avg = 0.0;
      humidity_sum_for_minute_avg = 0.0;
      pressure_sum_for_minute_avg = 0.0;
      samples_for_minute_avg = 0;
      batt_volt_sum_for_minute_avg = 0.0;
      batt_volt_samples_for_minute_avg = 0;

      // Existing sixtySecCounter related calculations and reset
      serialTerminal(0, "windSpdAvg [60 Sec]: " + String(windSpdAvg60Sec)); // Log the official 60-sec avg

      // Send only the latest minute's data to clients
      sendLatestMinuteDataToClients();

      sixtySecCounter = 0;
      windSpd60Sec = 0; // Reset sum for next minute's wind speed average
    }

#ifdef SIMULATE_SENSORS
    // Handle simulated rain accumulation for the current second
    if (simulated_rain_this_second_g > 0.0)
    {
      if (currMin.length() > 0)
      { // Ensure currMin is valid before using as index
        rainMinHour[currMin.toInt()] += simulated_rain_this_second_g;
      }
      dailyrainMM += simulated_rain_this_second_g;
      dailyRainIn = dailyrainMM / 10 / 2.54; // Keep imperial conversion consistent
      serialTerminal(0, "SIMULATED Rain Added: " + String(simulated_rain_this_second_g) + "mm. MinTotal: " + (currMin.length() > 0 ? String(rainMinHour[currMin.toInt()]) : "N/A") + "mm. Daily: " + String(dailyrainMM) + "mm");
      // simulated_rain_this_second_g is reset in the next call to generateSimulatedSensorData()
    }
#endif
    // 4. Other per-second calculations (wind direction, current min/max, etc.)
    // These use the `outsideTemp` etc. from `bmeSensorRead()` at the start of this 1-second block.

    if (windDirSwitch)
    {
      if (get_wind_direction() == -1)
      {
        windVaneStatus = false;
        serialTerminal(3, "Wind vane not detected");
      }
      else
      {
        windVaneStatus = true;
        winddir = get_wind_direction();
        if (winddir >= 0)
        { // Ensure winddir is not -1 before calculating index
          int windIdx = ((winddir % 360) / 22.5);
          windDirCd = windDir[windIdx];
        }
        else
        {
          windDirCd = "N/A"; // Or some other indicator for invalid direction
        }
      }
    }
    else
    {
      serialTerminal(0, "Wind vane disabled");
    }

    // Calculate amount of rainfall for the last 60 minutes
    rainHourMM = 0 + savedRainHourMM;
    savedRainHourMM = 0;
    for (int i = 0; i < 60; i++)
    {
      rainHourMM += rainMinHour[i];
      rainIn = rainHourMM / 10 / 2.54;
    }

    // Reset rainMinHour every 60 minute
    if (((currMin == "59") || (currMin == "00")) && (currSec.toInt() < 3))
    {
      for (int i = 0; i < 60; i++)
      {
        rainMinHour[i] = 0;
      }
      serialTerminal(0, "60 min rainfall [RESET]");
    }

    if ((currentSpeed > windgustKmH) && (currentSpeed > 1))
    {
      windgustKmH = currentSpeed;
      windGustMph = windgustKmH / 1.60934;
      windgustdir = winddir;
    }

    // Existing min/max for wind (these are instantaneous/gust related, not the 60s average)
    windMax = minMaxValue("max", windMax, currentSpeed, 1);
    windMin = minMaxValue("min", windMin, currentSpeed, 1);
    windGMax = minMaxValue("max", windGMax, currentSpeed, 1); // This is likely for gusts
    windGMin = minMaxValue("min", windGMin, currentSpeed, 1);

    tempMax = minMaxValue("max", tempMax, outsideTemp, -100);
    tempMin = minMaxValue("min", tempMin, outsideTemp, -100);

    humidityMax = minMaxValue("max", humidityMax, sensorHum, 0);
    humidityMin = minMaxValue("min", humidityMin, sensorHum, 0);

    pressureMax = minMaxValue("max", pressureMax, sensPress, 0);
    pressureMin = minMaxValue("min", pressureMin, sensPress, 0);

    // Reset daily rain mm at 23:59:58
    if ((currHour == "23") && (currMin == "59") && (currSec.toInt() >= 58))
    {
      dailyrainMM = 0;
      rainHourMM = 0;
      rainIn = 0;
      dailyRainIn = 0;
      windGMin = 0;
      windGMax = 0;
      windMax = 0;
      tempMin = 0;
      tempMax = 0;
      pressureMax = 0;
      pressureMin = 0;
      humidityMax = 0;
      humidityMin = 0;
    }

    // Reset wind gust every 60 seconds
    if (currSec == "30")
    {
      windgustKmH = 0;
      windGustMph = 0;
    }

    batteryCalc();
    printWeather();
  }
}

float minMaxValue(String comparisonType, float value, float current, float minValue)
{
  if (value == 0)
  {
    value = current;
  }
  if (comparisonType == "min")
  {
    if ((current < value) && (current > minValue))
    {
      value = current;
    }
  }
  if (comparisonType == "max")
  {
    if ((current > value) && (current > minValue))
    {
      value = current;
    }
  }
  return value;
}

void bmeSensorRead()
{
#ifdef SIMULATE_SENSORS
  outsideTemp = simulated_outsideTemp_g;
  sensPress = simulated_sensPress_g;
  sensorHum = simulated_sensorHum_g;

  validBMEReading = true;
  bmeSensorDetected = true;
  nanValue = false;
  firstBMEReading = false;

  // Calculate derived values from simulated data
  // if (outsideTemp >= 27) {
  //   realFeel = c1 + (c2 * outsideTemp) + (c3 * sensorHum) + (c4 * sensorHum * outsideTemp) + (c5 * (pow(outsideTemp, 2))) + (c6 * (pow(sensorHum, 2))) + (c7 * (pow(outsideTemp, 2)) * sensorHum) + (c8 * outsideTemp * (pow(sensorHum, 2))) + (c9 * (pow(outsideTemp, 2)) * (pow(sensorHum, 2)));
  // } else {
  //   realFeel = outsideTemp;
  // }
  dewPoint = outsideTemp - (((100 - sensorHum) / 5));
  tempF = (outsideTemp * 9 / 5) + 32;
  pressIn = (sensPress * 0.02953);
  dewPF = (dewPoint * 9 / 5) + 32;

  // serialTerminal(1, "SIMULATED BME: T:" + String(outsideTemp) + " P:" + String(sensPress) + " H:" + String(sensorHum));
  return; // Skip actual sensor reading
#endif

  // --- Original BME Sensor Reading Logic ---
  if (bmeSwitch == 1)
  {
    if (bme.begin(bmeAdr, &Wire))
    {
      bmeSensorDetected = true;
      serialTerminal(1, "BME280 sensor [OK] - Reading live sensor data");

      float tempReading, pressReading, humReading;
      int sensorSamples = 5;
      // Initialize to 0 before summing up new readings
      float tempSum = 0, pressSum = 0, humSum = 0;
      int validSamplesCount = 0;

      for (int i = 0; i < sensorSamples; i++)
      {
        tempReading = bme.readTemperature();
        pressReading = bme.readPressure() / 100.0F;
        humReading = bme.readHumidity();

        serialTerminal(1, "Sample " + String(i + 1) + ": T:" + String(tempReading) + " P:" + String(pressReading) + " H:" + String(humReading));

        if (isnan(tempReading) || isnan(pressReading) || isnan(humReading) || tempReading < -40 || tempReading > 85 || pressReading < 300 || pressReading > 1100 || humReading < 0 || humReading > 100)
        {
          serialTerminal(3, "Invalid sample from BME280 sensor");
          // Do not set bmeSensorDetected to false here, as other samples might be good.
          // We will check validSamplesCount later.
          nanValue = true; // Indicates at least one NaN or out-of-range reading occurred
        }
        else
        {
          tempSum += tempReading;
          pressSum += pressReading;
          humSum += humReading;
          validSamplesCount++;
          nanValue = false; // Reset if we get a valid reading
        }
      }

      if (validSamplesCount > 0)
      {
        outsideTemp = tempSum / validSamplesCount;
        sensPress = pressSum / validSamplesCount;
        sensorHum = humSum / validSamplesCount;

        prevTemp = outsideTemp;
        prevPress = sensPress;
        prevHum = sensorHum;

        firstBMEReading = false;  // A reading (even if averaged from fewer samples) was attempted
        validBMEReading = true;   // We have at least one valid sample
        bmeSensorDetected = true; // Sensor responded

        serialTerminal(1, "Avg T:" + String(outsideTemp) + " P:" + String(sensPress) + " H:" + String(sensorHum) + " from " + String(validSamplesCount) + " samples.");

        // Real feel calculation
        // if (outsideTemp >= 27) {
        //   realFeel = c1 + (c2 * outsideTemp) + (c3 * sensorHum) + (c4 * sensorHum * outsideTemp) + (c5 * (pow(outsideTemp, 2))) + (c6 * (pow(sensorHum, 2))) + (c7 * (pow(outsideTemp, 2)) * sensorHum) + (c8 * outsideTemp * (pow(sensorHum, 2))) + (c9 * (pow(outsideTemp, 2)) * (pow(sensorHum, 2)));
        // } else {
        //   realFeel = outsideTemp;
        // }

        // dew point calc
        dewPoint = outsideTemp - (((100 - sensorHum) / 5));

        // convert to Imperial for Wunderground
        tempF = (outsideTemp * 9 / 5) + 32;
        pressIn = (sensPress * 0.02953);
        dewPF = (dewPoint * 9 / 5) + 32;
      }
      else
      {
        // All samples were invalid
        serialTerminal(3, "All BME280 samples invalid. Using previous valid values if available.");
        // Keep previous valid values for outsideTemp, sensPress, sensorHum if firstBMEReading is false
        // If it's the first reading and all are bad, they will remain at their initial (likely 0) or uninitialized state.
        validBMEReading = false;
        // bmeSensorDetected might still be true if bme.begin() succeeded but readings failed.
        // If bme.begin() itself failed, bmeSensorDetected would be false from that point.
      }
      serialTerminal(1, "After read bmeSensorDetected: " + String(bmeSensorDetected) + " validBMEReading: " + String(validBMEReading) + " nanValue: " + String(nanValue));
    }
    else // bme.begin() failed
    {
      serialTerminal(3, "No BME280 sensor detected on bme.begin()");
      // Use the standard "Warning" pattern for a recoverable sensor error
      LedBlinker(ledSwitch, 500, LEDOrange, 500, LEDBlack, 3); // Slow Orange Pulse
      bmeSensorDetected = false;
      validBMEReading = false;
      // Attempt to re-detect if it fails.
      // Consider if detectBME280() should be called less frequently to avoid spamming I2C if sensor is truly gone.
      detectBME280();
    }
  }
  else
  {
    serialTerminal(0, "BME280 switch off - skipping measurement");
    validBMEReading = false; // Ensure this is false if sensor is switched off
  }
}

#ifdef SIMULATE_SENSORS
float get_wind_direction()
{
  return (float)simulated_winddir_g;
}
#else
// Original get_wind_direction function
float get_wind_direction()
{
  unsigned int adc;

  adc = analogRead(WIND_DIR_PIN); // get the current reading from the sensor

  if (adc < 130)
    return (112.5);
  if (adc < 200)
    return (67.5);
  if (adc < 235)
    return (90);
  if (adc < 375)
    return (157.5);
  if (adc < 620)
    return (135);
  if (adc < 850)
    return (203);
  if (adc < 1050)
    return (180);
  if (adc < 1550)
    return (22.5);
  if (adc < 1750)
    return (45);
  if (adc < 2300)
    return (247.5);
  if (adc < 2450)
    return (225);
  if (adc < 2730)
    return (337.5);
  if (adc < 3100)
    return (0);
  if (adc < 3250)
    return (292.5);
  if (adc < 3600)
    return (315);
  if (adc < 3950)
    return (270);
  return (-1); // wind vane not connected
}
#endif

unsigned long sensorTimer = 0;
bool sensorSendStatus = false, sleepStatus = true;
int wakeTime = 20;
bool wundAPIcall = false, windAPIcall = false, tsAPIcall = false, pwsWxAPICall = false, IoTCPAPICall = false;

void sendSensorData()
{
  unsigned long now = millis();
  const unsigned long API_BUFFER_MS = 10000UL; // 10-second buffer

  // If we are inside the 10-second buffer from the last API call, do nothing.
  if (now < nextApiCallAllowedTime)
  {
    return;
  }

  // Check each API. If its time is due, execute it and then exit the function.
  // This ensures only one API call runs per invocation of sendSensorData().

  if (now >= nextWundAPITime)
  {
    serialTerminal(0, "Wunderground API due for update.");
    updateWundAPI();
    // Set the next time for THIS API based on its interval.
    nextWundAPITime = now + (wundAPIIntervalMin * 60000UL);
    // Set the global "lock" to prevent ANY API call for the next 10 seconds.
    nextApiCallAllowedTime = now + API_BUFFER_MS;
    nextWundAPITimeStr = printNextAPICallTime(nextWundAPITime, "Wunderground API");
    return; // Only one API per loop
  }

  if (now >= nextTSAPITime)
  {
    serialTerminal(0, "ThingSpeak API due for update.");
    updateThingSpeak();
    nextTSAPITime = now + (tsAPIIntervalMin * 60000UL);
    nextApiCallAllowedTime = now + API_BUFFER_MS;
    nextTSAPITimeStr = printNextAPICallTime(nextTSAPITime, "ThingSpeak API");
    return;
  }

  if (now >= nextWindyAPITime)
  {
    serialTerminal(0, "Windy API due for update.");
    updateWindyAPI();
    nextWindyAPITime = now + (windyAPIIntervalMin * 60000UL);
    nextApiCallAllowedTime = now + API_BUFFER_MS;
    nextWindyAPITimeStr = printNextAPICallTime(nextWindyAPITime, "Windy API");
    return;
  }
}

// Helper to print the actual time/date for a future API call
String printNextAPICallTime(unsigned long futureMillis, const String &apiName)
{
  unsigned long nowMillis = millis();
  unsigned long deltaMillis = (futureMillis > nowMillis) ? (futureMillis - nowMillis) : 0;
  unsigned long futureEpoch = epochTime + (deltaMillis / 1000);

  tmElements_t futureTime;
  breakTime(futureEpoch + UTCOffset_sec, futureTime);
  
  // Format time and date components with leading zeros where needed
  String fHour = (futureTime.Hour < 10) ? "0" + String(futureTime.Hour) : String(futureTime.Hour);
  String fMin = (futureTime.Minute < 10) ? "0" + String(futureTime.Minute) : String(futureTime.Minute);
  String fSec = (futureTime.Second < 10) ? "0" + String(futureTime.Second) : String(futureTime.Second);
  String fDay = (futureTime.Day < 10) ? "0" + String(futureTime.Day) : String(futureTime.Day);
  String fMonth = (futureTime.Month < 10) ? "0" + String(futureTime.Month) : String(futureTime.Month);
  String fYear = String(futureTime.Year + epochYear).substring(2); // Get last two digits of the year
  
  // Construct the desired format: hh:mm:ss dd/mm/yy
  String dateTime = fHour + ":" + fMin + ":" + fSec + " " + fDay + "/" + fMonth + "/" + fYear;
  
  String msg = apiName + " queued at " + dateTime;
  serialTerminal(0, msg);
  return dateTime;
}

void handleAsyncWiFiScan(int n) {
    serialTerminal(5, "WiFi Scan done");
    if (n == 0) {
        serialTerminal(5, "No Available Networks");
    } else {
        serialTerminal(5, String(n) + " Networks found");
        for (int i = 0; i < n; ++i) {
            serialTerminal(5, String(i + 1) + ": " + WiFi.SSID(i) + " dB: " + WiFi.RSSI(i) + " Channel: " + WiFi.channel(i) + " Encryption: " + WiFi.encryptionType(i));
            // No delay needed here as this runs in the context of the WiFi event handler
        }
    }
    // After scan, we can proceed with the connection attempt.
    // The wifiMulti.run() in the main connection logic will now use these results.
    // We can also delete the scan results to free memory.
    WiFi.scanDelete();
}

void disableWiFi()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    initialBoot = false;
    WiFi.disconnect(true); // Disconnect from the network
    WiFi.mode(WIFI_OFF);   // Switch WiFi off
    serialTerminal(0, "WiFi disconnected");
  }
}

void disableBluetooth()
{
  btStop();
  serialTerminal(0, "Bluetooth disabled");
}

void setModemSleep()
{
  if (sleepSwitch == 1)
  {
    if ((wsStatus == false) && (loadVoltage < 3.45))
    {
      disableWiFi();
      serialTerminal(0, "Modem Sleep Enabled");
    }

    if ((wsStatus == false) && (loadVoltage >= 3.45))
    {
      enableWiFi();
      serialTerminal(0, "Staying awake - Battery > 3.45V");
    }

    if (wsStatus == true)
    {
      if (loadVoltage >= 3.45)
      {
        serialTerminal(0, "Staying awake - Battery > 3.45V");
      }
      else if (loadVoltage < 3.45)
      {
        serialTerminal(0, "Staying awake - Client connected");
      }
    }
  }

  if (sleepSwitch == 0)
  {
    serialTerminal(0, "Sleep mode disabled");
  }
}

void startConfigPortal() {
  serialTerminal(2, "WiFi connection failed. Starting Configuration Access Point.");
  
  // Stop the main web server and DNS if they are running
  ASyncServer.reset(); // Clear all existing routes to prevent conflicts
  dnsServer.stop();

  // Set device as a Wi-Fi access point
  WiFi.mode(WIFI_AP);
  WiFi.softAP("WeatherStation-Setup");
  IPAddress apIP = WiFi.softAPIP();
  serialTerminal(5, "AP IP address: " + apIP.toString());

  // Start DNS server for captive portal
  dnsServer.start(DNS_PORT, "*", apIP);

  // Define the HTML page for configuration
  String configPage = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
        <title>Weather Station WiFi Setup</title>
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <style>
            body { font-family: Arial, sans-serif; background-color: #f4f4f4; margin: 40px; }
            .container { background-color: #fff; padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); max-width: 400px; margin: auto; }
            h2 { color: #333; }
            input[type=text], input[type=password] { width: 100%; padding: 12px; margin: 8px 0; display: inline-block; border: 1px solid #ccc; border-radius: 4px; box-sizing: border-box; }
            button { background-color: #0d6efd; color: white; padding: 14px 20px; margin: 8px 0; border: none; border-radius: 4px; cursor: pointer; width: 100%; }
            button:hover { background-color: #0b5ed7; }
        </style>
    </head>
    <body>
        <div class="container">
            <h2>Weather Station WiFi Setup</h2>
            <p>Enter your WiFi credentials to connect the station to your network.</p>
            <form action="/save" method="POST">
                <label for="ssid">SSID</label>
                <input type="text" id="ssid" name="ssid" placeholder="Your WiFi network name">
                <label for="pass">Password</label>
                <input type="password" id="pass" name="pass" placeholder="Your WiFi password">
                <button type="submit">Save and Restart</button>
            </form>
        </div>
    </body>
    </html>
  )rawliteral";

  // Handle root URL
  ASyncServer.on("/", HTTP_GET, [configPage](AsyncWebServerRequest *request){
    request->send(200, "text/html", configPage);
  });

  // Handle form submission
  ASyncServer.on("/save", HTTP_POST, [](AsyncWebServerRequest *request){
    if (request->hasParam("ssid", true) && request->hasParam("pass", true)) {
      strlcpy(ssid, request->getParam("ssid", true)->value().c_str(), sizeof(ssid));
      strlcpy(password, request->getParam("pass", true)->value().c_str(), sizeof(password));
      serialTerminal(5, "New WiFi credentials received. Saving configuration.");
      saveConfig();
      request->send(200, "text/plain", "Credentials saved. The device will now restart.");
      delay(1000);
      systemRestart("Configuration updated via AP portal.");
    } else {
      request->send(400, "text/plain", "Bad Request. Both SSID and password are required.");
    }
  });

  ASyncServer.begin();
  serialTerminal(5, "Configuration portal is active.");

  unsigned long portalStartTime = millis();
  const unsigned long portalTimeout = 180000; // 3 minutes in milliseconds
  bool clientConnectedNotified = false; // Flag to prevent log spam

  while (millis() - portalStartTime < portalTimeout)
  {
    dnsServer.processNextRequest();
    
    int connectedStations = WiFi.softAPgetStationNum();

    if (connectedStations > 0 && !clientConnectedNotified)
    {
      // A client has just connected, and we haven't logged it yet.
      portalStartTime = millis();
      serialTerminal(0, "Client connected to AP. Timeout reset.");
      clientConnectedNotified = true; // Set the flag to prevent further messages
    } else if (connectedStations == 0 && clientConnectedNotified) {
      // The last client has disconnected.
      serialTerminal(0, "Client disconnected from AP.");
      clientConnectedNotified = false; // Reset the flag for the next connection
    }
    delay(10);
  }

  // If we exit the loop, it means the timeout was reached without credentials being saved.
  systemRestart("Config portal timed out. Restarting.");
}

void enableWiFi()
{
  if (WiFi.status() == WL_CONNECTED) {
    return; // Already connected, do nothing.
  }

  WiFi.disconnect(false); // Ensure we start fresh
  WiFi.mode(WIFI_STA);

  int retries = 0;
  const int maxRetries = 15; // Retry for up to 75 seconds (15 * 5s)

  while (WiFi.status() != WL_CONNECTED && retries < maxRetries) {
    retries++;
    serialTerminal(0, "WIFI Enabled. Attempting to connect to SSID: " + String(ssid) + " (Attempt " + String(retries) + "/" + String(maxRetries) + ")");
    
    // Start a new connection attempt on each loop iteration
    WiFi.begin(ssid, password);

    // Use the standard "In Progress" pattern
    LedBlinker(ledSwitch, 500, LEDCyan, 500, LEDBlack, 1); // Pulsing Cyan
    
    // Wait for the connection attempt to resolve, but with a shorter timeout for this single attempt.
    unsigned long attemptStartTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - attemptStartTime < 5000) {
        delay(100);
    }

    // If not connected after the attempt, disconnect fully before the next retry.
    if (WiFi.status() != WL_CONNECTED) {
        WiFi.disconnect(true); // true = also erase WiFi config from RAM
        delay(100); // Small delay to allow the stack to settle
    }
  }

  if (WiFi.status() == WL_CONNECTED) {
      rssi = WiFi.RSSI();
      IPaddress = String() + WiFi.localIP()[0] + "." + WiFi.localIP()[1] + "." + WiFi.localIP()[2] + "." + WiFi.localIP()[3];
      if (initialBoot == true)
      {
        serialTerminal(5, "SSID: " + String(ssid) + " IP: " + IPaddress + " RSSI: " + String(rssi));
      }
      // Use the standard "Success" pattern
      LedBlinker(ledSwitch, 100, LEDGreen, 100, LEDBlack, 2); // Quick Green Double-Flash
  } else {
      // Use the standard "Critical Failure" pattern
      LedBlinker(true, 100, LEDRed, 100, LEDBlack, 5); // Fast Red Flash
      // --- ADDED: Trigger Config Portal on Failure ---
      serialTerminal(3, "Failed to connect to WiFi. Entering configuration mode.");
      startConfigPortal();
      // The code will now loop inside the config portal until restart
      while(true) {
        dnsServer.processNextRequest();
        delay(10);
      }
    }
  }

void WiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
    switch (event) {
        case ARDUINO_EVENT_WIFI_SCAN_DONE:
            handleAsyncWiFiScan(info.wifi_scan_done.number);
            break; // This is the correct event for scan completion
        default:
            break;
    }
}

void monitorWiFiRSSI()
{
  if (WiFi.status() != WL_CONNECTED) return; // Don't check RSSI if not connected

  rssi = WiFi.RSSI();
  // Lower the threshold to be less sensitive to minor fluctuations. -90 is a more reasonable floor.
  if (rssi < -90.0) // Using a fixed, lower threshold instead of minWifiRSSI
  {
    String logMessage = "Critical low RSSI detected: " + String(rssi) + "dB. Forcing reconnect.";
    serialTerminal(2, logMessage);
    disableWiFi();
    delay(100);
    enableWiFi();
  } else {
    String logMessage = "WiFi RSSI check OK: " + String(rssi) + "dB";
    serialTerminal(0, logMessage);
  }
}
byte wifiAttempts = 0;
void wifiMonitor()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    String logMessage = "WiFi Disconnected!";
    serialTerminal(2, logMessage);
    disableWiFi();
    delay(100);
    enableWiFi();
  }
  else
  {
    String logMessage = "WiFi Connected";
    serialTerminal(0, logMessage);
  }
}
void wakeModemSleep()
{
  // setCpuFrequencyMhz(240);
  serialTerminal(0, "Modem Sleep Disabled - CPU @ 240Mhz");
  enableWiFi();
}

unsigned long webTimer = 0;
void updateWxWeb(unsigned long updateInterval)
{
  if (wsStatus == true)
  {
    if (millis() - webTimer >= updateInterval)
    {
      String payload;

      if ((validBMEReading == true) && (nanValue == false))
      {
        payload = wxJSONData("outsideTemp", String(outsideTemp));
        payload = wxJSONData("sensPress", String(sensPress));
        payload = wxJSONData("sensorHum", String(sensorHum));
      }
      payload = wxJSONData("tempMin", String(tempMin));
      payload = wxJSONData("tempMax", String(tempMax));
      payload = wxJSONData("humidityMin", String(humidityMin));
      payload = wxJSONData("humidityMax", String(humidityMax));
      payload = wxJSONData("pressureMin", String(pressureMin));
      payload = wxJSONData("pressureMax", String(pressureMax));
      payload = wxJSONData("windMin", String(windMin));
      payload = wxJSONData("windMax", String(windMax));
      payload = wxJSONData("windAvg", String(windSpdAvg60Sec));
      payload = wxJSONData("winddir", String(winddir));
      payload = wxJSONData("windDirCd", String(windDirCd));
      payload = wxJSONData("windspeedKmH", String(windspeedKmH));
      payload = wxJSONData("windgustKmH", String(windgustKmH));
      payload = wxJSONData("windGMin", String(windGMin));
      payload = wxJSONData("windGMax", String(windGMax));
      payload = wxJSONData("windGAvg", String(windGAvg));
      payload = wxJSONData("loadVoltage", String(loadVoltage));
      payload = wxJSONData("power_mW", String(power_mW));
      payload = wxJSONData("currentmA", String(current_mA));
      payload = wxJSONData("rssi", String(rssi));
      payload = wxJSONData("IPaddress", String(IPaddress));
      payload = wxJSONData("timeStamp", String(currTime));
      payload = wxJSONData("dataRefresh", String(wxDataUpload / 1000));
      payload = wxJSONData("rssilimit", String(minWifiRSSI));
      payload = wxJSONData("wifiinterval", String(wifiRSSIInterval / 1000));
      payload = wxJSONData("windRainCutoff", String(windRainCutoff));
      payload = wxJSONData("deepSleepTime", String(deepSleepTime));
      payload = wxJSONData("rainHourMM", String(rainHourMM));
      payload = wxJSONData("dailyrainMM", String(dailyrainMM));
      payload = wxJSONData("version", versionNR);
      payload = wxJSONData("firmware", versionNR);
      payload = wxJSONData("windyStatus", String(windyStatus));
      payload = wxJSONData("thingStatus", String(thingStatus));
      payload = wxJSONData("wundStatus", String(wundStatus));
      payload = wxJSONData("memoryLFS", String(freeBytesLFS) + "KB");
      payload = wxJSONData("freeHeapMem", String(freeMem) + "K");
      payload = wxJSONData("nextWindyAPITimeStr", String(nextWindyAPITimeStr));
      payload = wxJSONData("nextWundAPITimeStr", String(nextWundAPITimeStr));
      payload = wxJSONData("nextTSAPITimeStr", String(nextTSAPITimeStr));
      payload = wxJSONData("uptime", String(millis()));
  

      notifyClients(payload);

      webTimer = millis();
    }
  }
}

void updateThingSpeak()
{
  if ((thingSApiSwitch == 1))
  {
    serialTerminal(0, "Initiating TS Sensor Data Upload");
    if ((validBMEReading == true) && (nanValue == false))
    {
      HTTPClient thingAPI;
      thingAPI.setConnectTimeout(5000); // 5 second connection timeout
      thingAPI.setTimeout(10000);       // 10 second response timeout

      if (!thingAPI.begin("http://api.thingspeak.com/update?api_key=" + thingSpkAPIwR + "&field1=" + String(outsideTemp) + "&field2=" + String(sensPress) + "&field3=" + String(sensorHum) + "&field4=" + String(dewPoint) + "&field5=" + String(winddir) + "&field6=" + String(windSpdAvg60Sec) + "&field7=" + String(loadVoltage) + "&field8=" + String(dailyrainMM)))
      {
        serialTerminal(2, "Failed to connect to thingAPI");
        // Use the standard "Warning" pattern
        LedBlinker(ledSwitch, 500, LEDOrange, 500, LEDBlack, 3); // Slow Orange Pulse
        thingAPI.end();
      }
      else
      {

        int httpCode = thingAPI.GET();
        thingStatus = httpCode; // Store it for the UI
        thingAPI.end();
 
        if (httpCode == 200)
        {
          // Use the standard "Success" pattern
          LedBlinker(ledSwitch, 100, LEDGreen, 100, LEDBlack, 2); // Quick Green Double-Flash
          serialTerminal(0, "thingAPI [Updated] :: httpCode1: " + String(thingStatus));
        }
        else
        {
          serialTerminal(1, "Failed to connect to thingAPI - error code: " + String(thingStatus));
          // Use the standard "Warning" pattern
          LedBlinker(ledSwitch, 500, LEDOrange, 500, LEDBlack, 3); // Slow Orange Pulse
        }
      }
    }
    else
    {
      serialTerminal(2, "TS Upload skipped due to sensor failure");
    }
  }
  if (thingSApiSwitch == 0)
  {
    serialTerminal(0, "Thingspeak API Disabled");
  }
}
 
void updateWindyAPI()
{
  if (windyApiSwitch == 1)
  {
    serialTerminal(0, "Initiating Windy Sensor Data Upload");
    if ((bmeSensorDetected == true) && (validBMEReading == true) && (nanValue == false) && (windVaneStatus == true))
    {
      HTTPClient windyAPI;
      windyAPI.setConnectTimeout(10000); // 10 second connection timeout
      windyAPI.setTimeout(15000);       // 15 second response timeout
      String payload = "http://stations.windy.com/pws/update/" + windyAPIKey + "?temp=" + String(outsideTemp) + "&mbar=" + String(sensPress) + "&rh=" + String(sensorHum) + "&dewpoint=" + String(dewPoint);

      if (windgustKmH > 0)
      {
        payload = payload + "&gust=" + String(windgustKmH / 3.6);
      }
      if (windVaneStatus == true)
      {
        payload = payload + "&winddir=" + String(winddir) + "&wind=" + String(windSpdAvg60Sec / 3.6);
      }
      if (rainHourMM > 0)
      {
        payload = payload + "&precip=" + String(rainHourMM);
      }
      serialTerminal(0, payload);
      if (!windyAPI.begin(payload))
      {
        serialTerminal(2, "Failed to connect to Windy API");
        // Use the standard "Warning" pattern
        LedBlinker(ledSwitch, 500, LEDOrange, 500, LEDBlack, 3); // Slow Orange Pulse
        windyAPI.end();
      }
      else
      {
        int httpCode = windyAPI.GET();
        windyStatus = httpCode; // Store it for the UI
        windyAPI.end();

        if (httpCode == 200)
        {
          // Use the standard "Success" pattern
          LedBlinker(ledSwitch, 100, LEDGreen, 100, LEDBlack, 2); // Quick Green Double-Flash
          serialTerminal(0, "windyAPI [Updated] :: httpCode: " + String(windyStatus));
        }
        else
        {
          serialTerminal(1, "windyAPI [Failed] :: error code: " + String(windyStatus));
          // Use the standard "Warning" pattern
          LedBlinker(ledSwitch, 500, LEDOrange, 500, LEDBlack, 3); // Slow Orange Pulse
        }
      }
    }
    else
    {
      serialTerminal(2, "Windy upload skipped due to sensor failure");
    }
  }
  if (windyApiSwitch == 0)
  {
    serialTerminal(0, "Windy API Disabled");
  }
}

void updateWundAPI()
{
  if (wundApiSwitch == 1)
  {
    serialTerminal(0, "Initiating Wund Sensor Data Upload");
    if ((bmeSensorDetected == true) && (validBMEReading == true) && (nanValue == false))
    {
      HTTPClient wundAPI;
      wundAPI.setConnectTimeout(5000); // 5 second connection timeout
      wundAPI.setTimeout(10000);       // 10 second response timeout
      String payload = "http://rtupdate.wunderground.com/weatherstation/updateweatherstation.php?ID=" + wundStationID + "&PASSWORD=" + wundStationPw + "&tempf=" + String(tempF) + "&baromin=" + String((pressIn)) + "&humidity=" + String((sensorHum)) + "&dewptf=" + String(dewPF);

      if (windVaneStatus == true)
      {
        payload = payload + "&winddir=" + String(winddir) + "&windspeedmph=" + String(windSpeedMph);
      }
      if (windGustMph > 0)
      {
        payload = payload + "&windgustmph=" + String(windGustMph);
      }
      if (rainIn > 0)
      {
        payload = payload + "&rainin=" + String(rainIn);
      }

      if (dailyRainIn > 0)
      {
        payload = payload + "&dailyrainin=" + String(dailyRainIn);
      }
      payload = payload + "&dateutc=now&action=updateraw";

      serialTerminal(0, payload);
      if (!wundAPI.begin(payload))
      {
        // Use the standard "Warning" pattern
        LedBlinker(ledSwitch, 500, LEDOrange, 500, LEDBlack, 3); // Slow Orange Pulse
        serialTerminal(2, "Failed to connect to Wunderground API");
        wundAPI.end();
      }
      else
      {
        int httpCode = wundAPI.GET();
        wundStatus = httpCode; // Store it for the UI
        wundAPI.end();

        if (httpCode == 200)
        {
          // Use the standard "Success" pattern
          LedBlinker(ledSwitch, 100, LEDGreen, 100, LEDBlack, 2); // Quick Green Double-Flash
          serialTerminal(0, "wundAPI [Updated] :: httpCode: " + String(wundStatus));
        }
        else
        {
          serialTerminal(1, "wundAPI [Failed] :: error code: " + String(wundStatus));
          // Use the standard "Warning" pattern
          LedBlinker(ledSwitch, 500, LEDOrange, 500, LEDBlack, 3); // Slow Orange Pulse
        }
      }
    }
    else
    {
      serialTerminal(2, "Wund upload skipped due to sensor failure");
    }
  }
  if (wundApiSwitch == 0)
  {
    serialTerminal(0, "Wund API Disabled");
  }
}

/*
  void updatePWSWx()
  {
  if (pwsWXAPISwitch == 1)
  {
    serialTerminal(0, "Initiating PWSWeather Sensor Data Upload");
    if ((bmeSensorDetected == true) && (validBMEReading == true) && (nanValue == false))
    {
      HTTPClient pwsWXAPI;

      String payload = "https://www.pwsweather.com/pwsupdate/pwsupdate.php?ID=" + PWSWxStationID + "&PASSWORD=" + PWSWxStationPw + "&tempf=" + String(tempF) + "&baromin=" + String((pressIn)) + "&humidity=" + String((sensorHum)) + "&dewptf=" + String(dewPF);

      if (windVaneStatus == true)
      {
        payload = payload + "&winddir=" + String(winddir) + "&windspeedmph=" + String(windSpeedMph); // Note: windSpeedMph is not consistently updated, might be 0.
      }
      if (windGustMph > 0)
      {
        payload = payload + "&windgustmph=" + String(windGustMph);
      }
      if (rainIn > 0)
      {
        payload = payload + "&rainin=" + String(rainIn);
      }

      if (dailyRainIn > 0)
      {
        payload = payload + "&dailyrainin=" + String(dailyRainIn);
      }
      payload = payload + "&dateutc=now&action=updateraw";

      serialTerminal(0, payload);
      if (!pwsWXAPI.begin(payload))
      {
        LedBlinker(ledSwitch, 500, LEDRed, 500, LEDOrange, 5);
        serialTerminal(2, "Failed to connect to PWSWx API");
        pwsWXAPI.end();
      }
      else
      {
        PWSWxStatus = pwsWXAPI.GET();
        pwsWXAPI.end();

        if (PWSWxStatus == 200)
        {
          LedBlinker(ledSwitch, 200, LEDBlue, 500, LEDGreen, 4);
          serialTerminal(0, "PWSWx API [Updated] :: httpCode: " + String(PWSWxStatus));
        }
        else
        {
          serialTerminal(1, "PWSWx API [Failed] :: error code: " + String(PWSWxStatus));
          LedBlinker(ledSwitch, 500, LEDRed, 500, LEDOrange, 4);
        }
      }
    }
    else
    {
      serialTerminal(2, "PWSWx API upload skipped due to sensor failure");
    }
  }
  if (pwsWXAPISwitch == 0)
  {
    serialTerminal(0, "PWSWx API Disabled");
  }
  }

*/

void printWeather()
{
  if (wsStatus == true)
  {
    serialTerminal(0, "winddir=" + String(winddir) + " windspeedKmH=" + String(windspeedKmH) + " windgustKmH=" + String(windgustKmH) + " windgustdir=" + String(windgustdir) + " rainMinHour[" + String(currMin.toInt()) + "] " + rainMinHour[currMin.toInt()] + " rainHourMM=" + String(rainHourMM) + " dailyrainMM=" + String(dailyrainMM) + " Temperature: " + String(outsideTemp) + " Pressure: " + String(sensPress) + " Humidity: " + String(sensorHum) + " RSSI: " + String(rssi) + " loadVoltage: " + String(loadVoltage) + " current mA: " + String(current_mA));
  }
}

void detectBME280()
{

  if (bme.begin(0x76, &Wire))
  {
    serialTerminal(0, "0x76 BME280 sensor detected");
    bmeSensorDetected = true;
    bme.setSampling(Adafruit_BME280::MODE_FORCED,
                    Adafruit_BME280::SAMPLING_X1, // temperature
                    Adafruit_BME280::SAMPLING_X1, // pressure
                    Adafruit_BME280::SAMPLING_X1, // humidity
                    Adafruit_BME280::FILTER_OFF);
    serialTerminal(0, "forced mode, 1x temperature / 1x humidity / 1x pressure oversampling, ");
    serialTerminal(0, "filter off");
    bmeAdr = 0x76;
  }
  else if (bme.begin(0x77, &Wire))
  {
    serialTerminal(0, "0x77 BME280 sensor detected");
    bmeSensorDetected = true;
    bme.setSampling(Adafruit_BME280::MODE_FORCED,
                    Adafruit_BME280::SAMPLING_X1, // temperature
                    Adafruit_BME280::SAMPLING_X1, // pressure
                    Adafruit_BME280::SAMPLING_X1, // humidity
                    Adafruit_BME280::FILTER_OFF);
    serialTerminal(0, "forced mode, 1x temperature / 1x humidity / 1x pressure oversampling, ");
    serialTerminal(0, "filter off");
    bmeAdr = 0x77;
  }
  else
  {
    serialTerminal(3, "No BME280 sensor detected");
    bmeSensorDetected = false;
    validBMEReading = false;    
    // Use the standard "Warning" pattern
    LedBlinker(ledSwitch, 500, LEDOrange, 500, LEDBlack, 3); // Slow Orange Pulse
  }
}

unsigned long getTime()
{
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    timeAttempts++;
    serialTerminal(3, "Failed to obtain time - attempts: " + String(timeAttempts));
    // Use the standard "Critical Failure" pattern for repeated NTP failure
    LedBlinker(initialBoot, 100, LEDRed, 100, LEDBlack, 5); // Fast Red Flash

    if (timeAttempts > 5)
    {
      systemRestart("Time failed to sync");
    }
    return (0);
  }
  else
  {
    time(&now);
    return now;
  }
}

void syncTime(unsigned long syncTimer)
{
  int variance = 0;
  unsigned long tempTime = 0; // Use unsigned long to match getTime() return type
  if ((millis() - clockSyncTimer >= syncTimer) || (initialTimeSync == true))
  {
    // On initial boot, retry until success. For subsequent syncs, try only once.
    do {
        tempTime = getTime();
        if (tempTime > 0) {
            break; // Success! Exit the loop.
        }

        if (initialTimeSync) {
            serialTerminal(2, "Initial time sync failed. Retrying in 1 seconds...");
            delay(1000); // Wait 1 second before the next attempt.
        }

    } while (initialTimeSync && tempTime == 0);

    if (tempTime > 0)
    {
      variance = epochTime - tempTime;
      epochTime = tempTime;
      setCurrentTime();
      serialTerminal(0, "Time Sync [OK] - UNIX EpochTime: " + String(epochTime) + " Variance: " + String(variance) + " Current Time: " + currTime);
      // Use the standard "Success" pattern
      LedBlinker(ledSwitch, 100, LEDGreen, 100, LEDBlack, 2); // Quick Green Double-Flash
      timeSyncStatus = true;
    }
    else
    {
      serialTerminal(2, "Failed to obtain time (periodic check).");
      // Use the standard "Warning" pattern
      LedBlinker(ledSwitch, 500, LEDOrange, 500, LEDBlack, 3); // Slow Orange Pulse
      timeSyncStatus = false;
    }
    clockSyncTimer = millis();
  }
}

void setCurrentTime()
{

  if (millis() - clockTimer >= 1000)
  {
    clockTimer = millis();
    epochTime++;
    breakTime(epochTime + UTCOffset_sec, unixTimeStamp);

    currDay = String(unixTimeStamp.Day);

    weekDayIndex = unixTimeStamp.Wday - 1;  // Adjust for 0-based array
    currLWeekDay = Weekday[weekDayIndex];   // No change needed
    currSWeekDay = shWeekday[weekDayIndex]; // No change needed

    monthIndex = unixTimeStamp.Month;      // Month is 1-based from TimeLib
    currMonth = longMonth[monthIndex - 1]; // Adjust for 0-based array

    currYear = String(unixTimeStamp.Year + epochYear);

    currMin = (unixTimeStamp.Minute < 10) ? "0" + String(unixTimeStamp.Minute) : String(unixTimeStamp.Minute);
    currHour = (unixTimeStamp.Hour < 10) ? "0" + String(unixTimeStamp.Hour) : String(unixTimeStamp.Hour);
    currSec = (unixTimeStamp.Second < 10) ? "0" + String(unixTimeStamp.Second) : String(unixTimeStamp.Second);
    currTime = currHour + ":" + currMin + ":" + currSec;

    currSDate = currDay + "/" + monthIndex + "/" + currYear;

    currLDate = currLWeekDay + " " + currDay + " " + currMonth + " " + currYear;
    serialTerminal(1, "Date: " + currSDate + " " + currLDate + " time: " + currTime);
    timeStamp = currDay + "/" + monthIndex + "/" + currYear + " " + currTime;
  }
}

void systemRestart(String message)
{
  systemRestartStatus == true;
  // Use the standard "Critical Failure" pattern to indicate a restart
  LedBlinker(true, 100, LEDRed, 100, LEDBlack, 5); // Fast Red Flash
  serialTerminal(0, "RESTARTING DEVICE: " + message);
  ESP.restart();
}

void batteryMonitor()
{
  if (ina219Status == true)
  {
    if ((loadVoltage < 3.4) && (wsStatus == false))
    {
      ESP.deepSleep(deepSleepTime * 1e6);
      serialTerminal(3, "Battery Depleted " + String(loadVoltage) + "V - Going to Sleep for " + String(deepSleepTime));
    }
    else if ((loadVoltage >= 3.4) && (loadVoltage < 3.6))
    {
      serialTerminal(2, "Battery LOW " + String(loadVoltage) + "V - Charge soon");
    }
    else if (loadVoltage >= 3.6)
    {
      serialTerminal(0, "Battery Nominal " + String(loadVoltage) + "V");
    }
  }
  else if (ina219Status == false)
  {
    serialTerminal(3, "INA219 not detected - Battery check skipped");
  }
}

void batteryCalc()
{
  if (battCheckSwitch == 0)
  {
    serialTerminal(1, "Battery check off");
  }
  else if (battCheckSwitch == 1)
  {
    if (!ina219.begin())
    {
      serialTerminal(3, "Failed to find INA219 chip");
      ina219Status = false;
    }
    else
    {
      ina219Status = true;
      ina219.powerSave(false);
      shuntvoltage = ina219.getShuntVoltage_mV();
      busvoltage = ina219.getBusVoltage_V();
      current_mA = ina219.getCurrent_mA();

      power_mW = ina219.getPower_mW();
      loadVoltage = busvoltage + (shuntvoltage / 1000);
      ina219.powerSave(true);
      // energy = energy + (loadVoltage * current_mA / 36000);
      //  batMAH = batMAH + (current_mA / 36000);
    }
  }
}

void writeVarVals(const char *filename)
{
  if (initialBoot == false)
  {
    LittleFS.remove(filename);
    // Open file for writing
    File file = LittleFS.open(filename, FILE_WRITE);
    if (!file)
    {
      serialTerminal(5, "Failed to create file");
      return;
    }
    else
    {
      String payload;

      if ((validBMEReading == true) && (nanValue == false))
      {
        payload = wxVarValsJSON("outsideTemp", String(outsideTemp));
        payload = wxVarValsJSON("sensPress", String(sensPress));
        payload = wxVarValsJSON("sensorHum", String(sensorHum));
      }
      payload = wxVarValsJSON("tempMin", String(tempMin));
      payload = wxVarValsJSON("tempMax", String(tempMax));
      payload = wxVarValsJSON("humidityMin", String(humidityMin));
      payload = wxVarValsJSON("humidityMax", String(humidityMax));
      payload = wxVarValsJSON("pressureMin", String(pressureMin));
      payload = wxVarValsJSON("pressureMax", String(pressureMax));
      payload = wxVarValsJSON("windMax", String(windMax));
      payload = wxVarValsJSON("windAvg", String(windSpdAvg60Sec));
      payload = wxVarValsJSON("windGMin", String(windGMin));
      payload = wxVarValsJSON("windGMax", String(windGMax));
      payload = wxVarValsJSON("windGAvg", String(windGAvg));
      payload = wxVarValsJSON("loadVoltage", String(loadVoltage));
      payload = wxVarValsJSON("power_mW", String(power_mW));
      payload = wxVarValsJSON("currentmA", String(current_mA));
      payload = wxVarValsJSON("timeStamp", String(currTime));
      payload = wxVarValsJSON("dataRefresh", String(wxDataUpload));
      payload = wxVarValsJSON("rssilimit", String(minWifiRSSI));
      payload = wxVarValsJSON("wifiinterval", String(wifiRSSIInterval));
      payload = wxVarValsJSON("windRainCutoff", String(windRainCutoff));
      payload = wxVarValsJSON("savedRainHourMM", String(rainHourMM));
      payload = wxVarValsJSON("dailyrainMM", String(dailyrainMM));
      payload = wxVarValsJSON("version", versionNR);
      payload = wxVarValsJSON("firmware", versionNR);
      payload = wxVarValsJSON("windyStatus", String(windyStatus));
      payload = wxVarValsJSON("thingStatus", String(thingStatus));
      payload = wxVarValsJSON("wundStatus", String(wundStatus));
      payload = wxVarValsJSON("PWSWxStatus", String(PWSWxStatus));
      payload = wxVarValsJSON("memory", String(freeBytesLFS) + "KB");
      payload = wxVarValsJSON("darkModeSwitch", String(darkModeSwitch));
      payload = wxVarValsJSON("wifiMultiSwitch", String(wifiMultiSwitch));
      payload = wxVarValsJSON("errorLogSwitch", String(errorLogSwitch));
      payload = wxVarValsJSON("deepSleepTime", String(deepSleepTime));
      payload = wxVarValsJSON("sleepSwitch", String(sleepSwitch));
      payload = wxVarValsJSON("battCheckSwitch", String(battCheckSwitch));
      payload = wxVarValsJSON("windyApiSwitch", String(windyApiSwitch));
      payload = wxVarValsJSON("thingSApiSwitch", String(thingSApiSwitch));
      payload = wxVarValsJSON("wundApiSwitch", String(wundApiSwitch));
      payload = wxVarValsJSON("pwsWXAPISwitch", String(pwsWXAPISwitch));
      payload = wxVarValsJSON("bmeSwitch", String(bmeSwitch));
      payload = wxVarValsJSON("windDirSwitch", String(windDirSwitch));
      payload = wxVarValsJSON("windSpdSwitch", String(windSpdSwitch));
      payload = wxVarValsJSON("rainSwitch", String(rainSwitch));
      payload = wxVarValsJSON("ledSwitch", String(ledSwitch));

      file.println(payload);
      serialTerminal(0, "Variable JSON Saved: " + String(payload));
    }
    // Close the file
    file.close();
  }
}

bool varJsonRead;
void readVarVals(const char *filename)
{
  // Open file for reading
  if (LittleFS.exists(filename))
  {
    String jsonFile = readFile(LittleFS, filename);
    if (Serial)
    {
      Serial.println(("Reading Variable JSON Config"));
      Serial.println((jsonFile));
    }
    DynamicJsonDocument doc(1600);

    // Deserialize the JSON document
    DeserializationError error = deserializeJson(doc, jsonFile);
    switch (error.code())
    {
    case DeserializationError::Ok:
      Serial.println(("Variable JSON Deserialization succeeded"));
      varJsonRead = true;
      serialTerminal(5, "Variable JSON Deserialization succeeded varJsonRead: " + String(varJsonRead));
      break;
    case DeserializationError::InvalidInput:
      Serial.println(("Variable JSON Invalid input!"));
      varJsonRead = false;
      serialTerminal(5, "Variable JSON Invalid input! varJsonRead: " + String(varJsonRead));
      break;
    case DeserializationError::NoMemory:
      Serial.println(("Variable JSON Not enough memory"));
      varJsonRead = false;
      serialTerminal(5, "Variable JSON Not enough memory varJsonRead: " + String(varJsonRead));
      break;
    default:
      Serial.println(("Variable JSON Deserialization failed"));
      serialTerminal(5, "Variable JSON Deserialization failed");
      varJsonRead = false;
      break;
    }
    if (varJsonRead == true)
    {
      tempMin = doc["tempMin"].as<float>();
      tempMax = doc["tempMax"].as<float>();
      humidityMin = doc["humidityMin"].as<float>();
      humidityMax = doc["humidityMax"].as<float>();
      pressureMin = doc["pressureMin"].as<float>();
      pressureMax = doc["pressureMax"].as<float>();
      windMax = doc["windMax"].as<float>();
      windAvg = doc["windAvg"].as<float>();
      windGMin = doc["windGMin"].as<float>();
      windGMax = doc["windGMax"].as<float>();
      windGAvg = doc["windGAvg"].as<float>();
      wxDataUpload = doc["dataRefresh"].as<int>();
      wifiRSSIInterval = doc["wifiinterval"].as<int>();
      windRainCutoff = doc["windRainCutoff"].as<int>();
      savedRainHourMM = doc["savedRainHourMM"].as<float>();
      dailyrainMM = doc["dailyrainMM"].as<float>();
      darkModeSwitch = doc["darkModeSwitch"].as<int>();
      wifiMultiSwitch = doc["wifiMultiSwitch"].as<int>();
      errorLogSwitch = doc["errorLogSwitch"].as<int>();
      deepSleepTime = doc["deepSleepTime"].as<int>();
      sleepSwitch = doc["sleepSwitch"].as<int>();
      battCheckSwitch = doc["battCheckSwitch"].as<int>();

      windyApiSwitch = doc["windyApiSwitch"].as<int>();
      thingSApiSwitch = doc["thingSApiSwitch"].as<int>();
      wundApiSwitch = doc["wundApiSwitch"].as<int>();
      pwsWXAPISwitch = doc["pwsWXAPISwitch"].as<int>();
      bmeSwitch = doc["bmeSwitch"].as<int>();
      windDirSwitch = doc["windDirSwitch"].as<int>();
      windSpdSwitch = doc["windSpdSwitch"].as<int>();
      rainSwitch = doc["rainSwitch"].as<int>();
      ledSwitch = doc["ledSwitch"].as<int>();

      serialTerminal(5, "tempMin: " + String(tempMin) + " tempMax: " + String(tempMax) + " humidityMin: " + String(humidityMin) + " humidityMax: " + String(humidityMax) + " pressureMin: " + String(pressureMin) + " pressureMax: " + String(pressureMax) + " windMax: " + String(windMax) + " windAvg: " + String(windAvg) + " windGMin: " + String(windGMin) + " windGMax: " + String(windGMax) + " windGAvg: " + String(windGAvg) + " dataRefresh: " + String(wxDataUpload) + " wifiinterval: " + String(wifiRSSIInterval) + " windRainCutoff: " + String(windRainCutoff) + " savedRainHourMM: " + String(savedRainHourMM) + " dailyrainMM: " + String(dailyrainMM) + " darkModeSwitch: " + String(darkModeSwitch) + " wifiMultiSwitch: " + String(wifiMultiSwitch) + " errorLogSwitch: " + String(errorLogSwitch) + " deepSleepTime: " + String(deepSleepTime) + " sleepSwitch: " + String(sleepSwitch) + " battCheckSwitch: " + String(battCheckSwitch) + " windyApiSwitch: " + String(windyApiSwitch) + " thingSApiSwitch: " + String(thingSApiSwitch) + " wundApiSwitch: " + String(wundApiSwitch) + " pwsWXAPISwitch: " + String(pwsWXAPISwitch) + " bmeSwitch: " + String(bmeSwitch) + " windDirSwitch: " + String(windDirSwitch) + " windSpdSwitch: " + String(windSpdSwitch) + " rainSwitch: " + String(rainSwitch) + " ledSwitch: " + String(ledSwitch));
    }
  }
}

// Helper function to send a DynamicJsonDocument as a WebSocket message
void sendJsonDocumentToClients(DynamicJsonDocument &doc, const char *logMessageSuffix)
{
  if (!wsStatus)
    return;

  size_t jsonSize = measureJson(doc);
  // Add 1 for null terminator, though serializeJson might not strictly need it if length is passed.
  char *jsonBuffer = new (std::nothrow) char[jsonSize + 1];

  if (jsonBuffer)
  {
    serializeJson(doc, jsonBuffer, jsonSize + 1);
    ws.textAll(jsonBuffer, jsonSize); // Send using char* and length
    yield();                          // Give async_tcp a chance to process the sent data immediately
    delete[] jsonBuffer;
    serialTerminal(0, String("Sent ") + logMessageSuffix + ". Size: " + String(jsonSize));
  }
  else
  {
    // This is a critical error, could indicate heap exhaustion.
    serialTerminal(3, String("Failed to allocate buffer for ") + logMessageSuffix + " JSON");
  }
  doc.clear(); // Important to free memory used by the document
}

void sendLatestMinuteDataToClients()
{
  if (!wsStatus)
  {
    return;
  }
  serialTerminal(0, "Sending latest minute data to clients...");

  // Capacity for a small object with 7 members. ArduinoJson Assistant can help refine this.
  const int LATEST_DOC_CAPACITY = JSON_OBJECT_SIZE(7) + 256; // 256 for key strings and values
  DynamicJsonDocument latestDataDoc(LATEST_DOC_CAPACITY);

  // The history_data_index now points to the OLDEST data slot.
  // The newest data (the minute that just completed) is at the slot *before* this one, circularly.
  int latestDataSlot = (history_data_index - 1 + HISTORICAL_DATA_POINTS) % HISTORICAL_DATA_POINTS;

  if (temp_history[latestDataSlot] == -999.0f)
  {
    latestDataDoc["latestTemp"] = nullptr;
  }
  else
  {
    latestDataDoc["latestTemp"] = temp_history[latestDataSlot];
  }
  if (humidity_history[latestDataSlot] == -999.0f)
  {
    latestDataDoc["latestHum"] = nullptr;
  }
  else
  {
    latestDataDoc["latestHum"] = humidity_history[latestDataSlot];
  }
  if (pressure_history[latestDataSlot] == -999.0f)
  {
    latestDataDoc["latestPres"] = nullptr;
  }
  else
  {
    latestDataDoc["latestPres"] = pressure_history[latestDataSlot];
  }
  latestDataDoc["latestRain"] = rain_history[latestDataSlot]; // Rain is 0.0f if not -999
  if (windspeed_history[latestDataSlot] == -999.0f)
  {
    latestDataDoc["latestWind"] = nullptr;
  }
  else
  {
    latestDataDoc["latestWind"] = windspeed_history[latestDataSlot];
  }

  latestDataDoc["historyDataType"] = "latestMinuteUpdate";
  latestDataDoc["latestDataIndex"] = latestDataSlot; // Index of this new data point

  sendJsonDocumentToClients(latestDataDoc, "Latest Minute Data");
}

void sendHistoricalDataToClients()
{
  if (!wsStatus)
  { // If no WebSocket client is connected, do nothing
    serialTerminal(1, "No WebSocket clients connected, skipping historical data send.");
    return;
  }

  serialTerminal(0, "Preparing to send 24-hour historical data (10-min averages) to clients...");

  const int NUM_AVERAGES_TO_SEND = 144; // 24 hours * 6 (10-min blocks per hour)
  const int MINUTES_PER_AVERAGE = 10;
  
  // Capacity for: 3 top-level members + 6 arrays of 144 elements each.
  // JSON_OBJECT_SIZE(3) + 6 * JSON_ARRAY_SIZE(144)
  // Plus memory for 5*144 floats and string keys.
  // ArduinoJson assistant suggests around 8-9KB. Let's use 9KB.
  const int HIST_10MIN_AVG_DOC_CAPACITY = 9216; // 9KB
  DynamicJsonDocument historyDoc(HIST_10MIN_AVG_DOC_CAPACITY);

  JsonArray temp_avg_array = historyDoc.createNestedArray("tempData");
  JsonArray hum_avg_array = historyDoc.createNestedArray("humData");
  JsonArray pres_avg_array = historyDoc.createNestedArray("presData");
  JsonArray rain_sum_array = historyDoc.createNestedArray("rainData"); // Rain will be summed for the 10-min period
  JsonArray wind_avg_array = historyDoc.createNestedArray("windData");
  JsonArray batt_volt_avg_array = historyDoc.createNestedArray("battVoltData");

  for (int avg_block_idx = 0; avg_block_idx < NUM_AVERAGES_TO_SEND; ++avg_block_idx)
  {
    float temp_sum = 0.0, hum_sum = 0.0, pres_sum = 0.0, rain_total_for_block = 0.0, wind_sum = 0.0, batt_volt_sum = 0.0;
    int temp_valid_samples = 0, hum_valid_samples = 0, pres_valid_samples = 0, wind_valid_samples = 0, batt_volt_valid_samples = 0;
    // Rain doesn't need a valid_samples count as 0.0 is a valid sum.

    // history_data_index points to the OLDEST 1-minute data point.
    // We iterate through the 144 ten-minute blocks chronologically.
    int first_minute_slot_in_block = (history_data_index + avg_block_idx * MINUTES_PER_AVERAGE) % HISTORICAL_DATA_POINTS;

    for (int k = 0; k < MINUTES_PER_AVERAGE; ++k)
    {
      int current_minute_data_slot = (first_minute_slot_in_block + k) % HISTORICAL_DATA_POINTS;

      if (temp_history[current_minute_data_slot] != -999.0f)
      {
        temp_sum += temp_history[current_minute_data_slot];
        temp_valid_samples++;
      }
      if (humidity_history[current_minute_data_slot] != -999.0f)
      {
        hum_sum += humidity_history[current_minute_data_slot];
        hum_valid_samples++;
      }
      if (pressure_history[current_minute_data_slot] != -999.0f)
      {
        pres_sum += pressure_history[current_minute_data_slot];
        pres_valid_samples++;
      }
      // Rain is cumulative for the 10-minute block
      rain_total_for_block += rain_history[current_minute_data_slot]; // rain_history stores per-minute rain

      if (windspeed_history[current_minute_data_slot] != -999.0f)
      {
        wind_sum += windspeed_history[current_minute_data_slot];
        wind_valid_samples++;
      }
      if (batt_volt_history[current_minute_data_slot] != -999.0f)
      {
        batt_volt_sum += batt_volt_history[current_minute_data_slot];
        batt_volt_valid_samples++;
      }
    }

    if (temp_valid_samples > 0)
      temp_avg_array.add(temp_sum / temp_valid_samples);
    else
      temp_avg_array.add(nullptr);

    if (hum_valid_samples > 0)
      hum_avg_array.add(hum_sum / hum_valid_samples);
    else
      hum_avg_array.add(nullptr);

    if (pres_valid_samples > 0)
      pres_avg_array.add(pres_sum / pres_valid_samples);
    else
      pres_avg_array.add(nullptr);

    rain_sum_array.add(rain_total_for_block); // Add the sum for the 10-minute block

    if (wind_valid_samples > 0)
      wind_avg_array.add(wind_sum / wind_valid_samples);
    else
      wind_avg_array.add(nullptr);

    if (batt_volt_valid_samples > 0)
      batt_volt_avg_array.add(batt_volt_sum / batt_volt_valid_samples);
    else
      batt_volt_avg_array.add(nullptr);
  }

  historyDoc["historyDataType"] = "hist24h_10minAvgData";
  // historyStartIndex still refers to the absolute starting index of the oldest 1-minute
  // data in the ESP32's 1440-minute buffer. The client can use this to align
  // the start of its 24-hour, 10-minute-averaged display.
  historyDoc["historyStartIndex"] = history_data_index;

  // --- Wait for WebSocket to be ready before sending this chunk ---
  unsigned long waitStartTime = millis();
  bool canSendThisChunk = true;
  while (!ws.availableForWriteAll() && ws.count() > 0)
  {
    if (millis() - waitStartTime > 5000)
    { // Timeout after 5 seconds for this larger single chunk
      serialTerminal(2, "Timeout waiting for WebSocket to be writable for 10-min avg history. Skipping send.");
      canSendThisChunk = false;
      break;
    }
    serialTerminal(1, "WebSocket not ready for 10-min avg history, waiting 200ms...");
    delay(200);
    yield();
  }

  if (ws.count() == 0)
  {
    serialTerminal(2, "All clients disconnected while preparing 10-min avg historical data. Aborting.");
    return;
  }
  // --- End of WebSocket ready wait ---

  if (canSendThisChunk)
  {
    sendJsonDocumentToClients(historyDoc, "24h History (10-min Avg)");
  }
  else
  {
    historyDoc.clear(); // Still clear the doc if not sent
  }
  delay(100);
  yield();
}

unsigned long windTimer = 500;
unsigned long prevWindTimer = 0;
unsigned long writeValsTimer = 0;

void loop()
{
#ifndef SIMULATE_SENSORS
  wspeedIRQ();
  rainIRQ();
#endif

  if (millis() - prevWindTimer >= windTimer)
  {
    prevWindTimer = millis();
    currentSpeed = get_wind_speed();

    windspeedKmH = currentSpeed;
    windSpeedMph = windSpdAvg60Sec / 1.60934; // Note: windSpdAvg60Sec is updated once a minute in calcWeather
  }

  calcWeather(); // This now handles 1-second accumulations and 1-minute averaging/storage
  sendSensorData();
  getMemory();

  setCurrentTime();
  syncTime(30000);

  updateWxWeb(wxDataUpload);

  if (millis() - wsTimer >= 2000)
  {
    wsTimer = millis();
    ws.cleanupClients();
  }

  if (millis() - variableTimer >= 60000)
  {
    variableTimer = millis();
    monitorWiFiRSSI();
    batteryMonitor();
  }

  if (millis() - writeValsTimer >= 600000)
  {
    writeValsTimer = millis();
    writeVarVals(varFilename);
  }

  if (millis() - wifiRSSiTimer >= wifiRSSIInterval)
  {

    wifiMonitor();
    wifiRSSiTimer = millis();
  }
  ArduinoOTA.handle();
}
