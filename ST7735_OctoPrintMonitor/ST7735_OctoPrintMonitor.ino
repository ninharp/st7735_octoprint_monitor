/** The MIT License (MIT)

Copyright (c) 2018 David Payne

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

// Additional Contributions:
/* 15 Jan 2019 : Owen Carter : Add psucontrol option and processing */

 /**********************************************
 * Edit Settings.h for personalization
 ***********************************************/

#include "config.h"

#define VERSION "0.1"

#define HOSTNAME "OctMon-" 
#define CONFIG "/.config"

/* Useful Constants */
#define SECS_PER_MIN  (60UL)
#define SECS_PER_HOUR (3600UL)

/* Useful Macros for getting elapsed time */
#define numberOfSeconds(_time_) (_time_ % SECS_PER_MIN)  
#define numberOfMinutes(_time_) ((_time_ / SECS_PER_MIN) % SECS_PER_MIN) 
#define numberOfHours(_time_) (_time_ / SECS_PER_HOUR)

Adafruit_ST7735 display = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

const int buttonPin = D3;

// Bitcoin Client
BitcoinApiClient bitcoinClient;

ST7735DisplayUi   ui( &display );

void drawProgress(Adafruit_ST7735 *display, int percentage, String label);
void drawOtaProgress(unsigned int, unsigned int);
void drawScreen1(Adafruit_ST7735 *display, ST7735DisplayUiState* state, int16_t x, int16_t y);
void drawScreen2(Adafruit_ST7735 *display, ST7735DisplayUiState* state, int16_t x, int16_t y);
//void drawScreen3(Adafruit_ST7735 *display, ST7735DisplayUiState* state, int16_t x, int16_t y);
void drawHeaderOverlay(Adafruit_ST7735 *display, ST7735DisplayUiState* state);
void drawClock(Adafruit_ST7735 *display, ST7735DisplayUiState* state, int16_t x, int16_t y);
void drawWeather(Adafruit_ST7735 *display, ST7735DisplayUiState* state, int16_t x, int16_t y);
void drawClockHeaderOverlay(Adafruit_ST7735 *display, ST7735DisplayUiState* state);
void drawStringCentered(Adafruit_ST7735 *display, int16_t x, int16_t y, String text);

// Set the number of Frames supported
const int numberOfFrames = 2; //3;
FrameCallback frames[numberOfFrames];
FrameCallback clockFrame[3];
boolean isClockOn = false;

OverlayCallback overlays[] = { drawHeaderOverlay };
OverlayCallback clockOverlay[] = { drawClockHeaderOverlay };
int numberOfOverlays = 1;

// Time 
TimeClient timeClient(UtcOffset);
long lastEpoch = 0;
long firstEpoch = 0;
long displayOffEpoch = 0;
String lastMinute = "xx";
String lastSecond = "xx";
String lastReportStatus = "";
boolean displayOn = true;

// Button Debouncing
int buttonState = LOW;             // the current reading from the input pin
int lastButtonState = HIGH;   // the previous reading from the input pin
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers

// OctoPrint Client
OctoPrintClient printerClient(OctoPrintApiKey, OctoPrintServer, OctoPrintPort, OctoAuthUser, OctoAuthPass, HAS_PSU);
int printerCount = 0;

// Weather Client
OpenWeatherMapClient weatherClient(WeatherApiKey, CityIDs, 1, IS_METRIC, WeatherLanguage);

//declairing prototypes
void configModeCallback (WiFiManager *myWiFiManager);
int8_t getWifiQuality();

ESP8266WebServer server(WEBSERVER_PORT);
ESP8266HTTPUpdateServer serverUpdater;

String WEB_ACTIONS =  "<a class='w3-bar-item w3-button' href='/'><i class='fa fa-home'></i> Home</a>"
                      "<a class='w3-bar-item w3-button' href='/configure'><i class='fa fa-cog'></i> Configure</a>"
                      "<a class='w3-bar-item w3-button' href='/configureweather'><i class='fa fa-cloud'></i> Weather</a>"
                      "<a class='w3-bar-item w3-button' href='/systemreset' onclick='return confirm(\"Do you want to reset to default settings?\")'><i class='fa fa-undo'></i> Reset Settings</a>"
                      "<a class='w3-bar-item w3-button' href='/forgetwifi' onclick='return confirm(\"Do you want to forget to WiFi connection?\")'><i class='fa fa-wifi'></i> Forget WiFi</a>"
                      "<a class='w3-bar-item w3-button' href='/update'><i class='fa fa-wrench'></i> Firmware Update</a>"
                      "<a class='w3-bar-item w3-button' href='https://github.com/Qrome' target='_blank'><i class='fa fa-question-circle'></i> About</a>";
                            
String CHANGE_FORM =  "<form class='w3-container' action='/updateconfig' method='get'><h2>Station Config:</h2>"
                      "<p><label>OctoPrint API Key (get from your server)</label><input class='w3-input w3-border w3-margin-bottom' type='text' name='octoPrintApiKey' value='%OCTOKEY%' maxlength='60'></p>"
                      "<p><label>OctoPrint Host Name (usually octopi)</label><input class='w3-input w3-border w3-margin-bottom' type='text' name='octoPrintHostName' value='%OCTOHOST%' maxlength='60'></p>"
                      "<p><label>OctoPrint Address (do not include http://)</label><input class='w3-input w3-border w3-margin-bottom' type='text' name='octoPrintAddress' value='%OCTOADDRESS%' maxlength='60'></p>"
                      "<p><label>OctoPrint Port</label><input class='w3-input w3-border w3-margin-bottom' type='text' name='octoPrintPort' value='%OCTOPORT%' maxlength='5'  onkeypress='return isNumberKey(event)'></p>"
                      "<p><label>OctoPrint User (only needed if you have haproxy or basic auth turned on)</label><input class='w3-input w3-border w3-margin-bottom' type='text' name='octoUser' value='%OCTOUSER%' maxlength='30'></p>"
                      "<p><label>OctoPrint Password </label><input class='w3-input w3-border w3-margin-bottom' type='password' name='octoPass' value='%OCTOPASS%'></p><hr>"
                      "<p><input name='isClockEnabled' class='w3-check w3-margin-top' type='checkbox' %IS_CLOCK_CHECKED%> Display Clock when printer is off</p>"
                      "<p><input name='is24hour' class='w3-check w3-margin-top' type='checkbox' %IS_24HOUR_CHECKED%> Use 24 Hour Clock (military time)</p>"
                      "<p><input name='invDisp' class='w3-check w3-margin-top' type='checkbox' %IS_INVDISP_CHECKED%> Flip display orientation</p>"
                      "<p><input name='hasPSU' class='w3-check w3-margin-top' type='checkbox' %HAS_PSU_CHECKED%> Use OctoPrint PSU control plugin for clock/blank</p>"
                      "<p>Clock Sync / Weather Refresh (minutes) <select class='w3-option w3-padding' name='refresh'>%OPTIONS%</select></p>";
                      
