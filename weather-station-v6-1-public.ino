
/**
   TODO
   > add first boot config for API
   > add changelog to setup page
   > Move WIFI setup to Websockets
*/

// VERSION NUMBER - UPDATE ON SAVE //
const char *versionName = "V6.1.A Weather Station";
String versionNR = "6.1.A";
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
#include <ThingSpeak.h>
#include <TimeLib.h>
#include <ArduinoOTA.h>
#include <SPI.h>
#include <Arduino_JSON.h>

char ssid[] = "YOUR_SSID";
char password[] = "PASSWORD";



// LIVE CREDENTIALS //
// THINGSPEAK SETUP - Sign up here https://thingspeak.com/login?skipSSOCheck=true
// unsigned long myChannelNumber = 1657561; // Uncomment and replace with your channel ID
String thingSpkAPIwR = ""; //Read API Key
String thingSpkAPIr = ""; //Write API Key
byte thingStatus;
HTTPClient thingAPI;

// Windy API Setup - Sign Up Here - https://stations.windy.com/
String stationID = "";
String windyAPIKey = "";
byte windyStatus;

// Wunderground API Setup - Sign Up Here - https://www.wunderground.com/signup
String wundStationID = "";
String wundStationPw = "";
byte wundStatus;

// Station sensor pin assignment definitions - Below pin assignments must be updated to match your GPIO
#define WIND_SPD_PIN 25
#define WIND_DIR_PIN 33
#define RAIN_PIN 32

// web server variables
TaskHandle_t Task1; // Task for webserver core
AsyncWebServer ASyncServer(80);
AsyncWebSocket ws("/ws");

TaskHandle_t APITask;

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
bool bmeSwitch = 1;
bool windDirSwitch = 1;
bool windSpdSwitch = 1;
bool rainSwitch = 1;
bool darkModeSwitch = 0;
bool wifiMultiSwitch = 0;
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
float realFeel;
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
bool initalBoot = true;
bool timeSyncStatus = false;
bool wifiStatus = false;
bool systemRestartStatus = false;
bool nanValue = false;
unsigned long lastSecond; // The millis counter to see when a second rolls by
unsigned long sensorSendTimer = 0;

// Wind Variables.
String windDir[17] = {"N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE", "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW", "N"};
String windDirCd;
bool windStateChange = 0;
bool lastwindStateChange = 0;
unsigned long timeSinceLastTick = 0;
unsigned long lastTick = 0;
float windSpeed = 0.0, windMin, windMax, windAvg, windGMin, windGMax, windGAvg;
unsigned long lastWindIRQ = 0;
int winddir = 0;
bool windVaneStatus = false;
float currentSpeed = 0;
float windspeedKmH = 0;
float windSpeedMph = 0;
float windgustKmH = 0;
float windGustMph = 0;
int windgustdir = 0;
float windspdKmH_avg2m = 0;
int winddir_avg2m = 0;

// variables used in calculating rainfall
bool rainStateChange = 0;
bool lastrainStateChange = 0;
bool rainFirstRead = true;
float rainHourMM = 0; //accumulated rainfall in the past 60 min
float savedRainHourMM;
float rainIn = 0;
float dailyrainMM = 0; //accumulated rainfall in the past day
float dailyRainIn = 0;
float rainPHMin, rainPHMax, rainPHMAvg; // TODO still to add to dashboard
unsigned long raintime, rainlast, raininterval;
int windRainCutoff = 35; // To prevent false rain readings due to mast oscilation, wind speed in km/h above which rain measurements will be skipped
float windSpd3Sec, windSpdAvg3Sec, windSpd60Sec, windSpdAvg60Sec;
float windSpdAvg1min[60];
unsigned long rainTimer = 25;
unsigned long prevRainTimer;
double rainMMStep = 0.3550523091; // volume in mm per bucket tip. Please calibrate your rain tip bucket!
float rainMinHour[60];             // 60 floating numbers to keep track of 60 minutes of rain

// WIFI
WiFiMulti wifiMulti;
WiFiClient client;
float rssi;
int wifiRSSIInterval = 600000; //WiFi scanning interval milliseconds - useful for multi-AP networks
double minWifiRSSI = -86; //dB cutoff for rescanning WiFi
String IPaddress;

// NTP Time Setup
const char *ntpServer = "pool.ntp.org";
unsigned long epochTime;
int epochYear = 1970;
long yourUTC = 2; // Replace with your actual UTC time zone value in hours
long UTCOffset_sec = yourUTC * 60 * 60;
const int daylightOffset_sec = 0;
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

// Date variables
String currMin, currHour, currSec, currTime, currSWeekDay, currLWeekDay, currDay, currMonth, currYear, currSDate, currLDate, timeStamp;
const String Weekday[7] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
const String shWeekday[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
const String longMonth[12] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};

// LittleFS variables
int tBytesLFS, uBytesLFS, freeBytesLFS;

// memory variablse
int freeMem;
unsigned long memTimer = 0;

// SD variables
const int chipSelectSD = 27;
int tBytesSD, uBytesSD, freeBytesSD;
bool initSD = false;

void setup()
{
  Serial.begin(115200);

  xTaskCreate(webServerCode, "webservercode", 10000, NULL, 5, &Task1);

  delay(250);

  const uint8_t protocol = WIFI_PROTOCOL_LR;
  esp_wifi_set_protocol(WIFI_IF_STA, protocol);

  Wire.begin();

  // Rain sensor pin
  pinMode(RAIN_PIN, INPUT_PULLUP);

  // Wind anemometer and direction sensor pins
  gpio_pad_select_gpio(GPIO_NUM_25);
  gpio_set_direction(GPIO_NUM_25, GPIO_MODE_INPUT);
  gpio_pullup_en(GPIO_NUM_25);

  LittleFS.begin();
  if (!LittleFS.begin())
  {
    serialTerminal(3, "An Error has occurred while mounting LittleFS");
  }
  else
  {
    serialTerminal(4, "LittleFS mounted");
    calcLFSMemory();
  }

  delay(500);
  readVarVals(varFilename);
  delay(500);

  Serial.print("\r\nWaiting for SD card to initialise...");
  if (!SD.begin(chipSelectSD))
  { // CS is D8 in this example
    Serial.println("Initialising failed!");
    initSD = false;
  }
  else
  {
    serialTerminal(4, "Initialisation completed");
    initSD = true;
  }

  batteryCalc();
  batteryMonitor();
  ledSetup();

  LedBlinker(true, 100, LEDBlue, 250, LEDOrange, 2);

  leds[0] = CRGB::Black;
  FastLED.show();

  detectBME280();

  btStop();
  enableWiFi();

  configTime(UTCOffset_sec, daylightOffset_sec, ntpServer);
  epochTime = getTime();
  delay(250);
  initialTimeSync = true;
  syncTime(0);

  delay(250);
  serialTerminal(4, "Boot Initial Time Sync");
  initialTimeSync = false;

  // OTA Setup
  ArduinoOTA.setHostname(versionName);
  ArduinoOTA.begin();

  ASyncServer.begin();
  initWebSocket();
  delay(500);
  LedBlinker(true, 250, LEDGreen, 100, LEDBlack, 3);
  initalBoot = false;
}

