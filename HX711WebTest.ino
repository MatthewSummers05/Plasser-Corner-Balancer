#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "SPIFFS.h"
#include <Arduino_JSON.h>
#include "HX711.h"
#include "soc/rtc.h"
#include <ESPmDNS.h>
#include <LiquidCrystalIO.h>
#include <IoAbstractionWire.h>
#include <Wire.h>
#include "OneButton.h"
#include <INA219_WE.h>
#define I2C_ADDRESS 0x40
#define PIN_INPUT1 35
#define PIN_INPUT2 36

OneButton button1(PIN_INPUT1, false, false);
OneButton button2(PIN_INPUT2, false, false);

LiquidCrystalI2C_RS_EN(lcd, 0x20, false)

/*
TO DO
change network password and ssid
*/
float busVoltage_V = 0.0;
int batteryPercent;

unsigned long previousMillis1 = 0;
unsigned long previousMillis2 = 0;
unsigned long buttonPressStartTime;
unsigned long longPressDuration = 3000;
const long interval1 = 1000;
const long interval2 = 10;

String wifi_network_ssid;
String wifi_network_password;

String wifi_ssid = "EMDS_WiFi";
String wifi_password = "9a7b3c1d3e4f1g";

const char* soft_ap_ssid = "Plasser_Scale1";
const char* soft_ap_password = "123456789";

const char* calibFactorFilePath = "/caliFactor.txt";
const char* networkSSIDFilePath = "/ssid.json";
const char* networkPasswordFilePath = "/password.json";
const char* hostname = "plasserscale1";

const int LOADCELL1_DOUT_PIN = 14;
const int LOADCELL1_SCK_PIN = 13;
const int LOADCELL2_DOUT_PIN = 26;
const int LOADCELL2_SCK_PIN = 25;
const int LOADCELL3_DOUT_PIN = 33;
const int LOADCELL3_SCK_PIN = 32;
const int LOADCELL4_DOUT_PIN = 19;
const int LOADCELL4_SCK_PIN = 18;

//Starting the Webserver on port 80//
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
JSONVar readings;

String message = "";
String caliFactor1;
String caliFactor2;
String caliFactor3;
String caliFactor4;
double calibFactor1;
double calibFactor2;
double calibFactor3;
double calibFactor4;
String header;

int frntLWeight;
int frntRWeight;
int bckLWeight;
int bckRWeight;

int lcdIndex = 0;
bool buttonPressed = false;

HX711 scale1; //Front Left//
HX711 scale2; //Front Right//
HX711 scale3; //Back Left//
HX711 scale4; //Back Right// 
INA219_WE ina219 = INA219_WE(I2C_ADDRESS);

String getWeightReadings() {
  frntLWeight = (int)scale1.get_units(3);
  readings["weight1"] = String(frntLWeight);
  frntRWeight = (int)scale2.get_units(3);
  readings["weight2"] = String(frntRWeight);
  bckLWeight = (int)scale3.get_units(3);
  readings["weight3"] = String(bckLWeight);
  bckRWeight = (int)scale4.get_units(3);
  readings["weight4"] = String(bckRWeight);
  busVoltage_V = ina219.getBusVoltage_V();
  batteryPercent = (((busVoltage_V -12) / 4.8)*100);
  readings["batteryPercent"] = batteryPercent;


  int totalWeight = (frntLWeight + frntRWeight + bckLWeight + bckRWeight);
  readings["totalWeight"] = String(totalWeight);
  readings["caliFactor1"] = String(calibFactor1);
  readings["caliFactor2"] = String(calibFactor2);
  readings["caliFactor3"] = String(calibFactor3);
  readings["caliFactor4"] = String(calibFactor4);
  String jsonString = JSON.stringify(readings);
  return jsonString;
}