String THEME_FORM =   "<p>Theme Color <select class='w3-option w3-padding' name='theme'>%THEME_OPTIONS%</select></p>"
                      "<p><label>UTC Time Offset</label><input class='w3-input w3-border w3-margin-bottom' type='text' name='utcoffset' value='%UTCOFFSET%' maxlength='12'></p><hr>"
                      "<p><input name='isBasicAuth' class='w3-check w3-margin-top' type='checkbox' %IS_BASICAUTH_CHECKED%> Use Security Credentials for Configuration Changes</p>"
                      "<p><label>User ID (for this interface)</label><input class='w3-input w3-border w3-margin-bottom' type='text' name='userid' value='%USERID%' maxlength='20'></p>"
                      "<p><label>Password </label><input class='w3-input w3-border w3-margin-bottom' type='password' name='stationpassword' value='%STATIONPASSWORD%'></p>"
                      "<button class='w3-button w3-block w3-grey w3-section w3-padding' type='submit'>Save</button></form>";

String WEATHER_FORM = "<form class='w3-container' action='/updateweatherconfig' method='get'><h2>Weather Config:</h2>"
                      "<p><input name='isWeatherEnabled' class='w3-check w3-margin-top' type='checkbox' %IS_WEATHER_CHECKED%> Display Weather when printer is off</p>"
                      "<label>OpenWeatherMap API Key (get from <a href='https://openweathermap.org/' target='_BLANK'>here</a>)</label>"
                      "<input class='w3-input w3-border w3-margin-bottom' type='text' name='openWeatherMapApiKey' value='%WEATHERKEY%' maxlength='60'>"
                      "<p><label>%CITYNAME1% (<a href='http://openweathermap.org/find' target='_BLANK'><i class='fa fa-search'></i> Search for City ID</a>) "
                      "or full <a href='http://openweathermap.org/help/city_list.txt' target='_BLANK'>city list</a></label>"
                      "<input class='w3-input w3-border w3-margin-bottom' type='text' name='city1' value='%CITY1%' onkeypress='return isNumberKey(event)'></p>"
                      "<p><input name='metric' class='w3-check w3-margin-top' type='checkbox' %METRIC%> Use Metric (Celsius)</p>"
                      "<p>Weather Language <select class='w3-option w3-padding' name='language'>%LANGUAGEOPTIONS%</select></p>"
                      "<button class='w3-button w3-block w3-grey w3-section w3-padding' type='submit'>Save</button></form>"
                      "<script>function isNumberKey(e){var h=e.which?e.which:event.keyCode;return!(h>31&&(h<48||h>57))}</script>";

String LANG_OPTIONS = "<option>ar</option>"
                      "<option>bg</option>"
                      "<option>ca</option>"
                      "<option>cz</option>"
                      "<option>de</option>"
                      "<option>el</option>"
                      "<option>en</option>"
                      "<option>fa</option>"
                      "<option>fi</option>"
                      "<option>fr</option>"
                      "<option>gl</option>"
                      "<option>hr</option>"
                      "<option>hu</option>"
                      "<option>it</option>"
                      "<option>ja</option>"
                      "<option>kr</option>"
                      "<option>la</option>"
                      "<option>lt</option>"
                      "<option>mk</option>"
                      "<option>nl</option>"
                      "<option>pl</option>"
                      "<option>pt</option>"
                      "<option>ro</option>"
                      "<option>ru</option>"
                      "<option>se</option>"
                      "<option>sk</option>"
                      "<option>sl</option>"
                      "<option>es</option>"
                      "<option>tr</option>"
                      "<option>ua</option>"
                      "<option>vi</option>"
                      "<option>zh_cn</option>"
                      "<option>zh_tw</option>";

String COLOR_THEMES = "<option>red</option>"
                      "<option>pink</option>"
                      "<option>purple</option>"
                      "<option>deep-purple</option>"
                      "<option>indigo</option>"
                      "<option>blue</option>"
                      "<option>light-blue</option>"
                      "<option>cyan</option>"
                      "<option>teal</option>"
                      "<option>green</option>"
                      "<option>light-green</option>"
                      "<option>lime</option>"
                      "<option>khaki</option>"
                      "<option>yellow</option>"
                      "<option>amber</option>"
                      "<option>orange</option>"
                      "<option>deep-orange</option>"
                      "<option>blue-grey</option>"
                      "<option>brown</option>"
                      "<option>grey</option>"
                      "<option>dark-grey</option>"
                      "<option>black</option>"
                      "<option>w3schools</option>";
                            

void setup() {
  Serial.begin(115200);
  SPIFFS.begin();
  delay(10);
  
  //New Line to clear from start garbage
  Serial.println();
  
  // Initialize digital pin for LED (little blue light on the Wemos D1 Mini)
  pinMode(externalLight, OUTPUT);

  // Button
  pinMode(buttonPin, INPUT);

  readSettings();

  // Initialize display
  display.initR(INITR_144GREENTAB); // Init ST7735R chip, green tab
  digitalWrite(TFT_LED, HIGH); // Put on Backlight LED of TFT
  Serial.println(F("TFT Initialized"));

  display.setTextWrap(false);
  display.fillScreen(ST77XX_BLACK);
  //display.setCursor(20, 60);
  display.setFont(&FreeSansBold12pt7b);
  drawStringCentered(&display, 0, 60, "#nin3d");
  display.setFont(&FreeSans9pt7b);
  drawStringCentered(&display, 0, 90, "Version "+String(VERSION));
  
  delay(1000);
   
  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  
  // Uncomment for testing wifi manager
  //wifiManager.resetSettings();
  wifiManager.setAPCallback(configModeCallback);
  
  String hostname(HOSTNAME);
  hostname += String(ESP.getChipId(), HEX);
  if (!wifiManager.autoConnect((const char *)hostname.c_str())) {// new addition
    delay(3000);
    WiFi.disconnect(true);
    ESP.reset();
    delay(5000);
  }
  
  // You can change the transition that is used
  // SLIDE_LEFT, SLIDE_RIGHT, SLIDE_TOP, SLIDE_DOWN
  ui.setFrameAnimation(SLIDE_RIGHT);
  ui.setTargetFPS(1);
  ui.disableAllIndicators();
  ui.setFrames(frames, (numberOfFrames));
  frames[0] = drawScreen1; // Tempscreen
  frames[1] = drawScreen2; // Timescreen
  //frames[2] = drawScreen3;
  clockFrame[0] = drawClock;
  clockFrame[1] = drawWeather;
  clockFrame[2] = drawBitcoin;
  ui.setOverlays(overlays, numberOfOverlays);
  
  // Inital UI takes care of initalising the display too.
  ui.init();
  if (INVERT_DISPLAY) {
    display.setRotation(2);  //connections at top of OLED display
  }
  
  // print the received signal strength:
  Serial.print("Signal Strength (RSSI): ");
  Serial.print(getWifiQuality());
  Serial.println("%");

  if (ENABLE_OTA) {
    ArduinoOTA.onStart([]() {
      Serial.println("Start");
    });
    ArduinoOTA.onEnd([]() {
      Serial.println("\nEnd");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });
    ArduinoOTA.setHostname((const char *)hostname.c_str()); 
    if (OTA_Password != "") {
      ArduinoOTA.setPassword(((const char *)OTA_Password.c_str()));
    }
    ArduinoOTA.begin();
  }

  if (WEBSERVER_ENABLED) {
    server.on("/", displayPrinterStatus);
    server.on("/systemreset", handleSystemReset);
    server.on("/forgetwifi", handleWifiReset);
    server.on("/updateconfig", handleUpdateConfig);
    server.on("/updateweatherconfig", handleUpdateWeather);
    server.on("/configure", handleConfigure);
    server.on("/configureweather", handleWeatherConfigure);
    server.onNotFound(redirectHome);
    serverUpdater.setup(&server, "/update", www_username, www_password);
    // Start the server
    server.begin();
    Serial.println("Server started");
    // Print the IP address
    String webAddress = "http://" + WiFi.localIP().toString() + ":" + String(WEBSERVER_PORT) + "/";
    Serial.println("Use this URL : " + webAddress);
    display.setTextWrap(false);
    display.fillScreen(ST77XX_BLACK);
    display.setTextColor(ST77XX_WHITE);
    display.setFont();
    drawStringCentered(&display, 0, 30, "Web Interface On");
    drawStringCentered(&display, 0, 50, "You May Connect to IP");
    display.setTextColor(ST77XX_YELLOW);
    drawStringCentered(&display, 0, 70, WiFi.localIP().toString());
    drawStringCentered(&display, 0, 86, "Port: " + String(WEBSERVER_PORT));
    display.setTextColor(ST77XX_WHITE);
  } else {
    Serial.println("Web Interface is Disabled");
    display.setTextWrap(false);
    display.fillScreen(ST77XX_BLACK);
    display.setTextColor(ST77XX_WHITE);
    display.setTextSize(1);
    display.setFont();
    drawStringCentered(&display, 0, 50, "Web Interface is Off");
    drawStringCentered(&display, 0, 70, "Enable in config.h");
  }
  flashLED(5, 500);
  findMDNS();  //go find Octoprint Server by the hostname
  Serial.println("*** Leaving setup()");
}