void getMemory()
{
  if (millis() - memTimer > 5000)
  {
    memTimer = millis();
    freeMem = ESP.getFreeHeap();
    serialTerminal(0, "Free Heap: " + String(freeMem) + "K");
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
      logMessage = currTime + " " + "[INFO] " + value;
      terminalWebSocket(logMessage);
      if (Serial)
        Serial.println((" [INFO] " + value));

      break;
    case 1:
      if (debugSwitch == 1)
      {
        logMessage = "[DEBUG] " + value;
        logErrors(logMessage);
        terminalWebSocket(currTime + " " + logMessage);
        if (Serial)
          Serial.println((" [DEBUG] " + value));
      }
      break;
    case 2:
      logMessage = "[WARNING]," + value;
      logErrors(logMessage);
      terminalMsg = "[WARNING] " + value;
      terminalWebSocket(currTime + " " + terminalMsg);
      if (Serial)
        Serial.println((" [WARNING] " + value));

      break;
    case 3:
      logMessage = "[CRITICAL]," + value;
      logErrors(logMessage);
      terminalMsg = "[CRITICAL] " + value;
      terminalWebSocket(currTime + " " + terminalMsg);
      if (Serial)
        Serial.println((" [CRITICAL] " + value));

      break;
    case 4:
      logMessage = "[BOOT]," + value;
      logErrors(logMessage);
      terminalMsg = "[BOOT] " + value;
      terminalWebSocket(currTime + " " + logMessage);
      if (Serial)
        Serial.println((" [BOOT] " + value));

      break;
    case 5:
      logMessage = "[STATUS]," + value;
      logErrors(logMessage);
      terminalMsg = "[STATUS] " + value;
      terminalWebSocket(currTime + " " + terminalMsg);
      if (Serial)
        Serial.println((" [STATUS] " + value));

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

void calcSDMemory()
{
  tBytesSD = SD.totalBytes() / 1048576;
  uBytesSD = SD.usedBytes() / 1048576;
  freeBytesSD = tBytesSD - uBytesSD;
  serialTerminal(1, "SD Free:" + String(freeBytesSD) + "MB | Total:" + String(tBytesSD) + "MB | Used:" + String(uBytesSD) + "MB");
}

void logErrors(String errorMessage)
{

  if ((errorLogSwitch == 1) && (initSD == true))
  {
    calcSDMemory();
    logFile = SD.open("/systemLog.csv", FILE_APPEND);
    int fileSize1 = logFile.size();
    serialTerminal(1, "logFile size: " + String(logFile.size()));

    if ((fileSize1 > 3000000) || (freeBytesSD > 0 && freeBytesSD < 5000))
    {
      clearLogFile();
      calcSDMemory();
    }
    else if (logFile)
    {

      errorMessage = currSDate + " " + currTime + "," + errorMessage;

      logFile.println(errorMessage);

      logFile.close(); // close the file
      calcSDMemory();
      serialTerminal(0, "Log data written to SD at " + String(currTime));
    }
  }
  if (!initSD)
  {
    serialTerminal(0, "SD init failed " + String(currTime));
  }
}

void clearLogFile()
{
  SD.remove("/systemLog.csv");
  serialTerminal(0, "logFile deleted");
  logFile = SD.open("/systemLog.csv", FILE_APPEND);
  logFile.println("Date,Level,Error Message");
  logFile.close();
  calcSDMemory();
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
      serialTerminal(5, "WebSocket client " + String(client->id()) + " connected from " + client->remoteIP().toString().c_str());

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

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
  {
    data[len] = 0;
    message = (char *)data;
    serialTerminal(0, "ws message:" + message);
    int msgChar1 = message.substring(0, 1).toInt();
    String msgChar2 = message.substring(1, 2);
    int msgChar3 = message.substring(2, 3).toInt();

    String msgChar3Hour, msgChar3min;
    serialTerminal(0, "message char: " + String(msgChar1) + msgChar2 + String(msgChar3));

    if ((msgChar1 >= 0) && (msgChar2 == "D"))
    {
      serialTerminal(0, "Toggle state:" + String(message));
      if (msgChar1 == 0)
      {
        uploadSwitch = msgChar3;
        serialTerminal(0, "Data upload:" + String(uploadSwitch));
      }
      if (msgChar1 == 1)
      {
        debugSwitch = msgChar3;
        serialTerminal(0, "debugSwitch:" + String(debugSwitch));
      }
      if (msgChar1 == 2)
      {
        errorLogSwitch = msgChar3;
        serialTerminal(0, "Data logging:" + String(errorLogSwitch));
      }
      if (msgChar1 == 3)
      {
        battCheckSwitch = msgChar3;
        serialTerminal(0, "Battery Check:" + String(battCheckSwitch));
      }
      if (msgChar1 == 4)
      {
        sleepSwitch = msgChar3;
        serialTerminal(0, "Sleep Mode:" + String(sleepSwitch));
      }
      if (msgChar1 == 5)
      {
        darkModeSwitch = msgChar3;
        serialTerminal(0, "darkMode:" + String(darkModeSwitch));
      }
      if (msgChar1 == 6)
      {
        wifiMultiSwitch = msgChar3;
        serialTerminal(0, "wifiMultiSwitch:" + String(wifiMultiSwitch));
      }
      if (msgChar1 == 7)
      {
        ledSwitch = msgChar3;
        serialTerminal(0, "ledSwitch:" + String(ledSwitch));
      }
      notifyClients(wxJSONData(msgChar2 + "toggle" + msgChar1, String(msgChar3)));
    }

    if ((msgChar1 >= 0) && (msgChar2 == "T"))
    {
      serialTerminal(0, "Toggle state:" + String(message));
      if (msgChar1 == 0)
      {
        terminalSwitch = msgChar3;
        serialTerminal(0, "terminalSwitch:" + String(terminalSwitch));
      }

      notifyClients(wxJSONData(msgChar2 + "toggle" + msgChar1, String(msgChar3)));
    }

    if ((msgChar1 >= 0) && (msgChar2 == "A"))
    {
      serialTerminal(0, "Toggle state:" + String(message));
      if (msgChar1 == 0)
      {
        thingSApiSwitch = msgChar3;
        serialTerminal(0, "thingSApiSwitch:" + String(thingSApiSwitch));
      }
      if (msgChar1 == 1)
      {
        windyApiSwitch = msgChar3;
        serialTerminal(0, "windyApiSwitch:" + String(windyApiSwitch));
      }
      if (msgChar1 == 2)
      {
        wundApiSwitch = msgChar3;
        serialTerminal(0, "wundApiSwitch:" + String(wundApiSwitch));
      }

      notifyClients(wxJSONData(msgChar2 + "toggle" + msgChar1, String(msgChar3)));
    }

    if ((msgChar1 >= 0) && (msgChar2 == "S"))
    {
      serialTerminal(0, "Toggle state:" + String(message));
      if (msgChar1 == 1)
      {
        bmeSwitch = msgChar3;
        serialTerminal(0, "bmeSwitch:" + String(bmeSwitch));
      }
      if (msgChar1 == 2)
      {
        windSpdSwitch = msgChar3;
        serialTerminal(0, "windSpdSwitch:" + String(windSpdSwitch));
      }
      if (msgChar1 == 3)
      {
        windDirSwitch = msgChar3;
        serialTerminal(0, "windDirSwitch:" + String(windDirSwitch));
      }
      if (msgChar1 == 4)
      {
        rainSwitch = msgChar3;
        serialTerminal(0, "rainSwitch:" + String(rainSwitch));
      }

      notifyClients(wxJSONData(msgChar2 + "toggle" + msgChar1, String(msgChar3)));
    }

    if ((msgChar1 >= 0) && (msgChar2 == "F"))
    {
      if (msgChar1 == 0)
      {
        int msgLength = message.length();
        float tempVal = (message.substring(2, msgLength).toFloat()) * 1000;

        if (wxDataUpload == 0)
        {
          wxDataUpload = 1000;
        }
        else
        {
          wxDataUpload = tempVal;
        }
        serialTerminal(0, "form message: " + message + " " + "msgLength: " + String(msgLength) + " wxDataUpload: " + String(wxDataUpload));
      }

      if (msgChar1 == 2)
      {
        int msgLength = message.length();
        minWifiRSSI = (message.substring(2, msgLength).toDouble());
        serialTerminal(0, "form message: " + message + " " + "msgLength: " + String(msgLength) + " minWifiRSSI: " + String(minWifiRSSI));
      }
      if (msgChar1 == 3)
      {
        int msgLength = message.length();
        wifiRSSIInterval = (message.substring(2, msgLength).toFloat()) * 1000;
        serialTerminal(0, "form message: " + message + " " + "msgLength: " + String(msgLength) + " wifiRSSIInterval: " + String(wifiRSSIInterval));
      }
      if (msgChar1 == 4)
      {
        int msgLength = message.length();
        windRainCutoff = (message.substring(2, msgLength).toInt());
        serialTerminal(0, "form message: " + message + " " + "msgLength: " + String(msgLength) + " windRainCutoff: " + String(windRainCutoff));
      }
      if (msgChar1 == 5)
      {
        int msgLength = message.length();
        deepSleepTime = (message.substring(2, msgLength).toInt());
        serialTerminal(0, "form message: " + message + " " + "msgLength: " + String(msgLength) + " deepSleepTime: " + String(deepSleepTime));
      }
    }
  }
  if (strcmp((char *)data, "getValues") == 0)
  {
    String payload = pageJSONData("Atoggle0", String(thingSApiSwitch));
    payload = pageJSONData("Atoggle1", String(windyApiSwitch));
    payload = pageJSONData("Atoggle2", String(wundApiSwitch));
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

    notifyClients(payload);
  }
  writeVarVals(varFilename);
}

void webServerCode(void *pvParameters)
{

  ASyncServer.on("/index", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(LittleFS, "/index.html", "text/html", false);
  });

  ASyncServer.on("index", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(LittleFS, "/index.html", "text/html", false);
  });

  ASyncServer.on("/", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(LittleFS, "/index.html", "text/html", false);
  });
  ASyncServer.on("/connect", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    sleepSwitch = 0;
    request->send(LittleFS, "/index.html", "text/html", false);
  });

  ASyncServer.on("/disconnect", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    sleepSwitch = 1;
    request->send(LittleFS, "/index.html", "text/html", false);
  });

  ASyncServer.on("/systemLog.csv", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SD, "/systemLog.csv", String(), false);
  });


  ASyncServer.on("/chart", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(LittleFS, "/charts.html", "text/html", false);
  });

  ASyncServer.on("/system", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(LittleFS, "/system.html", "text/html", false);
  });

  ASyncServer.on("/varVals.json", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(SD, "/varVals.json", String(), false);
  });

  ASyncServer.serveStatic("/192x192-splash.png", LittleFS, "/192x192-splash.png");
  ASyncServer.serveStatic("/512x512-splash.png", LittleFS, "/512x512-splash.png");
  ASyncServer.serveStatic("/splash.svg", LittleFS, "/splash.svg");
  ASyncServer.serveStatic("/sw.js", LittleFS, "/sw.js");
  ASyncServer.serveStatic("/index.js", LittleFS, "/index.js");
  ASyncServer.serveStatic("/manifest.json", LittleFS, "/manifest.json");
  ASyncServer.serveStatic("/style.css", LittleFS, "/style.css");
  ASyncServer.serveStatic("/chart.js", LittleFS, "/chart.js");

  ASyncServer.serveStatic("/bootstrap.min.css", LittleFS, "/bootstrap.min.css");
  ASyncServer.serveStatic("/bootstrap.min.js", LittleFS, "/bootstrap.min.js");

  ASyncServer.serveStatic("/update.js", LittleFS, "/update.js");
  ASyncServer.serveStatic("/Wind-Marker.svg", LittleFS, "/Wind-Marker.svg");
  ASyncServer.serveStatic("/Wind-Dial.svg", LittleFS, "/Wind-Dial.svg");
  ASyncServer.serveStatic("/favicon-32x32.png", LittleFS, "/favicon-32x32.png");

  ASyncServer.on("/restart", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->redirect("/system");
    systemRestart("Restart initiated");
  });

  ASyncServer.on("/clearerrorlog", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    serialTerminal(0, "Clearing log file");
    clearLogFile();
    request->redirect("/index");

  });

  // below to move to websockets
  /*
    ASyncServer.on("/saveWiFiRestart", HTTP_GET, [](AsyncWebServerRequest * request) {
      String tempWifiSSID, tempWifiPW;
      if ((request->hasParam(wifiSSIDInput)) && (request->hasParam(wifiPwInput))) {
        tempWifiSSID = request->getParam(wifiSSIDInput)->value();
        int ssidLen = tempWifiSSID.length() + 1;
        ssid[ssidLen];
        tempWifiSSID.toCharArray(ssid, ssidLen);

        tempWifiPW = request->getParam(wifiPwInput)->value();

        int pwLen = tempWifiPW.length() + 1;
        password[pwLen];
        tempWifiPW.toCharArray(password, pwLen);
      } else {
        //  inputMessage = "No message sent";
        // inputParam = "none";
      }
      request->send(LittleFS, "/system.html", String(), false);
      request->redirect("/system");

      systemRestart("WiFi Config Saved");
    });

    ASyncServer.on("/saveWiFi", HTTP_GET, [](AsyncWebServerRequest * request) {
      String tempWifiSSID, tempWifiPW;
      if ((request->hasParam(wifiSSIDInput)) && (request->hasParam(wifiPwInput))) {
        tempWifiSSID = request->getParam(wifiSSIDInput)->value();
        int ssidLen = tempWifiSSID.length() + 1;
        ssid[ssidLen];
        tempWifiSSID.toCharArray(ssid, ssidLen);

        tempWifiPW = request->getParam(wifiPwInput)->value();

        int pwLen = tempWifiPW.length() + 1;
        password[pwLen];
        tempWifiPW.toCharArray(password, pwLen);
      } else {
        //  inputMessage = "No message sent";
        // inputParam = "none";
      }
      request->send(LittleFS, "/system.html", String(), false);

      //delay(200);
    });
  */

  ASyncServer.onNotFound(notFound);

  vTaskDelete(NULL);
}

