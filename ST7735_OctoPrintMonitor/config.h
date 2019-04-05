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

/******************************************************************************
 * Printer Monitor is designed for the NodeMCU 0.9/1.0
 * NodeMCU:  https://amzn.to/2HRklWy
 * 1.44" TFT SPI 128x128 Display (128128) ST7735
 * TFT Display:  https://amzn.to/2FPuGjI
 ******************************************************************************/
/******************************************************************************
 * NOTE: The settings here are the default settings for the first loading.  
 * After loading you will manage changes to the settings via the Web Interface.  
 * If you want to change settings again in the settings.h, you will need to 
 * erase the file system on the Wemos or use the “Reset Settings” option in 
 * the Web Interface.
 ******************************************************************************/

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <ESP8266HTTPUpdateServer.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include <SPI.h>
#include "TimeClient.h"
#include "OctoPrintClient.h"
#include "OpenWeatherMapClient.h"
#include "BitcoinApiClient.h"
#include "FS.h"
#include "ST7735DisplayUi.h"

// OctoPrint Monitoring -- Monitor your 3D printer OctoPrint Server
String OctoPrintApiKey = "E3B9002E35F94EE7B1355E10C3E8763E";   // ApiKey from your User Account on OctoPrint
String OctoPrintHostName = "octopi";// Default 'octopi' -- or hostname if different (optional if your IP changes)
String OctoPrintServer = "192.168.178.86";   // IP or Address of your OctoPrint Server (DO NOT include http://)
int OctoPrintPort = 80;        // the port you are running your OctoPrint server on (usually 80);
String OctoAuthUser = "";      // only used if you have haproxy or basic athentintication turned on (not default)
String OctoAuthPass = "";      // only used with haproxy or basic auth (only needed if you must authenticate)

// Weather Configuration
boolean DISPLAYWEATHER = false; // true = show weather when not printing / false = no weather
String WeatherApiKey = "be48be5d2d1ad12625651a299746e5e8"; // Your API Key from http://openweathermap.org/
// Default City Location (use http://openweathermap.org/find to find city ID)
int CityIDs[] = { 2953234 }; //Only USE ONE for weather marquee
boolean IS_METRIC = true; // false = Imperial and true = Metric
// Languages: ar, bg, ca, cz, de, el, en, fa, fi, fr, gl, hr, hu, it, ja, kr, la, lt, mk, nl, pl, pt, ro, ru, se, sk, sl, es, tr, ua, vi, zh_cn, zh_tw
String WeatherLanguage = "de";  //Default (en) English

// Bitcoin Client - NONE or empty is off
String BitcoinCurrencyCode = "EUR";  // Change to USD, GBD, EUR, or NONE -- this can be managed in the Web Interface

// Webserver
const int WEBSERVER_PORT = 80; // The port you can access this device on over HTTP
const boolean WEBSERVER_ENABLED = true;  // Device will provide a web interface via http://[ip]:[port]/
boolean IS_BASIC_AUTH = true;  // true = require athentication to change configuration settings / false = no auth
char* www_username = "admin";  // User account for the Web Interface
char* www_password = "password";  // Password for the Web Interface

// Date and Time
float UtcOffset = 2; // Hour offset from GMT for your timezone
boolean IS_24HOUR = true;     // 23:00 millitary 24 hour clock
int minutesBetweenDataRefresh = 15;
boolean DISPLAYCLOCK = true;   // true = Show Clock when not printing / false = turn off display when not printing

// Display Settings
const int TFT_CS = D8;
const int TFT_RST = D3; // Or set to -1 and connect to Arduino RESET pin
const int TFT_DC = D2;
const int TFT_LED = D4;
boolean INVERT_DISPLAY = false; // true = pins at top | false = pins at the bottom

// LED Settings
const int externalLight = D1; // Set to unused pin, like D1, to disable use of built-in LED (LED_BUILTIN)

// PSU Control
boolean HAS_PSU = false; // Set to true if https://github.com/kantlivelong/OctoPrint-PSUControl/ in use

// OTA Updates
boolean ENABLE_OTA = true;     // this will allow you to load firmware to the device over WiFi (see OTA for ESP8266)
String OTA_Password = "";      // Set an OTA password here -- leave blank if you don't want to be prompted for password

String themeColor = "light-green"; // this can be changed later in the web interface.