void findMDNS() {
  if (OctoPrintHostName == "" || ENABLE_OTA == false) {
    return; // nothing to do here
  }
  // We now query our network for 'web servers' service
  // over tcp, and get the number of available devices
  int n = MDNS.queryService("http", "tcp");
  if (n == 0) {
    Serial.println("no services found - make sure OctoPrint server is turned on");
    return;
  }
  Serial.println("*** Looking for " + OctoPrintHostName + " over mDNS");
  for (int i = 0; i < n; ++i) {
    // Going through every available service,
    // we're searching for the one whose hostname
    // matches what we want, and then get its IP
    Serial.println("Found: " + MDNS.hostname(i));
    if (MDNS.hostname(i) == OctoPrintHostName) {
      IPAddress serverIp = MDNS.IP(i);
      OctoPrintServer = serverIp.toString();
      OctoPrintPort = MDNS.port(i); // save the port
      Serial.println("*** Found OctoPrint Server " + OctoPrintHostName + " http://" + OctoPrintServer + ":" + OctoPrintPort);
      writeSettings(); // update the settings
    }
  }
}

//************************************************************
// Main Looop
//************************************************************
void loop() {
  
   //Get Time Update
  if((getMinutesFromLastRefresh() >= minutesBetweenDataRefresh) || lastEpoch == 0) {
    getUpdateTime();
  }

  if (lastMinute != timeClient.getMinutes() && !printerClient.isPrinting()) {
    // Check status every 60 seconds
    digitalWrite(externalLight, LOW);
    lastMinute = timeClient.getMinutes(); // reset the check value
    printerClient.getPrinterJobResults();
    printerClient.getPrinterPsuState();
    digitalWrite(externalLight, HIGH);

    //also update bitcoin heror new time if clause?e?
    bitcoinClient.updateBitcoinData(BitcoinCurrencyCode);
  } else if (printerClient.isPrinting()) {
    if (lastSecond != timeClient.getSeconds() && timeClient.getSeconds().endsWith("0")) {
      lastSecond = timeClient.getSeconds();
      // every 10 seconds while printing get an update
      digitalWrite(externalLight, LOW);
      printerClient.getPrinterJobResults();
      printerClient.getPrinterPsuState();
      digitalWrite(externalLight, HIGH);
    }
  }

  int reading = digitalRead(buttonPin);

  if (reading != lastButtonState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    // whatever the reading is at, it's been there for longer than the debounce
    // delay, so take it as the actual current state:

    if (reading != buttonState) {
      buttonState = reading;

      if (buttonState == LOW) {
        ui.nextFrame();
        //Serial.println("Button pressed!");
      }
    }
  }

  // save the reading. Next time through the loop, it'll be the lastButtonState:
  lastButtonState = reading;

  checkDisplay(); // Check to see if the printer is on or offline and change display.

  ui.update();

  if (WEBSERVER_ENABLED) {
    server.handleClient();
  }
  if (ENABLE_OTA) {
    ArduinoOTA.handle();
  }
}

void getUpdateTime() {
  digitalWrite(externalLight, LOW); // turn on the LED
  Serial.println();

  if (displayOn && DISPLAYWEATHER) {
    Serial.println("Getting Weather Data...");
    weatherClient.updateWeather();
  }

  Serial.println("Updating Time...");
  //Update the Time
  timeClient.updateTime();
  lastEpoch = timeClient.getCurrentEpoch();
  Serial.println("Local time: " + timeClient.getAmPmFormattedTime());

  digitalWrite(externalLight, HIGH);  // turn off the LED
}

boolean authentication() {
  if (IS_BASIC_AUTH && (strlen(www_username) >= 1 && strlen(www_password) >= 1)) {
    return server.authenticate(www_username, www_password);
  } 
  return true; // Authentication not required
}

void handleSystemReset() {
  if (!authentication()) {
    return server.requestAuthentication();
  }
  Serial.println("Reset System Configuration");
  if (SPIFFS.remove(CONFIG)) {
    redirectHome();
    ESP.restart();
  }
}

void handleUpdateWeather() {
  if (!authentication()) {
    return server.requestAuthentication();
  }
  DISPLAYWEATHER = server.hasArg("isWeatherEnabled");
  WeatherApiKey = server.arg("openWeatherMapApiKey");
  CityIDs[0] = server.arg("city1").toInt();
  IS_METRIC = server.hasArg("metric");
  WeatherLanguage = server.arg("language");
  writeSettings();
  isClockOn = false; // this will force a check for the display
  checkDisplay();
  lastEpoch = 0;
  redirectHome();
}