void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Not found");
}

void rainIRQ()
{
  if (millis() - prevRainTimer >= rainTimer)
  {
    prevRainTimer = millis();
    rainStateChange = digitalRead(RAIN_PIN);

    // compare the rainStateChange to its previous state
    if (rainStateChange != lastrainStateChange)
    {
      // if the state has changed, increment the counter

      if (rainFirstRead == true)
      {
        dailyrainMM = 0;
        rainFirstRead = false;
      }
      else
      {

        if (rainStateChange == HIGH)
        {
          if (rainSwitch == 1)
          {
            if ((windSpdAvg3Sec < windRainCutoff) && (windSpeed < windRainCutoff))
            {
              raintime = millis();                // grab current time
              raininterval = raintime - rainlast; // calculate interval between this and last event

              if (raininterval > 250) // ignore switch-bounce glitches less than 250ms after initial edge
              {
                dailyrainMM += rainMMStep; // Each dump i  0.355 mm of water
                dailyRainIn = dailyrainMM / 10 / 2.54;
                rainMinHour[currMin.toInt()] += rainMMStep; // Increase this minute's amount of rain
                serialTerminal(0, "Rain event detected at " + currTime + " | dailyrainMM: " + dailyrainMM + "mm | rainMinHour[" + currMin.toInt() + "]: " + rainMinHour[currMin.toInt()] + "mm");
                rainlast = raintime; // set up for next event
              }
              serialTerminal(5, "Rain event detected @ " + String(windspeedKmH) + "Km/h");
            }

            if (windSpdAvg3Sec >= windRainCutoff)
            {
              serialTerminal(2, "High wind speed @ " + String(windSpdAvg3Sec) + "Km/h - Skipping rain measurement");
            }

            rainFirstRead = false;
          }
          else
          {
            serialTerminal(2, "Rain sensor off - Skipping rain measurement");
          }
        }
      }
    }
    // save the current state as the last state, for next time through the loop
    lastrainStateChange = rainStateChange;
  }
}

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
        timeSinceLastTick   = millis() - lastTick;
        lastTick = millis();
      }
    }
  }
  lastwindStateChange = windStateChange;
}