String getWeightReadingsWithScreen() { //Getting the readings and outputting them as a json file to send to the clients//
   frntLWeight = (int)scale1.get_units(3);
   readings["weight1"] = String(frntLWeight);
   frntRWeight = (int)scale2.get_units(3);
   readings["weight2"] = String(frntRWeight);
   bckLWeight = (int)scale3.get_units(3);
   readings["weight3"] = String(bckLWeight);
   bckRWeight = (int)scale4.get_units(3);
   readings["weight4"] = String(bckRWeight);
   busVoltage_V = ina219.getBusVoltage_V();
   batteryPercent = (((busVoltage_V -12) / 4.8)*100);
   readings["batteryPercent"] = batteryPercent;

   int totalWeight = (frntLWeight + frntRWeight + bckLWeight + bckRWeight);
   readings["totalWeight"] = String(totalWeight);
   readings["caliFactor1"] = String(calibFactor1);
   readings["caliFactor2"] = String(calibFactor2);
   readings["caliFactor3"] = String(calibFactor3);
   readings["caliFactor4"] = String(calibFactor4);

   lcd.clear();
   lcd.setCursor(0, 0);
   lcd.print("Front Left:");
   lcd.print(frntLWeight);
   lcd.print("kg");
   lcd.setCursor(0, 1);
   lcd.print("Front Right:");
   lcd.print(frntRWeight);
   lcd.print("kg");
   lcd.setCursor(0, 2);
   lcd.print("Back Left:");
   lcd.print(bckLWeight);
   lcd.print("kg");
   lcd.setCursor(0, 3);
   lcd.print("Back Right:");
   lcd.print(bckRWeight);
   lcd.print("kg");

   String jsonString = JSON.stringify(readings);
   return jsonString;
}

void getNetworkSSID() {
  File file = SPIFFS.open(networkSSIDFilePath, "r");
  if (!file) {
    Serial.println("Failed to open SSID file, using default value.");
    wifi_network_ssid = "default_ssid";
    return;
  }

  size_t size = file.size();
  if (size == 0) {
    Serial.println("SSID file is empty, using default value.");
    wifi_network_ssid = "default_ssid";
    file.close();
    return;
  }

  std::unique_ptr<char[]> buf(new char[size + 1]);
  file.readBytes(buf.get(), size);
  buf[size] = '\0';  // Null-terminate the buffer
  file.close();

  JSONVar config = JSON.parse(buf.get());
  if (JSON.typeof(config) == "undefined") {
    Serial.println("Failed to parse SSID file, using default value.");
    wifi_network_ssid = "default_ssid";
    return;
  }

  wifi_network_ssid = (const char*)config["ssid"];
  if (wifi_network_ssid.length() == 0) {
    wifi_network_ssid = "default_ssid";
  }

  Serial.println("SSID: " + wifi_network_ssid);
}

void saveNetworkSSID() {
  JSONVar config;
  config["ssid"] = wifi_network_ssid;

  File file = SPIFFS.open(networkSSIDFilePath, "w");
  if (!file) {
    Serial.println("Failed to open SSID file for writing.");
    notifyClients("error");
    return;
  }
  file.print(JSON.stringify(config));
  file.close();
  Serial.println("Saved SSID: " + wifi_network_ssid);
}

void getNetworkPassword() {
  File file = SPIFFS.open(networkPasswordFilePath, "r");
  if (!file) {
    Serial.println("Failed to open password file, using default value.");
    wifi_network_password = "default_password";
    return;
  }

  size_t size = file.size();
  if (size == 0) {
    Serial.println("Password file is empty, using default value.");
    wifi_network_password = "default_password";
    file.close();
    return;
  }

  std::unique_ptr<char[]> buf(new char[size + 1]);
  file.readBytes(buf.get(), size);
  buf[size] = '\0';  // Null-terminate the buffer
  file.close();

  JSONVar config = JSON.parse(buf.get());
  if (JSON.typeof(config) == "undefined") {
    Serial.println("Failed to parse password file, using default value.");
    wifi_network_password = "default_password";
    return;
  }

  wifi_network_password = (const char*)config["password"];
  if (wifi_network_password.length() == 0) {
    wifi_network_password = "default_password";
  }

  Serial.println("Password: " + wifi_network_password);
}

void saveNetworkPassword() {
  JSONVar config;
  config["password"] = wifi_network_password;

  File file = SPIFFS.open(networkPasswordFilePath, "w");
  if (!file) {
    Serial.println("Failed to open password file for writing.");
    notifyClients("error");
    return;
  }
  file.print(JSON.stringify(config));
  file.close();
  Serial.println("Saved Password: " + wifi_network_password);
}


void click1() {
  if (lcdIndex == 0)
  {
    lcd.clear();
    lcdIndex = 4;
  }
  else
  {
    lcd.clear();
    lcdIndex -= 1;
  }
  Serial.println("moved up a page");
}

void click2() {
  if (lcdIndex == 4)
  {
    lcd.clear();
    lcdIndex = 0;
  }
  else
  {
    lcd.clear();
    lcdIndex += 1;
  }
  Serial.println("moved down a page");
}

void initSPIFFS() {
  if (!SPIFFS.begin(true)) {
    Serial.println("An error has occurred while mounting SPIFFS");
    notifyClients("error");
  }
  else {
    Serial.println("SPIFFS mounted successfully");
  }
}