void handleUpdateConfig() {
  boolean flipOld = INVERT_DISPLAY;
  if (!authentication()) {
    return server.requestAuthentication();
  }
  OctoPrintApiKey = server.arg("octoPrintApiKey");
  OctoPrintHostName = server.arg("octoPrintHostName");
  OctoPrintServer = server.arg("octoPrintAddress");
  OctoPrintPort = server.arg("octoPrintPort").toInt();
  OctoAuthUser = server.arg("octoUser");
  OctoAuthPass = server.arg("octoPass");
  DISPLAYCLOCK = server.hasArg("isClockEnabled");
  IS_24HOUR = server.hasArg("is24hour");
  INVERT_DISPLAY = server.hasArg("invDisp");
  HAS_PSU = server.hasArg("hasPSU");
  minutesBetweenDataRefresh = server.arg("refresh").toInt();
  themeColor = server.arg("theme");
  UtcOffset = server.arg("utcoffset").toFloat();
  String temp = server.arg("userid");
  temp.toCharArray(www_username, sizeof(temp));
  temp = server.arg("stationpassword");
  temp.toCharArray(www_password, sizeof(temp));
  writeSettings();
  findMDNS();
  printerClient.getPrinterJobResults();
  printerClient.getPrinterPsuState();
  if (INVERT_DISPLAY != flipOld) {
    ui.init();
    if(INVERT_DISPLAY)     
      display.setRotation(2);
    ui.update();
  }
  checkDisplay();
  lastEpoch = 0;
  redirectHome();
}

void handleWifiReset() {
  if (!authentication()) {
    return server.requestAuthentication();
  }
  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  redirectHome();
  WiFiManager wifiManager;
  wifiManager.resetSettings();
  ESP.restart();
}

void handleWeatherConfigure() {
  if (!authentication()) {
    return server.requestAuthentication();
  }
  digitalWrite(externalLight, LOW);
  String html = "";

  server.sendHeader("Cache-Control", "no-cache, no-store");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");

  html = getHeader();
  server.sendContent(html);
  
  String form = WEATHER_FORM;
  String isWeatherChecked = "";
  if (DISPLAYWEATHER) {
    isWeatherChecked = "checked='checked'";
  }
  form.replace("%IS_WEATHER_CHECKED%", isWeatherChecked);
  form.replace("%WEATHERKEY%", WeatherApiKey);
  form.replace("%CITYNAME1%", weatherClient.getCity(0));
  form.replace("%CITY1%", String(CityIDs[0]));
  String checked = "";
  if (IS_METRIC) {
    checked = "checked='checked'";
  }
  form.replace("%METRIC%", checked);
  String options = LANG_OPTIONS;
  options.replace(">"+String(WeatherLanguage)+"<", " selected>"+String(WeatherLanguage)+"<");
  form.replace("%LANGUAGEOPTIONS%", options);
  server.sendContent(form);
  
  html = getFooter();
  server.sendContent(html);
  server.sendContent("");
  server.client().stop();
  digitalWrite(externalLight, HIGH);
}

void handleConfigure() {
  if (!authentication()) {
    return server.requestAuthentication();
  }
  digitalWrite(externalLight, LOW);
  String html = "";

  server.sendHeader("Cache-Control", "no-cache, no-store");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");

  html = getHeader();
  server.sendContent(html);
  
  String form = CHANGE_FORM;
  
  form.replace("%OCTOKEY%", OctoPrintApiKey);
  form.replace("%OCTOHOST%", OctoPrintHostName);
  form.replace("%OCTOADDRESS%", OctoPrintServer);
  form.replace("%OCTOPORT%", String(OctoPrintPort));
  form.replace("%OCTOUSER%", OctoAuthUser);
  form.replace("%OCTOPASS%", OctoAuthPass);
  String isClockChecked = "";
  if (DISPLAYCLOCK) {
    isClockChecked = "checked='checked'";
  }
  form.replace("%IS_CLOCK_CHECKED%", isClockChecked);
  String is24hourChecked = "";
  if (IS_24HOUR) {
    is24hourChecked = "checked='checked'";
  }
  form.replace("%IS_24HOUR_CHECKED%", is24hourChecked);
  String isInvDisp = "";
  if (INVERT_DISPLAY) {
    isInvDisp = "checked='checked'";
  }
  form.replace("%IS_INVDISP_CHECKED%", isInvDisp);
  String hasPSUchecked = "";
  if (HAS_PSU) {
    hasPSUchecked = "checked='checked'";
  }
  form.replace("%HAS_PSU_CHECKED%", hasPSUchecked);
  
  String options = "<option>10</option><option>15</option><option>20</option><option>30</option><option>60</option>";
  options.replace(">"+String(minutesBetweenDataRefresh)+"<", " selected>"+String(minutesBetweenDataRefresh)+"<");
  form.replace("%OPTIONS%", options);

  server.sendContent(form);

  form = THEME_FORM;
  
  String themeOptions = COLOR_THEMES;
  themeOptions.replace(">"+String(themeColor)+"<", " selected>"+String(themeColor)+"<");
  form.replace("%THEME_OPTIONS%", themeOptions);
  form.replace("%UTCOFFSET%", String(UtcOffset));
  String isUseSecurityChecked = "";
  if (IS_BASIC_AUTH) {
    isUseSecurityChecked = "checked='checked'";
  }
  form.replace("%IS_BASICAUTH_CHECKED%", isUseSecurityChecked);
  form.replace("%USERID%", String(www_username));
  form.replace("%STATIONPASSWORD%", String(www_password));

  server.sendContent(form);
  
  html = getFooter();
  server.sendContent(html);
  server.sendContent("");
  server.client().stop();
  digitalWrite(externalLight, HIGH);
}

void displayMessage(String message) {
  digitalWrite(externalLight, LOW);

  server.sendHeader("Cache-Control", "no-cache, no-store");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");
  String html = getHeader();
  server.sendContent(String(html));
  server.sendContent(String(message));
  html = getFooter();
  server.sendContent(String(html));
  server.sendContent("");
  server.client().stop();
  
  digitalWrite(externalLight, HIGH);
}

void redirectHome() {
  // Send them back to the Root Directory
  server.sendHeader("Location", String("/"), true);
  server.sendHeader("Cache-Control", "no-cache, no-store");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.send(302, "text/plain", "");
  server.client().stop();
}

String getHeader() {
  return getHeader(false);
}

String getHeader(boolean refresh) {
  String menu = WEB_ACTIONS;

  String html = "<!DOCTYPE HTML>";
  html += "<html><head><title>Printer Monitor</title><link rel='icon' href='data:;base64,='>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  if (refresh) {
    html += "<meta http-equiv=\"refresh\" content=\"30\">";
  }
  html += "<link rel='stylesheet' href='https://www.w3schools.com/w3css/4/w3.css'>";
  html += "<link rel='stylesheet' href='https://www.w3schools.com/lib/w3-theme-" + themeColor + ".css'>";
  html += "<link rel='stylesheet' href='https://cdnjs.cloudflare.com/ajax/libs/font-awesome/4.7.0/css/font-awesome.min.css'>";
  html += "</head><body>";
  html += "<nav class='w3-sidebar w3-bar-block w3-card' style='margin-top:88px' id='mySidebar'>";
  html += "<div class='w3-container w3-theme-d2'>";
  html += "<span onclick='closeSidebar()' class='w3-button w3-display-topright w3-large'><i class='fa fa-times'></i></span>";
  html += "<div class='w3-cell w3-left w3-xxxlarge' style='width:60px'><i class='fa fa-cube'></i></div>";
  html += "<div class='w3-padding'>Menu</div></div>";
  html += menu;
  html += "</nav>";
  html += "<header class='w3-top w3-bar w3-theme'><button class='w3-bar-item w3-button w3-xxxlarge w3-hover-theme' onclick='openSidebar()'><i class='fa fa-bars'></i></button><h2 class='w3-bar-item'>Printer Monitor</h2></header>";
  html += "<script>";
  html += "function openSidebar(){document.getElementById('mySidebar').style.display='block'}function closeSidebar(){document.getElementById('mySidebar').style.display='none'}closeSidebar();";
  html += "</script>";
  html += "<br><div class='w3-container w3-large' style='margin-top:88px'>";
  return html;
}