boolean validReading;
// Returns the instantaneous wind speed
float circumfKMH = 1583.3626974; //for 3 cup anemometer with 14cm diameter, 43.98229715cm circumference X 36 to convert from cm/ms to km/h.
float get_wind_speed()
{
  validReading = true;
  if (timeSinceLastTick != 0)
  {
    windSpeed = circumfKMH / timeSinceLastTick; 
  }
  if (windSpeed < 0.3)
  {
    windSpeed = 0;
  }
  if (windSpeed > 180)
  {
    validReading = false;
  }
  if (validReading)
  {
    return (windSpeed);
  }
}

unsigned long windTimer = 1000;
unsigned long prevWindTimer;

void calcWeather()
{
  if (millis() - prevWindTimer >= windTimer)
  {
    prevWindTimer = millis();
    currentSpeed = get_wind_speed();

    windspeedKmH = currentSpeed;
    windSpeedMph = windSpdAvg60Sec / 1.60934;
  }

  if (millis() - lastSecond >= 1000)
  {
    lastSecond = millis();
    threeSecCounter++;
    sixtySecCounter++;

    windSpd3Sec += currentSpeed;
    windSpd60Sec += currentSpeed;
    if (threeSecCounter >= 3)
    {
      windSpdAvg3Sec = windSpd3Sec / 3;
      serialTerminal(0, "windSpdAvg [3 Sec]: " + String(windSpdAvg3Sec));
      threeSecCounter = 0;
      windSpd3Sec = 0;
    }

    if (sixtySecCounter >= 60)
    {
      windSpdAvg60Sec = windSpd60Sec / 60;
      serialTerminal(0, "windSpdAvg [60 Sec]: " + String(windSpdAvg60Sec));
      sixtySecCounter = 0;
      windSpd60Sec = 0;
    }

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

        int windIdx = ((winddir % 360) / 22.5);
        windDirCd = windDir[windIdx];
      }
    }
    else
    {
      serialTerminal(3, "Wind vane disabled");
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

    windGMax = minMaxValue("max", windGMax, currentSpeed, 1);
    windGMin = minMaxValue("min", windGMin, currentSpeed, 1);

    bmeSensorRead();
    tempMax = minMaxValue("max", tempMax, outsideTemp, -100);
    tempMin = minMaxValue("min", tempMin, outsideTemp, -100);

    humidityMax = minMaxValue("max", humidityMax, sensorHum, 0);
    humidityMin = minMaxValue("min", humidityMin, sensorHum, 0);

    pressureMax = minMaxValue("max", pressureMax, sensPress, 0);
    pressureMin = minMaxValue("min", pressureMin, sensPress, 0);

    // Reset min/max variables daily at 23:59:58
    if ((currHour == "23") && (currMin == "59") && (currSec.toInt() >= 58))
    {
      dailyrainMM = 0;
      rainHourMM = 0;
      rainIn = 0;
      dailyRainIn = 0;
      windGMax = 0;
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
  if (bmeSwitch == 1)
  {
    if (bme.begin(bmeAdr, &Wire))
    {
      bmeSensorDetected = true;
      serialTerminal(0, "BME280 sensor [OK] - Reading live sensor data");

      float tempReading, pressReading, humReading;
      int sensorSamples = 5;
      outsideTemp = 0, sensPress = 0, sensorHum = 0;
      for (int i = 0; i < sensorSamples; i++)
      {
        tempReading = bme.readTemperature();
        pressReading = bme.readPressure() / 100.0F;
        humReading = bme.readHumidity();

        serialTerminal(1, "T:" + String(tempReading) + " P:" + String(pressReading) + " H:" + String(humReading));

        firstBMEReading = false;

        if (isnan(tempReading) || isnan(pressReading) || isnan(humReading))
        {
          serialTerminal(3, "No BME280 sensor detected");

          bmeSensorDetected = false;
          validBMEReading = false;
          nanValue = true;
        }
        else
        {
          outsideTemp = tempReading + outsideTemp;
          sensPress = pressReading + sensPress;
          sensorHum = sensorHum + humReading;
          nanValue = false;
        }
      }
      outsideTemp = outsideTemp / sensorSamples;
      sensPress = sensPress / sensorSamples;
      sensorHum = sensorHum / sensorSamples;

      prevTemp = outsideTemp;
      prevPress = sensPress;
      prevHum = sensorHum;

      serialTerminal(1, "Avg T:" + String(outsideTemp) + " P:" + String(sensPress) + " H:" + String(sensorHum));

      if ((outsideTemp == 0) && (sensorHum == 0) && (sensPress == 0))
      {
        validBMEReading = false;

        serialTerminal(3, "Invalid BME280 reading T:" + String(outsideTemp) + " P:" + String(sensPress) + " H:" + String(sensorHum));
      }
      else if ((outsideTemp > -10) && (sensorHum > 0.1) && (sensPress > 900) && (sensPress < 1050))
      {
        validBMEReading = true;
        nanValue = false;
        bmeSensorDetected = true;

        serialTerminal(1, "Valid BME280 reading");
        serialTerminal(1, "Valid T:" + String(outsideTemp) + " P:" + String(sensPress) + " H:" + String(sensorHum));

        // Real feel calculation - uncomment below if you want to include. Leave to save processing time
        /*if (outsideTemp >= 27)
          {
          realFeel = c1 + (c2 * outsideTemp) + (c3 * sensorHum) + (c4 * sensorHum * outsideTemp) + (c5 * (pow(outsideTemp, 2))) + (c6 * (pow(sensorHum, 2))) + (c7 * (pow(outsideTemp, 2)) * sensorHum) + (c8 * outsideTemp * (pow(sensorHum, 2))) + (c9 * (pow(outsideTemp, 2)) * (pow(sensorHum, 2)));
          }
          else
          {
          realFeel = outsideTemp;
          }*/

        // dew point calc
        dewPoint = outsideTemp - (((100 - sensorHum) / 5));

        // convert to Imperial for Wunderground
        tempF = (outsideTemp * 9 / 5) + 32;
        pressIn = (sensPress * 0.02953);
        dewPF = (dewPoint * 9 / 5) + 32;

        serialTerminal(1, "After read bmeSensorDetected: " + String(bmeSensorDetected) + "validBMEReading: " + String(validBMEReading) + "nanValue: " + String(nanValue));
      }
    }
    else if (!bme.begin(bmeAdr, &Wire))
    {
      serialTerminal(3, "No BME280 sensor detected");
      LedBlinker(ledSwitch, 500, LEDRed, 1000, LEDOrange, 3);
      bmeSensorDetected = false;
      validBMEReading = false;
      detectBME280();
    }
  }
  else
  {
    serialTerminal(2, "BME280 switch off - skipping measurement");
  }
}

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

unsigned long sensorTimer;
bool sensorSendStatus = false, sleepStatus = true;
int wakeTime = 15;
bool wundAPIcall = false, windAPIcall = false, tsAPIcall = false;

void sendSensorData()
{
  if (millis() - sensorTimer >= 250)
  {
    sensorTimer = millis();
    if ((currSec.toInt() > 0) && (currSec.toInt() <= wakeTime) && (sensorSendStatus == false))
    {
      if ((currSec.toInt() > 0) && (!wundAPIcall))
      {
        updateWundAPI();
        wundAPIcall = true;
      }
      if ((currSec.toInt() > 3) && (!tsAPIcall))
      {
        updateThingSpeak();
        tsAPIcall = true;
      }
      if ((currSec.toInt() > 9) && (!windAPIcall))
      {
        updateWindyAPI();
        windAPIcall = true;
        sensorSendStatus = true;

      }
    }

    if (currSec.toInt() > wakeTime)
    {
      wundAPIcall = false;
      tsAPIcall = false;
      windAPIcall = false;
      sensorSendStatus = false;
    }
  }
}

void disableWiFi()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    initalBoot = false;
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

void enableWiFi()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    WiFi.disconnect(false); // Reconnect the network
    WiFi.mode(WIFI_STA);

    if (wifiMultiSwitch)
    {
      wifiMulti.addAP(ssid, password);

      if (initalBoot == true)
      {
        serialTerminal(5, "WIFI Enabled");
        serialTerminal(5, "Scanning for SSIDs: " + String(ssid));
      }

      // WiFi.scanNetworks will give total networks

      int n = WiFi.scanNetworks(); /*Scan for available network*/
      serialTerminal(5, "WiFi Scan done");
      if (n == 0)
      {
        serialTerminal(5, "No Available Networks");
      }
      else
      {
        serialTerminal(5, String(n) + " Networks found"); 
        for (int i = 0; i < n; ++i)
        {
          serialTerminal(5, String(i + 1) + ": " + WiFi.SSID(i) + " dB: " + WiFi.RSSI(i) + " Encryption: " + WiFi.encryptionType(i));
          delay(10);
        }
      }

      unsigned long startAttemptTime = millis();

      while ((wifiMulti.run() != WL_CONNECTED) && (millis() - startAttemptTime < 20000))
      {
        serialTerminal(0, ".");
        LedBlinker(ledSwitch, 100, LEDGreen, 500, LEDBlue, 1);
        delay(250);
      }
    }

    if (!wifiMultiSwitch)
    {
      WiFi.begin(ssid, password);
      if (initalBoot == true)
      {
        serialTerminal(4, "WIFI Enabled");
        serialTerminal(4, "Connecting to SSID" + String(ssid));
      }
      unsigned long startAttemptTime = millis();
      while ((WiFi.status() != WL_CONNECTED) && (millis() - startAttemptTime < 10000))
      {
        serialTerminal(0, ".");
        LedBlinker(ledSwitch, 100, LEDGreen, 500, LEDBlue, 1);
        delay(250);
      }
    }

    if (WiFi.status() == WL_CONNECTED)
    {
      rssi = WiFi.RSSI();
      IPaddress = String() + WiFi.localIP()[0] + "." + WiFi.localIP()[1] + "." + WiFi.localIP()[2] + "." + WiFi.localIP()[3];
      if (initalBoot == true)
      {
        serialTerminal(5, "SSID: " + String(ssid) + " IP: " + IPaddress + " RSSI: " + String(rssi));
      }
      LedBlinker(ledSwitch, 50, LEDBlue, 100, LEDBlack, 3);
    }
    else if (WiFi.status() != WL_CONNECTED)
    {
      LedBlinker(ledSwitch, 50, LEDRed, 100, LEDBlack, 3);
    }
  }
}