void notifyClients(String caliFactor) { // Notifying all clients of the redadings to display them on the webpage//
  ws.textAll(caliFactor);
}

void restartESP() {
    esp_restart();
}

//Message handler that  checks if the message is to get the readings, restart the ESP or if it is sending the new calibration factor//
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    message = (char*)data;
    //Checking if we are calling the readings from the esp for the webpage to update
    if (strcmp((char*)data, "getReadings") == 0) {
    String weightReadings = getWeightReadings();
    Serial.print(weightReadings);
    notifyClients(weightReadings);
   }
   //Checking if the message is restart and triggers a restart on the esp//
    else if (strcmp((char*)data, "restart") == 0) {
    restartESP();
   }
   //Checking if the message is tare and tares all 4 of the scales at the same time//
   else if (strcmp((char*)data, "tare") == 0) { 
    scale1.tare();
    scale2.tare();
    scale3.tare();
    scale4.tare();
    Serial.println("Tared all scales");
   }
    else {
      String receivedMessage = String((char*)data);
      if (receivedMessage.startsWith("calib1:")) {
        calibFactor1 = receivedMessage.substring(7).toDouble();
        Serial.print("Calibration Factor 1: ");
        Serial.println(calibFactor1);
      }
      else if (receivedMessage.startsWith("calib2:")) {
        calibFactor2 = receivedMessage.substring(7).toDouble();
        Serial.print("Calibration Factor 2: ");
        Serial.println(calibFactor2);
      }
      else if (receivedMessage.startsWith("calib3:")) {
        calibFactor3 = receivedMessage.substring(7).toDouble();
        Serial.print("Calibration Factor 3: ");
        Serial.println(calibFactor3);
      }
      else if (receivedMessage.startsWith("calib4:")) {
        calibFactor4 = receivedMessage.substring(7).toDouble();
        Serial.print("Calibration Factor 4: ");
        Serial.println(calibFactor4);
      }
      else if (receivedMessage.startsWith("ssid:")) {
        wifi_network_ssid = receivedMessage.substring(5);
        saveNetworkSSID();
        Serial.print("New SSID:");
        Serial.println(wifi_network_ssid);
      }
      else if (receivedMessage.startsWith("password:")) {
        wifi_network_password = receivedMessage.substring(9);
        saveNetworkPassword();
        Serial.print("New Password:");
        Serial.println(wifi_network_password);
        delay(200);
        esp_restart();
      }
      saveCalibrationFactors();
    }
  }
}

//Client connect and disconnect handler//
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      notifyClients(caliFactor1);
      notifyClients(caliFactor2);
      notifyClients(caliFactor3);
      notifyClients(caliFactor4);
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

//Starts the Websocket so that the ESP can communicate with the webpage//
void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

// Opening the text file in the SPIFFS and loads the values as the new calibration factors
void loadCalibrationFactors() {
  // Open the file
  File file = SPIFFS.open(calibFactorFilePath, "r");
  if (!file) {
    Serial.println("Failed to open calibration factor file, using defaults.");
    notifyClients("error");
    return;
  }
String lines = file.readStringUntil('\n');
if (lines.length() == 0){
  file.close();
  file = SPIFFS.open(calibFactorFilePath, "w");
  file.print("Factor 1: ");
  file.println("1");
  file.print("Factor 2: ");
  file.println("2");
  file.print("Factor 3: ");
  file.println("3");
  file.print("Factor 4: ");
  file.println("4");

  Serial.println("Wrote the cali factors into file!");

  file.close();
}
  file = SPIFFS.open(calibFactorFilePath, "r");
  // Read each line containing a calibration factor
  for (int i = 1; i < 5; ++i) {
    String line = file.readStringUntil('\n');
    if (line.length() > 0) {
      // Parse the calibration factor from the line
      float factor = line.substring(line.indexOf(':') + 1).toFloat();
      // Assign the calibration factor to the appropriate variable
      if (i == 1){
        calibFactor1 = factor;
      }
      else if (i == 2){
        calibFactor2 = factor;
      }
      else if (i == 3){
        calibFactor3 = factor;
      }
      else if (i == 4){
        calibFactor4 = factor;
      }
    }
  }

  file.close();

  Serial.println("Loaded Calibration Factors:");
  Serial.print("Factor 1: ");
  Serial.println(calibFactor1);
  Serial.print("Factor 2: ");
  Serial.println(calibFactor2);
  Serial.print("Factor 3: ");
  Serial.println(calibFactor3);
  Serial.print("Factor 4: ");
  Serial.println(calibFactor4);
}