String getFooter() {
  int8_t rssi = getWifiQuality();
  Serial.print("Signal Strength (RSSI): ");
  Serial.print(rssi);
  Serial.println("%");
  String html = "<br><br><br>";
  html += "</div>";
  html += "<footer class='w3-container w3-bottom w3-theme w3-margin-top'>";
  if (lastReportStatus != "") {
    html += "<i class='fa fa-external-link'></i> Report Status: " + lastReportStatus + "<br>";
  }
  html += "<i class='fa fa-paper-plane-o'></i> Version: " + String(VERSION) + "<br>";
  html += "<i class='fa fa-rss'></i> Signal Strength: ";
  html += String(rssi) + "%";
  html += "</footer>";
  html += "</body></html>";
  return html;
}

void displayPrinterStatus() {
  digitalWrite(externalLight, LOW);
  String html = "";

  server.sendHeader("Cache-Control", "no-cache, no-store");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");
  server.sendContent(String(getHeader(true)));

  String displayTime = timeClient.getAmPmHours() + ":" + timeClient.getMinutes() + ":" + timeClient.getSeconds() + " " + timeClient.getAmPm();
  if (IS_24HOUR) {
    displayTime = timeClient.getHours() + ":" + timeClient.getMinutes() + ":" + timeClient.getSeconds();
  }
  
  html += "<div class='w3-cell-row' style='width:100%'><h2>Time: " + displayTime + "</h2></div><div class='w3-cell-row'>";
  html += "<div class='w3-cell w3-container' style='width:100%'><p>";
  html += "Host Name: " + OctoPrintHostName + "<br>";
  if (printerClient.getError() != "") {
    html += "Status: Offline<br>";
    html += "Reason: " + printerClient.getError() + "<br>";
  } else {
    html += "Status: " + printerClient.getState();
    if (printerClient.isPSUoff() && HAS_PSU) {  
      html += ", PSU off";
    }
    html += "<br>";
  }
  
  if (printerClient.isPrinting()) {
    html += "File: " + printerClient.getFileName() + "<br>";
    float fileSize = printerClient.getFileSize().toFloat();
    if (fileSize > 0) {
      fileSize = fileSize / 1024;
      html += "File Size: " + String(fileSize) + "KB<br>";
    }
    int filamentLength = printerClient.getFilamentLength().toInt();
    if (filamentLength > 0) {
      float fLength = float(filamentLength) / 1000;
      html += "Filament: " + String(fLength) + "m<br>";
    }
  
    html += "Tool Temperature: " + printerClient.getTempToolActual() + "&#176; C<br>";
    if ( printerClient.getTempBedActual() != 0 ) {
        html += "Bed Temperature: " + printerClient.getTempBedActual() + "&#176; C<br>";
    }
    
    int val = printerClient.getProgressPrintTimeLeft().toInt();
    int hours = numberOfHours(val);
    int minutes = numberOfMinutes(val);
    int seconds = numberOfSeconds(val);
    html += "Est. Print Time Left: " + zeroPad(hours) + ":" + zeroPad(minutes) + ":" + zeroPad(seconds) + "<br>";
  
    val = printerClient.getProgressPrintTime().toInt();
    hours = numberOfHours(val);
    minutes = numberOfMinutes(val);
    seconds = numberOfSeconds(val);
    html += "Printing Time: " + zeroPad(hours) + ":" + zeroPad(minutes) + ":" + zeroPad(seconds) + "<br>";
    html += "<style>#myProgress {width: 100%;background-color: #ddd;}#myBar {width: " + printerClient.getProgressCompletion() + "%;height: 30px;background-color: #4CAF50;}</style>";
    html += "<div id=\"myProgress\"><div id=\"myBar\" class=\"w3-medium w3-center\">" + printerClient.getProgressCompletion() + "%</div></div>";
  } else {
    html += "<hr>";
  }

  html += "</p></div></div>";

  server.sendContent(html); // spit out what we got
  html = "";

  if (DISPLAYWEATHER) {
    if (weatherClient.getCity(0) == "") {
      html += "<p>Please <a href='/configureweather'>Configure Weather</a> API</p>";
      if (weatherClient.getError() != "") {
        html += "<p>Weather Error: <strong>" + weatherClient.getError() + "</strong></p>";
      }
    } else {
      html += "<div class='w3-cell-row' style='width:100%'><h2>" + weatherClient.getCity(0) + ", " + weatherClient.getCountry(0) + "</h2></div><div class='w3-cell-row'>";
      html += "<div class='w3-cell w3-left w3-medium' style='width:120px'>";
      html += "<img src='http://openweathermap.org/img/w/" + weatherClient.getIcon(0) + ".png' alt='" + weatherClient.getDescription(0) + "'><br>";
      html += weatherClient.getHumidity(0) + "% Humidity<br>";
      html += weatherClient.getWind(0) + " <span class='w3-tiny'>" + getSpeedSymbol() + "</span> Wind<br>";
      html += "</div>";
      html += "<div class='w3-cell w3-container' style='width:100%'><p>";
      html += weatherClient.getCondition(0) + " (" + weatherClient.getDescription(0) + ")<br>";
      html += weatherClient.getTempRounded(0) + getTempSymbol(true) + "<br>";
      html += "<a href='https://www.google.com/maps/@" + weatherClient.getLat(0) + "," + weatherClient.getLon(0) + ",10000m/data=!3m1!1e3' target='_BLANK'><i class='fa fa-map-marker' style='color:red'></i> Map It!</a><br>";
      html += "</p></div></div>";
    }
    
    server.sendContent(html); // spit out what we got
    html = ""; // fresh start
  }

  server.sendContent(String(getFooter()));
  server.sendContent("");
  server.client().stop();
  digitalWrite(externalLight, HIGH);
}

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());

  
  display.setTextWrap(false);
  display.fillScreen(ST77XX_BLACK);
  display.setTextColor(ST77XX_WHITE);
  display.setTextSize(1);
  display.setFont();
  drawStringCentered(&display, 0, 30, "Wifi Manager");
  drawStringCentered(&display, 0, 40, "Please connect to AP");
  display.setTextColor(ST77XX_YELLOW);
  drawStringCentered(&display, 0, 58, myWiFiManager->getConfigPortalSSID());
  display.setTextColor(ST77XX_WHITE);
  drawStringCentered(&display, 0, 76, "To setup Wifi");
  drawStringCentered(&display, 0, 86, "connection");
  
  Serial.println("Wifi Manager");
  Serial.println("Please connect to AP");
  Serial.println(myWiFiManager->getConfigPortalSSID());
  Serial.println("To setup Wifi Configuration");
  flashLED(20, 50);
}