void monitorWiFiRSSI()
{
  if (WiFi.RSSI() < minWifiRSSI)
  {
    String logMessage = "Reconnecting WiFi - RSSI < " + String(minWifiRSSI) + " at " + String(WiFi.RSSI()) + "dB";
    serialTerminal(2, logMessage);
    disableWiFi();
    delay(100);
    enableWiFi();
  }
  else if (WiFi.RSSI() >= minWifiRSSI)
  {
    String logMessage = "WiFi - RSSI >= " + String(minWifiRSSI) + " at " + String(WiFi.RSSI()) + "dB";
    serialTerminal(0, logMessage);
  }
}


void wakeModemSleep()
{
  // setCpuFrequencyMhz(240);
  serialTerminal(0, "Modem Sleep Disabled - CPU @ 240Mhz");
  enableWiFi();
}

//Publish data to web server
unsigned long webTimer;
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
      payload = wxJSONData("memorySD", String(freeBytesSD) + "MB");
      payload = wxJSONData("freeHeapMem", String(freeMem) + "K");

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
      if (!thingAPI.begin("https://api.thingspeak.com/update?api_key=" + thingSpkAPIwR + "&field1=" + String(outsideTemp) + "&field2=" + String(sensPress) + "&field3=" + String(sensorHum) + "&field4=" + String(dewPoint) + "&field5=" + String(winddir) + "&field6=" + String(windSpdAvg60Sec) + "&field7=" + String(loadVoltage) + "&field8=" + String(dailyrainMM)))
      {
        serialTerminal(2, "Failed to connect to thingAPI");
        LedBlinker(ledSwitch, 500, LEDRed, 500, LEDOrange, 3);
        thingAPI.end();
      }
      else
      {
        thingStatus = thingAPI.GET();

        thingAPI.end();

        if (thingStatus == 200)
        {
          serialTerminal(0, "httpCode1: " + String(thingStatus));
          LedBlinker(ledSwitch, 200, LEDLightSteelBlue, 500, LEDGreen, 3);
          serialTerminal(0, "thingAPI [Updated] :: httpCode1: " + String(thingStatus));
        }
        else
        {
          serialTerminal(1, "Failed to connect to thingAPI - error code: " + String(thingStatus));
          LedBlinker(ledSwitch, 500, LEDRed, 500, LEDOrange, 3);
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

      String payload = "https://stations.windy.com/pws/update/" + windyAPIKey + "?temp=" + String(outsideTemp) + "&station=" + stationID + "&mbar=" + String(sensPress) + "&rh=" + String(sensorHum) + "&dewpoint=" + String(dewPoint);

      if (windgustKmH > 0)
      {
        payload = payload + "&gust=" + String(windgustKmH / 3.6);
      }
      if (windVaneStatus == true)
      {
        payload = payload + "&winddir=" + String(winddir) + "&windspeedmph=" + String(windSpdAvg60Sec / 3.6);
      }
      if (rainHourMM > 0)
      {
        payload = payload + "&precip=" + String(rainHourMM);
      }
      serialTerminal(0, payload);
      if (!windyAPI.begin(payload))
      {
        serialTerminal(2, "Failed to connect to Windy API");
        LedBlinker(ledSwitch, 500, LEDRed, 500, LEDOrange, 4);
        windyAPI.end();
      }
      else
      {
        windyStatus = windyAPI.GET();

        windyAPI.end();
        if (windyStatus == 200)
        {

          LedBlinker(ledSwitch, 200, LEDYellow, 500, LEDGreen, 4);
          serialTerminal(0, "windyAPI [Updated] :: httpCode: " + String(windyStatus));
        }
        else
        {
          serialTerminal(1, "windyAPI [Failed] :: error code: " + String(windyStatus));
          LedBlinker(ledSwitch, 500, LEDRed, 500, LEDOrange, 4);
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
      String payload = "https://weatherstation.wunderground.com/weatherstation/updateweatherstation.php?ID=" + wundStationID + "&PASSWORD=" + wundStationPw + "&tempf=" + String(tempF) + "&baromin=" + String((pressIn)) + "&humidity=" + String((sensorHum)) + "&dewptf=" + String(dewPF) + "&dateutc=now&action=updateraw";

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
      serialTerminal(0, payload);
      if (!wundAPI.begin(payload))
      {
        LedBlinker(ledSwitch, 500, LEDRed, 500, LEDOrange, 5);
        serialTerminal(2, "Failed to connect to Wunderground API");
        wundAPI.end();
      }
      else
      {
        wundStatus = wundAPI.GET();
        wundAPI.end();

        if (wundStatus == 200)
        {
          LedBlinker(ledSwitch, 200, LEDBlue, 500, LEDGreen, 4);
          serialTerminal(0, "wundAPI [Updated] :: httpCode: " + String(wundStatus));
        }
        else
        {
          serialTerminal(1, "wundAPI [Failed] :: error code: " + String(wundStatus));
          LedBlinker(ledSwitch, 500, LEDRed, 500, LEDOrange, 4);
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

void printWeather()
{
  if (wsStatus == true)
  {
    serialTerminal(0, "winddir=" + String(winddir) + " windspeedKmH=" + String(windspeedKmH) + " windgustKmH=" + String(windgustKmH) + " windgustdir=" + String(windgustdir) + " rainMinHour[" + String(currMin.toInt()) + "] " + rainMinHour[currMin.toInt()] + " rainHourMM=" + String(rainHourMM) + " dailyrainMM=" + String(dailyrainMM) + " Temperature: " + String(outsideTemp) + " Pressure: " + String(sensPress) + " Humidity: " + String(sensorHum) + " RSSI: " + String(rssi) + " loadVoltage: " + String(loadVoltage) + " current mA: " + String(current_mA));
  }
}

// Checks for BME280 as can be different between variants. Usually 0x76 or 0x77
void detectBME280()
{
  if (bme.begin(0x76, &Wire))
  {
    serialTerminal(0, "0x76 BME280 sensor detected");
    bmeSensorDetected = true;
    
    bmeAdr = 0x76;
  }
  else if (bme.begin(0x77, &Wire))
  {
    serialTerminal(0, "0x77 BME280 sensor detected");
    bmeSensorDetected = true;
  }
  else
  {
    serialTerminal(3, "No BME280 sensor detected");
    bmeSensorDetected = false;
    validBMEReading = false;
    LedBlinker(ledSwitch, 500, LEDRed, 1000, LEDOrange, 3);
  }
  if(bmeSensorDetected)
  {
    bme.setSampling(Adafruit_BME280::MODE_FORCED,
                    Adafruit_BME280::SAMPLING_X1, 
                    Adafruit_BME280::SAMPLING_X1, 
                    Adafruit_BME280::SAMPLING_X1, 
                    Adafruit_BME280::FILTER_OFF);
    serialTerminal(0, "forced mode, 1x temperature / 1x humidity / 1x pressure oversampling, ");
    serialTerminal(0, "filter off");
  }
}

unsigned long getTime()
{
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    serialTerminal(3, "Failed to obtain time");
    return (0);
  }
  time(&now);
  return now;
}

void syncTime(unsigned long syncTimer)
{
  int variance = 0, tempTime = 0;
  if (((millis() - clockSyncTimer >= syncTimer) || (initialTimeSync == true)) && (sensorSendStatus == true))

  {
    if (getTime() > 0)
    {
      tempTime = getTime();
      variance = epochTime - tempTime;
      epochTime = tempTime;
      serialTerminal(0, "Time Sync [OK] - UNIX EpochTime: " + String(epochTime) + " Variance: " + String(variance));
      LedBlinker(ledSwitch, 200, LEDLightSteelBlue, 200, LEDYellow, 3);
      timeSyncStatus = true;
    }
    else
    {
      serialTerminal(2, "Failed to obtain time");
      LedBlinker(ledSwitch, 500, LEDRed, 500, LEDOrange, 5);
      timeSyncStatus = false;
    }
    clockSyncTimer = millis();
    setCurrentTime();
    serialTerminal(0, "current Time: " + currTime);
  }
}

void setCurrentTime()
{

  if (millis() - clockTimer >= 1000)
  {
    clockTimer = millis();
    epochTime++;

    breakTime(epochTime + UTCOffset_sec, unixTimeStamp);

    currDay = unixTimeStamp.Day;

    weekDayIndex = unixTimeStamp.Wday - 1;
    currLWeekDay = Weekday[weekDayIndex];
    currSWeekDay = shWeekday[weekDayIndex];

    monthIndex = unixTimeStamp.Month;
    currMonth = longMonth[monthIndex];

    currYear = unixTimeStamp.Year + epochYear;

    if (unixTimeStamp.Minute < 10)
    {
      currMin = "0" + String(unixTimeStamp.Minute);
    }
    else
    {
      currMin = String(unixTimeStamp.Minute);
    }

    if (unixTimeStamp.Hour < 10)
    {
      currHour = "0" + String(unixTimeStamp.Hour);
    }
    else
    {
      currHour = String(unixTimeStamp.Hour);
    }

    if (unixTimeStamp.Second < 10)
    {
      currSec = "0" + String(unixTimeStamp.Second);
    }
    else
    {
      currSec = String(unixTimeStamp.Second);
    }
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
  serialTerminal(2, "RESTARTING DEVICE in 3 sec");
  delay(3000);
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
//
//void writeVarVals(const char *filename)
//{
//  if ((initalBoot == false) && (initSD == true))
//  {
//    SD.remove(filename);
//    // Open file for writing
//    File file = SD.open(filename, FILE_WRITE);
//    if (!file)
//    {
//      serialTerminal(3, "Failed to create file");
//      return;
//    }
//    else
//    {
//      String payload;
//
//      if ((validBMEReading == true) && (nanValue == false))
//      {
//        payload = wxVarValsJSON("outsideTemp", String(outsideTemp));
//        payload = wxVarValsJSON("sensPress", String(sensPress));
//        payload = wxVarValsJSON("sensorHum", String(sensorHum));
//      }
//      payload = wxVarValsJSON("tempMin", String(tempMin));
//      payload = wxVarValsJSON("tempMax", String(tempMax));
//      payload = wxVarValsJSON("humidityMin", String(humidityMin));
//      payload = wxVarValsJSON("humidityMax", String(humidityMax));
//      payload = wxVarValsJSON("pressureMin", String(pressureMin));
//      payload = wxVarValsJSON("pressureMax", String(pressureMax));
//      payload = wxVarValsJSON("windMax", String(windMax));
//      payload = wxVarValsJSON("windAvg", String(windSpdAvg60Sec));
//      payload = wxVarValsJSON("windGMin", String(windGMin));
//      payload = wxVarValsJSON("windGMax", String(windGMax));
//      payload = wxVarValsJSON("windGAvg", String(windGAvg));
//      payload = wxVarValsJSON("loadVoltage", String(loadVoltage));
//      payload = wxVarValsJSON("power_mW", String(power_mW));
//      payload = wxVarValsJSON("currentmA", String(current_mA));
//      payload = wxVarValsJSON("timeStamp", String(currTime));
//      payload = wxVarValsJSON("dataRefresh", String(wxDataUpload));
//      payload = wxVarValsJSON("rssilimit", String(minWifiRSSI));
//      payload = wxVarValsJSON("wifiinterval", String(wifiRSSIInterval));
//      payload = wxVarValsJSON("windRainCutoff", String(windRainCutoff));
//      payload = wxVarValsJSON("savedRainHourMM", String(rainHourMM));
//      payload = wxVarValsJSON("dailyrainMM", String(dailyrainMM));
//      payload = wxVarValsJSON("version", versionNR);
//      payload = wxVarValsJSON("firmware", versionNR);
//      payload = wxVarValsJSON("windyStatus", String(windyStatus));
//      payload = wxVarValsJSON("thingStatus", String(thingStatus));
//      payload = wxVarValsJSON("wundStatus", String(wundStatus));
//      payload = wxVarValsJSON("memory", String(freeBytesLFS) + "KB");
//      payload = wxVarValsJSON("darkModeSwitch", String(darkModeSwitch));
//      payload = wxVarValsJSON("wifiMultiSwitch", String(wifiMultiSwitch));
//      payload = wxVarValsJSON("errorLogSwitch", String(errorLogSwitch));
//      payload = wxVarValsJSON("deepSleepTime", String(deepSleepTime));
//      payload = wxVarValsJSON("sleepSwitch", String(sleepSwitch));
//      payload = wxVarValsJSON("battCheckSwitch", String(battCheckSwitch));
//      payload = wxVarValsJSON("windyApiSwitch", String(windyApiSwitch));
//      payload = wxVarValsJSON("thingSApiSwitch", String(thingSApiSwitch));
//      payload = wxVarValsJSON("wundApiSwitch", String(wundApiSwitch));
//      payload = wxVarValsJSON("bmeSwitch", String(bmeSwitch));
//      payload = wxVarValsJSON("windDirSwitch", String(windDirSwitch));
//      payload = wxVarValsJSON("windSpdSwitch", String(windSpdSwitch));
//      payload = wxVarValsJSON("rainSwitch", String(rainSwitch));
//      payload = wxVarValsJSON("ledSwitch", String(ledSwitch));
//
//      file.println(payload);
//      serialTerminal(0, "Variable JSON Saved: " + String(payload));
//    }
//    // Close the file
//    file.close();
//  }
//  if (initSD == false)
//  {
//    serialTerminal(3, "Variable JSON Write Failed - SD Card not initalised!");
//  }
//}
//
//bool varJsonRead;
//void readVarVals(const char *filename)
//{
//  // Open file for reading
//  if ((SD.exists(filename))&& (initSD == true))
//  {
//
//    // File jsonFile = SD.open(filename,FILE_READ);
//    String jsonFile = readFile(SD, filename);
//    if (Serial)
//    {
//      Serial.println(("Reading Variable JSON Config"));
//      Serial.println((jsonFile));
//    }
//    DynamicJsonDocument doc(1600);
//
//    // Deserialize the JSON document
//    DeserializationError error = deserializeJson(doc, jsonFile);
//    switch (error.code())
//    {
//      case DeserializationError::Ok:
//        Serial.println(("Variable JSON Deserialization succeeded"));
//        varJsonRead = true;
//        serialTerminal(5, "Variable JSON Deserialization succeeded varJsonRead: " + String(varJsonRead));
//        break;
//      case DeserializationError::InvalidInput:
//        Serial.println(("Variable JSON Invalid input!"));
//        varJsonRead = false;
//        serialTerminal(5, "Variable JSON Invalid input! varJsonRead: " + String(varJsonRead));
//        break;
//      case DeserializationError::NoMemory:
//        Serial.println(("Variable JSON Not enough memory"));
//        varJsonRead = false;
//        serialTerminal(5, "Variable JSON Not enough memory varJsonRead: " + String(varJsonRead));
//        break;
//      default:
//        Serial.println(("Variable JSON Deserialization failed"));
//        serialTerminal(5, "Variable JSON Deserialization failed");
//        varJsonRead = false;
//        break;
//    }
//    if (varJsonRead == true)
//    {
//      tempMin = doc["tempMin"].as<float>();
//      tempMax = doc["tempMax"].as<float>();
//      humidityMin = doc["humidityMin"].as<float>();
//      humidityMax = doc["humidityMax"].as<float>();
//      pressureMin = doc["pressureMin"].as<float>();
//      pressureMax = doc["pressureMax"].as<float>();
//      windMax = doc["windMax"].as<float>();
//      windAvg = doc["windAvg"].as<float>();
//      windGMin = doc["windGMin"].as<float>();
//      windGMax = doc["windGMax"].as<float>();
//      windGAvg = doc["windGAvg"].as<float>();
//      wxDataUpload = doc["dataRefresh"].as<int>();
//      wifiRSSIInterval = doc["wifiinterval"].as<int>();
//      windRainCutoff = doc["windRainCutoff"].as<int>();
//      savedRainHourMM = doc["savedRainHourMM"].as<float>();
//      dailyrainMM = doc["dailyrainMM"].as<float>();
//      darkModeSwitch = doc["darkModeSwitch"].as<int>();
//      wifiMultiSwitch = doc["wifiMultiSwitch"].as<int>();
//      errorLogSwitch = doc["errorLogSwitch"].as<int>();
//      deepSleepTime = doc["deepSleepTime"].as<int>();
//      sleepSwitch = doc["sleepSwitch"].as<int>();
//      battCheckSwitch = doc["battCheckSwitch"].as<int>();
//
//      windyApiSwitch = doc["windyApiSwitch"].as<int>();
//      thingSApiSwitch = doc["thingSApiSwitch"].as<int>();
//      wundApiSwitch = doc["wundApiSwitch"].as<int>();
//
//      bmeSwitch = doc["bmeSwitch"].as<int>();
//      windDirSwitch = doc["windDirSwitch"].as<int>();
//      windSpdSwitch = doc["windSpdSwitch"].as<int>();
//      rainSwitch = doc["rainSwitch"].as<int>();
//      ledSwitch = doc["ledSwitch"].as<int>();
//
//      serialTerminal(5, "tempMin: " + String(tempMin) + " tempMax: " + String(tempMax) + " humidityMin: " + String(humidityMin) + " humidityMax: " + String(humidityMax) + " pressureMin: " + String(pressureMin) + " pressureMax: " + String(pressureMax) + " windMax: " + String(windMax) + " windAvg: " + String(windAvg) + " windGMin: " + String(windGMin) + " windGMax: " + String(windGMax) + " windGAvg: " + String(windGAvg) + " dataRefresh: " + String(wxDataUpload) + " wifiinterval: " + String(wifiRSSIInterval) + " windRainCutoff: " + String(windRainCutoff) + " savedRainHourMM: " + String(savedRainHourMM) + " dailyrainMM: " + String(dailyrainMM) + " darkModeSwitch: " + String(darkModeSwitch) + " wifiMultiSwitch: " + String(wifiMultiSwitch) + " errorLogSwitch: " + String(errorLogSwitch) + " deepSleepTime: " + String(deepSleepTime) + " sleepSwitch: " + String(sleepSwitch) + " battCheckSwitch: " + String(battCheckSwitch) + " windyApiSwitch: " + String(windyApiSwitch) + " thingSApiSwitch: " + String(thingSApiSwitch) + " wundApiSwitch: " + String(wundApiSwitch) + " bmeSwitch: " + String(bmeSwitch) + " windDirSwitch: " + String(windDirSwitch) + " windSpdSwitch: " + String(windSpdSwitch) + " rainSwitch: " + String(rainSwitch) + " ledSwitch: " + String(ledSwitch));
//    }
//  }
//  if (initSD == false)
//  {
//    serialTerminal(3, "Variable JSON Write Failed - SD Card not initalised!");
//  }
//}


void writeVarVals(const char *filename)
{
  if (initalBoot == false)
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

    // File jsonFile = LittleFS.open(filename,FILE_READ);
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

      bmeSwitch = doc["bmeSwitch"].as<int>();
      windDirSwitch = doc["windDirSwitch"].as<int>();
      windSpdSwitch = doc["windSpdSwitch"].as<int>();
      rainSwitch = doc["rainSwitch"].as<int>();
      ledSwitch = doc["ledSwitch"].as<int>();

      serialTerminal(5, "tempMin: " + String(tempMin) + " tempMax: " + String(tempMax) + " humidityMin: " + String(humidityMin) + " humidityMax: " + String(humidityMax) + " pressureMin: " + String(pressureMin) + " pressureMax: " + String(pressureMax) + " windMax: " + String(windMax) + " windAvg: " + String(windAvg) + " windGMin: " + String(windGMin) + " windGMax: " + String(windGMax) + " windGAvg: " + String(windGAvg) + " dataRefresh: " + String(wxDataUpload) + " wifiinterval: " + String(wifiRSSIInterval) + " windRainCutoff: " + String(windRainCutoff) + " savedRainHourMM: " + String(savedRainHourMM) + " dailyrainMM: " + String(dailyrainMM) + " darkModeSwitch: " + String(darkModeSwitch) + " wifiMultiSwitch: " + String(wifiMultiSwitch) + " errorLogSwitch: " + String(errorLogSwitch) + " deepSleepTime: " + String(deepSleepTime) + " sleepSwitch: " + String(sleepSwitch) + " battCheckSwitch: " + String(battCheckSwitch) + " windyApiSwitch: " + String(windyApiSwitch) + " thingSApiSwitch: " + String(thingSApiSwitch) + " wundApiSwitch: " + String(wundApiSwitch) + " bmeSwitch: " + String(bmeSwitch) + " windDirSwitch: " + String(windDirSwitch) + " windSpdSwitch: " + String(windSpdSwitch) + " rainSwitch: " + String(rainSwitch) + " ledSwitch: " + String(ledSwitch));
    }
  }
}

void loop()
{

  wspeedIRQ();
  rainIRQ();
  calcWeather();
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

  if (millis() - variableTimer >= 45000) {
    variableTimer = millis();
    writeVarVals(varFilename);
    batteryMonitor();
  }

  if (millis() - wifiRSSiTimer >= wifiRSSIInterval)
  {
    monitorWiFiRSSI();
    wifiRSSiTimer = millis();
  }
  ArduinoOTA.handle();
}