// Saving the current calibration factors into the text file so that they can be fetched again and the values remain persistent
void saveCalibrationFactors() {
  File file = SPIFFS.open(calibFactorFilePath, "w");
  if (!file) {
    Serial.println("Failed to open calibration factor file for writing.");
    notifyClients("error");
    return;
  }

  // Write each calibration factor to the file
  file.print("Factor 1: ");
  file.println(calibFactor1);
  file.print("Factor 2: ");
  file.println(calibFactor2);
  file.print("Factor 3: ");
  file.println(calibFactor3);
  file.print("Factor 4: ");
  file.println(calibFactor4);

  file.close();

  Serial.println("Calibration Factors saved.");

  scale1.set_scale(calibFactor1);
  scale2.set_scale(calibFactor2);
  scale3.set_scale(calibFactor3);
  scale4.set_scale(calibFactor4);
  Serial.println("Set scales with new calibration factors.");
}


void setup() {
  Serial.begin(115200);
  pinMode(23, OUTPUT);
  pinMode(27, OUTPUT);
  pinMode(34, INPUT);
  digitalWrite(23, HIGH);
  digitalWrite(27, LOW);

  initWebSocket();
  initSPIFFS();

  getNetworkSSID();
  getNetworkPassword();

  Wire.begin();
  lcd.begin(20, 4);
  lcd.configureBacklightPin(3);
  lcd.backlight();
  Serial.println("Started wire and lcd");

  //Starting the ESP's WiFi as its own network and as a device on the network//
  WiFi.mode(WIFI_AP_STA);
  Serial.println("\n[*] Creating ESP32 AP");
  WiFi.softAP(soft_ap_ssid, soft_ap_password);
  Serial.print("[+] AP Created with IP Gateway ");
  Serial.println(WiFi.softAPIP());
  WiFi.begin(wifi_network_ssid, wifi_network_password);
  Serial.println("\n[*] Connecting to WiFi Network");

  unsigned long startAttemptTime = millis();
  unsigned long timeout = 15000;

  while (WiFi.status() != WL_CONNECTED && (millis() - startAttemptTime) < timeout)
  {
    Serial.print(".");
    lcd.clear();
    unsigned long previousMillis = 0;
    const long interval = 500;
     int i = 0;

     while (i <= 5) {  // Adjusted the loop condition to iterate only 3 times
    unsigned long currentMillis = millis();  // Get the current time
    
    // Check if it's time to update the screen
    if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;  // Save the last update time
        
         // Set cursor position
        
        if (i == 0)
        {  // Change condition to start from 0
            lcd.setCursor(4, 0);
            lcd.print("Loading.");
        }
        else if (i == 1)
        {
            lcd.setCursor(4, 0);
            lcd.print("Loading..");
        }
        else if (i == 2)
        {
            lcd.setCursor(4, 0);
            lcd.print("Loading...");
        }
        else if (i == 3)
        {
            lcd.setCursor(4, 0);
            lcd.print("Loading....");
        }
        else if (i == 4)
        {
            lcd.setCursor(4, 0);
            lcd.print("Loading.....");
        }
        else if (i == 5)
        {
            lcd.setCursor(4, 0);
            lcd.print("Loading......");
        }
        
        i++;
       }
    }
  }
  WiFi.setHostname(hostname);
  Serial.print("\n[+] Connected to WiFi network with local IP : ");
  Serial.println(WiFi.localIP());
  Serial.print ("\n ESP32 Hostname: ");
  Serial.println(WiFi.getHostname());
  server.begin();

  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/index.html", "text/html");
  });

  server.serveStatic("/", SPIFFS, "/");

  server.begin();

  loadCalibrationFactors();

  if (!MDNS.begin(hostname)) {
    Serial.println("Error setting up MDNS responder!");
    notifyClients("error");
  }
  Serial.println("mDNS responder started");

  //Code to start and read the HX711 modules//
    Serial.println("Initializing the scale");
    scale1.begin(LOADCELL1_DOUT_PIN, LOADCELL1_SCK_PIN);
    scale2.begin(LOADCELL2_DOUT_PIN, LOADCELL2_SCK_PIN);
    scale3.begin(LOADCELL3_DOUT_PIN, LOADCELL3_SCK_PIN);
    scale4.begin(LOADCELL4_DOUT_PIN, LOADCELL4_SCK_PIN);

    scale1.set_scale(calibFactor1);
    scale2.set_scale(calibFactor2);
    scale3.set_scale(calibFactor3);
    scale4.set_scale(calibFactor4);
    Serial.println("Set scales");

    scale1.tare();
    scale2.tare();
    scale3.tare();
    scale4.tare();
    Serial.println("Tared the scales");

    button1.attachClick(click1);
    button2.attachClick(click2);

    ina219.setMeasureMode(TRIGGERED);
}