void flashLED(int number, int delayTime) {
  for (int inx = 0; inx < number; inx++) {
      delay(delayTime);
      digitalWrite(externalLight, LOW);
      delay(delayTime);
      digitalWrite(externalLight, HIGH);
      delay(delayTime);
  }
}

void drawScreen1(Adafruit_ST7735 *display, ST7735DisplayUiState* state, int16_t x, int16_t y) {
  String bed_act = printerClient.getValueRounded(printerClient.getTempBedActual());
  String tool_act = printerClient.getValueRounded(printerClient.getTempToolActual());
  String bed_tgt = printerClient.getValueRounded(printerClient.getTempBedTarget());
  String tool_tgt = printerClient.getValueRounded(printerClient.getTempToolTarget());
  
  display->setFont();
  display->setTextSize(1);
  if (bed_act != "0") {
    drawStringCentered(display, x, 4 + y, "Temperature");
  } else {
    drawStringCentered(display, x, 4 + y, "Tool Temp");
  }
  
  display->setFont(&FreeSans12pt7b);
  if (bed_act != "0") {
    display->drawBitmap(12, 46, nozzle_bits, nozzle_width, nozzle_height, ST77XX_WHITE);
    display->drawBitmap(64, 46, bed_bits, bed_width, bed_height, ST77XX_WHITE);
    
    // soll
    display->setCursor(7 + x, 40 + y);
    display->println(tool_act);
    display->setCursor(78 + x, 40 + y);
    display->println(bed_tgt);

    // ist
    display->setCursor(7 + x, 104 + y);
    display->println(tool_tgt);
    display->setCursor(78 + x, 104 + y);
    display->println(bed_act);
  } else {
    display->drawBitmap(x + (display->width() - nozzle_width) / 2, 46, nozzle_bits, nozzle_width, nozzle_height, ST77XX_WHITE);
    drawStringCentered(display, x, 40 + y, tool_tgt);
    drawStringCentered(display, x, 104 + y, tool_act);
  }
}

void drawScreen2(Adafruit_ST7735 *display, ST7735DisplayUiState* state, int16_t x, int16_t y) {
  display->setFont();
  display->setTextSize(1);
  drawStringCentered(display, x, 4 + y, "Time Overview");

  display->setFont(&FreeSans9pt7b);
  drawStringCentered(display, x, 34 + y, "Elapsed");
  drawStringCentered(display, x, 80 + y, "Remaining");

  display->setFont(&FreeSansBold12pt7b);
  
  int val = printerClient.getProgressPrintTime().toInt();
  int hours = numberOfHours(val);
  int minutes = numberOfMinutes(val);
  int seconds = numberOfSeconds(val);

  String time = zeroPad(hours) + ":" + zeroPad(minutes) + ":" + zeroPad(seconds);
  drawStringCentered(display, x, 56 + y, time);

  val = printerClient.getProgressPrintTimeLeft().toInt();
  hours = numberOfHours(val);
  minutes = numberOfMinutes(val);
  seconds = numberOfSeconds(val);

  time = zeroPad(hours) + ":" + zeroPad(minutes) + ":" + zeroPad(seconds);
  drawStringCentered(display, x, 104 + y, time);
}

void drawClock(Adafruit_ST7735 *display, ST7735DisplayUiState* state, int16_t x, int16_t y) {
  
  String displayTime = timeClient.getAmPmHours() + ":" + timeClient.getMinutes() + ":" + timeClient.getSeconds();
  if (IS_24HOUR) {
    displayTime = timeClient.getHours() + ":" + timeClient.getMinutes() + ":" + timeClient.getSeconds(); 
  }

  display->setFont(&FreeSansBold12pt7b);
  drawStringCentered(display, x, y + (display->height() + 12) / 2, displayTime);
}

void drawBitcoin(Adafruit_ST7735 *display, ST7735DisplayUiState* state, int16_t x, int16_t y) {
  display->drawBitmap(x + 2, 36, bitcoin_bits, bitcoin_width, bitcoin_height, ST77XX_WHITE);
  if (BitcoinCurrencyCode != "NONE" && BitcoinCurrencyCode != "") {
    String btc = bitcoinClient.getRate() + " " + bitcoinClient.getCode();
    display->setFont();
    display->setTextSize(1);
    //display->setFont(Roboto_14);
    display->setCursor(52 + x, 50 + y);
    display->println("Bitcoin BTC");
    //display->setFont(Roboto_14);
    display->setCursor(52 + x, 67 + y);
    display->println(btc);
  }
}

void drawWeather(Adafruit_ST7735 *display, ST7735DisplayUiState* state, int16_t x, int16_t y) {
  display->setFont();
  display->setTextSize(2);
  display->setCursor(4 + x, 42 + y);
  display->println(weatherClient.getTempRounded(0) + getTempSymbol());
  //display.drawChar(34 + x, 38 + y, , ST77XX_WHITE, ST77XX_BLACK, 2);

  display->setTextSize(1);
  display->setCursor(4 + x, 68 + y);
  display->println(weatherClient.getCondition(0));

  display->setFont(&Meteocons_Regular_42);
  display->setCursor(70 + x, 80 + y);
  display->println(weatherClient.getWeatherIcon(0));
  display->setFont();
}

String getTempSymbol() {
  return getTempSymbol(false);
}

String getTempSymbol(boolean forHTML) {
  String rtnValue = "F";
  if (IS_METRIC) {
    rtnValue = "C";
  }
  if (forHTML) {
    rtnValue = "&#176;" + rtnValue;
  } else {
    rtnValue = char((unsigned char)(247)) + rtnValue;
  }
  return rtnValue;
}

String getSpeedSymbol() {
  String rtnValue = "mph";
  if (IS_METRIC) {
    rtnValue = "kph";
  }
  return rtnValue;
}

String zeroPad(int value) {
  String rtnValue = String(value);
  if (value < 10) {
    rtnValue = "0" + rtnValue;
  }
  return rtnValue;
}

void drawHeaderOverlay(Adafruit_ST7735 *display, ST7735DisplayUiState* state) {
  
  String displayTime = timeClient.getAmPmHours() + ":" + timeClient.getMinutes();
  if (IS_24HOUR) {
    displayTime = timeClient.getHours() + ":" + timeClient.getMinutes();
  }
  display->setTextColor(ST77XX_WHITE);
  display->setFont();
  display->setTextSize(1);
  display->setCursor(1, 116);
  display->println(displayTime);
  
  if (!IS_24HOUR) {
    String ampm = timeClient.getAmPm();
    display->setCursor(32, 116);
    display->println(ampm);
  }

  // Draw top horizontal line
  display->drawRect(0, 17, 128, 1, ST77XX_WHITE);
  
  display->fillRect(58, 116, 25, 10, ST77XX_WHITE);
  display->setTextSize(1);
  String percent = String(printerClient.getProgressCompletion()) + "%";
  display->setCursor(58,116);
  display->println(percent);
  
  // Draw indicator to show next update
  int updatePos = (printerClient.getProgressCompletion().toFloat() / float(100)) * 128;
  display->drawRect(0, 105, 128, 7, ST77XX_WHITE);
  display->fillRect(1, 106, updatePos, 5, ST77XX_WHITE);

  // Draw bottom horizontal line
  display->drawRect(0, 111, 128, 1, ST77XX_WHITE);
  
  drawRssi(display);
}

void drawClockHeaderOverlay(Adafruit_ST7735 *display, ST7735DisplayUiState* state) {
  display->setTextColor(ST77XX_WHITE);
  display->setFont();
  display->setTextSize(1);
  drawStringCentered(display, 0, 4, OctoPrintHostName);
  
  /*
  String displayTime = timeClient.getAmPmHours() + ":" + timeClient.getMinutes();
  if (IS_24HOUR) {
    displayTime = timeClient.getHours() + ":" + timeClient.getMinutes();
  }
  display->setCursor(1, 116);
  display->println(displayTime);
  */
  
  if (!IS_24HOUR) {
    display->setTextSize(1);
    //display->setCursor(32, 116);
    //display->println(timeClient.getAmPm());
    if (printerClient.isPSUoff()) {
      display->setCursor(1, 116);
      display->println("psu off");
    } else {
      display->setCursor(1, 116);
      display->println("offline");
    }
  } else {
    if (printerClient.isPSUoff()) {
      display->setCursor(1, 116);
      display->println("psu off");
    } else {
      display->setCursor(1, 116);
      display->println("offline");
    }
  }

  // Draw bottom horizontal line
  display->drawRect(0, 111, 128, 1, ST77XX_WHITE);
  //display->drawRect(0, 43, 128, 2, ST77XX_WHITE);
 
  drawRssi(display);
}

void drawStringCentered(Adafruit_ST7735 *display, int16_t x, int16_t y, String text) {
  int16_t  x1, y1;
  uint16_t w, h;
  display->getTextBounds(text, x, y, &x1, &y1, &w, &h);
  if (w > display->width())
    display->setCursor(x, y);
  else
    display->setCursor(((display->width()-w)/2)+x, y);
  display->println(text);
}

void drawRssi(Adafruit_ST7735 *display) {
  int8_t quality = getWifiQuality();
  uint32_t color = ST77XX_RED;
  if (quality >= 30)
    color = ST77XX_YELLOW;
  else if (quality >= 60)
    color = ST77XX_GREEN;

  // clear out field of rssi
  //display->fillRect(114, 113, 14, 16, ST77XX_BLACK);
  
  for (int8_t i = 0; i < 4; i++) {
    for (int8_t j = 0; j < 3 * (i + 2); j++) {
      if (quality > i * 25 || j == 0) {
        display->drawPixel(114 + 4 * i, 127 - j, color);
      }
    }
  }
}

// converts the dBm to a range between 0 and 100%
int8_t getWifiQuality() {
  int32_t dbm = WiFi.RSSI();
  if(dbm <= -100) {
      return 0;
  } else if(dbm >= -50) {
      return 100;
  } else {
      return 2 * (dbm + 100);
  }
}


void writeSettings() {
  // Save decoded message to SPIFFS file for playback on power up.
  File f = SPIFFS.open(CONFIG, "w");
  if (!f) {
    Serial.println("File open failed!");
  } else {
    Serial.println("Saving settings now...");
    f.println("UtcOffset=" + String(UtcOffset));
    f.println("octoKey=" + OctoPrintApiKey);
    f.println("octoHost=" + OctoPrintHostName);
    f.println("octoServer=" + OctoPrintServer);
    f.println("octoPort=" + String(OctoPrintPort));
    f.println("octoUser=" + OctoAuthUser);
    f.println("octoPass=" + OctoAuthPass);
    f.println("refreshRate=" + String(minutesBetweenDataRefresh));
    f.println("themeColor=" + themeColor);
    f.println("IS_BASIC_AUTH=" + String(IS_BASIC_AUTH));
    f.println("www_username=" + String(www_username));
    f.println("www_password=" + String(www_password));
    f.println("DISPLAYCLOCK=" + String(DISPLAYCLOCK));
    f.println("is24hour=" + String(IS_24HOUR));
    f.println("invertDisp=" + String(INVERT_DISPLAY));
    f.println("isWeather=" + String(DISPLAYWEATHER));
    f.println("weatherKey=" + WeatherApiKey);
    f.println("CityID=" + String(CityIDs[0]));
    f.println("isMetric=" + String(IS_METRIC));
    f.println("language=" + String(WeatherLanguage));
    f.println("hasPSU=" + String(HAS_PSU));
  }
  f.close();
  readSettings();
  timeClient.setUtcOffset(UtcOffset);
}