//Loop to get a reading from the scale every 1 second as well as display the readings on the screen and the website
void loop() {
  unsigned long currentMillis = millis();
//Checking what page of the lcd we are on and call the readings and update display appropriately
//Calls get readings updating the webpage and lcd of weight values and handles what shouldbe displayed on the screen if it is not the weight values. Happens every second
if (currentMillis - previousMillis1 >= interval1)
  {
    previousMillis1 = currentMillis;
    busVoltage_V = ina219.getBusVoltage_V();
    batteryPercent = (((busVoltage_V -12) / 4.8)*100);
//Checking if the battery is low, and flashes the LED if it is low
      if (batteryPercent <= 20)
      {
        int ledState = digitalRead(27);
        ledState = !ledState;
        digitalWrite(27, ledState); 
      }
      else
      {
        digitalWrite(27, LOW);
      }
      if (lcdIndex == 0)
      {
        String weightReadings = getWeightReadingsWithScreen();
        notifyClients(weightReadings);
      }
      else if (lcdIndex == 1)
      {
        batteryPercent = (((busVoltage_V -12) / 4.8)*100);
        if (batteryPercent <= 20)
        {
          String weightReadings = getWeightReadings();
          notifyClients(weightReadings);
          lcd.setCursor(0, 0);
          lcd.print("Battery Percentage: ");
          lcd.setCursor(0, 1);
          lcd.print(batteryPercent);
          lcd.print("%");
          lcd.setCursor(0, 2);
          lcd.print("Battery Low");
          lcd.setCursor(0, 3);
          lcd.print("Connect Charger");
        }
        else if (batteryPercent >= 20)
        {
          String weightReadings = getWeightReadings();
          notifyClients(weightReadings);
          lcd.setCursor(0, 1);
          lcd.print("Battery Percentage: ");
          lcd.setCursor(0, 2);
          lcd.print(batteryPercent);
          lcd.print("%");
        }
      }
      else if (lcdIndex == 2)
      {
        String weightReadings = getWeightReadings();
        notifyClients(weightReadings);
        lcd.setCursor(0, 0);
        lcd.print("Web address:");
        lcd.setCursor(0, 1);
        lcd.print(hostname);
        lcd.print(".local");
        lcd.setCursor(0, 2);
        lcd.print("Connected WiFi:");
        lcd.setCursor(0, 3);
        lcd.print(wifi_network_ssid);
      }
     else if (lcdIndex == 3)
      {
       String weightReadings = getWeightReadings();
       notifyClients(weightReadings);
       lcd.setCursor(0, 0);
       lcd.print("Scale network:");
       lcd.setCursor(0, 1);
       lcd.print(soft_ap_ssid);
       lcd.setCursor(0, 2);
       lcd.print("Network password:");
       lcd.setCursor(0, 3);
       lcd.print(soft_ap_password);
     }
      else if (lcdIndex == 4)
      {
       String weightReadings = getWeightReadings();
       notifyClients(weightReadings);
       lcd.setCursor(0, 0);
       lcd.print("Scale WiFi IP:");
       lcd.setCursor(0, 1);
       lcd.print(WiFi.softAPIP());
       lcd.setCursor(0, 2);
       lcd.print("IP On WiFi:");
       lcd.setCursor(0, 3);
       lcd.print(WiFi.localIP());
      }
 }
      int buttonState = digitalRead(34);

  if (buttonState == HIGH) { // Button is pressed
    if (!buttonPressed) {
      buttonPressed = true;
      buttonPressStartTime = millis(); // Record the time the button was pressed
    }
      else {
      // Check if the button has been pressed for the long press duration
      if (millis() - buttonPressStartTime >= longPressDuration) {
        Serial.println("Long press detected");
        digitalWrite(23, LOW); // Set pin 27 to HIGH
        buttonPressed = false; // Reset the button pressed state to prevent repeated triggering
      }
    }
  }
  else { // Button is not pressed
    buttonPressed = false; // Reset the button pressed state if the button is released
    }
      button1.tick();
      button2.tick();
      ws.cleanupClients();
 }