void readSettings() {
  if (SPIFFS.exists(CONFIG) == false) {
    Serial.println("Settings File does not yet exists.");
    writeSettings();
    return;
  }
  File fr = SPIFFS.open(CONFIG, "r");
  String line;
  while(fr.available()) {
    line = fr.readStringUntil('\n');

    if (line.indexOf("UtcOffset=") >= 0) {
      UtcOffset = line.substring(line.lastIndexOf("UtcOffset=") + 10).toFloat();
      Serial.println("UtcOffset=" + String(UtcOffset));
    }
    if (line.indexOf("octoKey=") >= 0) {
      OctoPrintApiKey = line.substring(line.lastIndexOf("octoKey=") + 8);
      OctoPrintApiKey.trim();
      Serial.println("OctoPrintApiKey=" + OctoPrintApiKey);
    }
    if (line.indexOf("octoHost=") >= 0) {
      OctoPrintHostName = line.substring(line.lastIndexOf("octoHost=") + 9);
      OctoPrintHostName.trim();
      Serial.println("OctoPrintHostName=" + OctoPrintHostName);
    }
    if (line.indexOf("octoServer=") >= 0) {
      OctoPrintServer = line.substring(line.lastIndexOf("octoServer=") + 11);
      OctoPrintServer.trim();
      Serial.println("OctoPrintServer=" + OctoPrintServer);
    }
    if (line.indexOf("octoPort=") >= 0) {
      OctoPrintPort = line.substring(line.lastIndexOf("octoPort=") + 9).toInt();
      Serial.println("OctoPrintPort=" + String(OctoPrintPort));
    }
    if (line.indexOf("octoUser=") >= 0) {
      OctoAuthUser = line.substring(line.lastIndexOf("octoUser=") + 9);
      OctoAuthUser.trim();
      Serial.println("OctoAuthUser=" + OctoAuthUser);
    }
    if (line.indexOf("octoPass=") >= 0) {
      OctoAuthPass = line.substring(line.lastIndexOf("octoPass=") + 9);
      OctoAuthPass.trim();
      Serial.println("OctoAuthPass=" + OctoAuthPass);
    }
    if (line.indexOf("refreshRate=") >= 0) {
      minutesBetweenDataRefresh = line.substring(line.lastIndexOf("refreshRate=") + 12).toInt();
      Serial.println("minutesBetweenDataRefresh=" + String(minutesBetweenDataRefresh));
    }
    if (line.indexOf("themeColor=") >= 0) {
      themeColor = line.substring(line.lastIndexOf("themeColor=") + 11);
      themeColor.trim();
      Serial.println("themeColor=" + themeColor);
    }
    if (line.indexOf("IS_BASIC_AUTH=") >= 0) {
      IS_BASIC_AUTH = line.substring(line.lastIndexOf("IS_BASIC_AUTH=") + 14).toInt();
      Serial.println("IS_BASIC_AUTH=" + String(IS_BASIC_AUTH));
    }
    if (line.indexOf("www_username=") >= 0) {
      String temp = line.substring(line.lastIndexOf("www_username=") + 13);
      temp.trim();
      temp.toCharArray(www_username, sizeof(temp));
      Serial.println("www_username=" + String(www_username));
    }
    if (line.indexOf("www_password=") >= 0) {
      String temp = line.substring(line.lastIndexOf("www_password=") + 13);
      temp.trim();
      temp.toCharArray(www_password, sizeof(temp));
      Serial.println("www_password=" + String(www_password));
    }
    if (line.indexOf("DISPLAYCLOCK=") >= 0) {
      DISPLAYCLOCK = line.substring(line.lastIndexOf("DISPLAYCLOCK=") + 13).toInt();
      Serial.println("DISPLAYCLOCK=" + String(DISPLAYCLOCK));
    }
    if (line.indexOf("is24hour=") >= 0) {
      IS_24HOUR = line.substring(line.lastIndexOf("is24hour=") + 9).toInt();
      Serial.println("IS_24HOUR=" + String(IS_24HOUR));
    }
    if(line.indexOf("invertDisp=") >= 0) {
      INVERT_DISPLAY = line.substring(line.lastIndexOf("invertDisp=") + 11).toInt();
      Serial.println("INVERT_DISPLAY=" + String(INVERT_DISPLAY));
    }
    if (line.indexOf("hasPSU=") >= 0) {
      HAS_PSU = line.substring(line.lastIndexOf("hasPSU=") + 7).toInt();
      Serial.println("HAS_PSU=" + String(HAS_PSU));
    }
    if (line.indexOf("isWeather=") >= 0) {
      DISPLAYWEATHER = line.substring(line.lastIndexOf("isWeather=") + 10).toInt();
      Serial.println("DISPLAYWEATHER=" + String(DISPLAYWEATHER));
    }
    if (line.indexOf("weatherKey=") >= 0) {
      WeatherApiKey = line.substring(line.lastIndexOf("weatherKey=") + 11);
      WeatherApiKey.trim();
      Serial.println("WeatherApiKey=" + WeatherApiKey);
    }
    if (line.indexOf("CityID=") >= 0) {
      CityIDs[0] = line.substring(line.lastIndexOf("CityID=") + 7).toInt();
      Serial.println("CityID: " + String(CityIDs[0]));
    }
    if (line.indexOf("isMetric=") >= 0) {
      IS_METRIC = line.substring(line.lastIndexOf("isMetric=") + 9).toInt();
      Serial.println("IS_METRIC=" + String(IS_METRIC));
    }
    if (line.indexOf("language=") >= 0) {
      WeatherLanguage = line.substring(line.lastIndexOf("language=") + 9);
      WeatherLanguage.trim();
      Serial.println("WeatherLanguage=" + WeatherLanguage);
    }
  }
  fr.close();
  printerClient.updateOctoPrintClient(OctoPrintApiKey, OctoPrintServer, OctoPrintPort, OctoAuthUser, OctoAuthPass, HAS_PSU);
  weatherClient.updateWeatherApiKey(WeatherApiKey);
  weatherClient.updateLanguage(WeatherLanguage);
  weatherClient.setMetric(IS_METRIC);
  weatherClient.updateCityIdList(CityIDs, 1);
  timeClient.setUtcOffset(UtcOffset);
}

int getMinutesFromLastRefresh() {
  int minutes = (timeClient.getCurrentEpoch() - lastEpoch) / 60;
  return minutes;
}

int getMinutesFromLastDisplay() {
  int minutes = (timeClient.getCurrentEpoch() - displayOffEpoch) / 60;
  return minutes;
}

// Toggle on and off the display if user defined times
void checkDisplay() {
  if (!displayOn && DISPLAYCLOCK) {
    enableDisplay(true);
  }
  if (displayOn && !(printerClient.isOperational() || printerClient.isPrinting()) && !DISPLAYCLOCK) {
    // Put Display to sleep
    display.fillScreen(ST77XX_BLACK);
     
    display.setTextSize(2);
    //display.setTextAlignment(TEXT_ALIGN_CENTER);
    //display.setContrast(255); // default is 255
    display.setCursor(64, 5);
    display.println("Printer Offline\nSleep Mode...");
     
    delay(5000);
    enableDisplay(false);
    Serial.println("Printer is offline going down to sleep...");
    return;    
  } else if (!displayOn && !DISPLAYCLOCK) {
    if (printerClient.isOperational()) {
      // Wake the Screen up
      enableDisplay(true);
      display.fillScreen(ST77XX_BLACK);
       
      display.setTextSize(2);
      display.setCursor(64, 5);
      display.println("Printer Online\nWake up...");
       
      Serial.println("Printer is online waking up...");
      delay(5000);
      return;
    }
  } else if (DISPLAYCLOCK) {
    if ((!printerClient.isOperational() || printerClient.isPSUoff()) && !isClockOn) {
      Serial.println("Clock Mode is turned on.");
      if (!DISPLAYWEATHER) {
        //ui.disableAutoTransition();
        ui.enableAutoTransition();
        ui.setFrames(clockFrame, 2);
        //if bitcoin foo here
        clockFrame[0] = drawClock;
        clockFrame[1] = drawBitcoin;
      } else {
        ui.enableAutoTransition();
        ui.setFrames(clockFrame, 3);
        clockFrame[0] = drawClock;
        clockFrame[1] = drawWeather;
        clockFrame[2] = drawBitcoin;
      }
      ui.setOverlays(clockOverlay, numberOfOverlays);
      isClockOn = true;
    } else if (printerClient.isOperational() && !printerClient.isPSUoff() && isClockOn) {
      Serial.println("Printer Monitor is active.");
      ui.setFrames(frames, numberOfFrames);
      ui.setOverlays(overlays, numberOfOverlays);
      ui.enableAutoTransition();
      isClockOn = false;
    }
  }
}

void enableDisplay(boolean enable) {
  displayOn = enable;
  if (enable) {
    if (getMinutesFromLastDisplay() >= minutesBetweenDataRefresh) {
      // The display has been off longer than the minutes between refresh -- need to get fresh data
      lastEpoch = 0; // this should force a data pull
      displayOffEpoch = 0;  // reset
    }
    //display.displayOn();
    Serial.println("Display was turned ON: " + timeClient.getFormattedTime());
  } else {
    //display.displayOff();
    Serial.println("Display was turned OFF: " + timeClient.getFormattedTime());
    displayOffEpoch = lastEpoch;
  }
}
