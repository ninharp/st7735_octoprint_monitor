# 1,44" TFT - Printer Monitor (OctoPrint 3D Printer Monitor)

This printer monitor is in general a hardly adapted version of Qrome's Printer Monitor 
for the small OLed 0,96" Displays, i had a bunch of 1,44" TFT's laying around so that 
i decide to make a version for this type of displays.
So the most features are the same as the one's from Qrome's monitor, with some small 
additions. 
I will add some more features at time, the bigger screen size got a little more possibilities :)

[Qrome's Printer Monitor](https://github.com/Qrome/printer-monitor)

## Features:
* Displays the print status from OctoPrint Server
* Estimated time remaining
* Time Printing
* Percentage complete
* Progress bar
* Bed and Tool Temperature
* Screen turns off when printer is turned off or disconnected
* Screen turns on when printer is Operational or connected
* Option to display a clock screen instead of sleep mode
* Option to display 24 hour clock or AM/PM style
* Option to display Current Weather when printer is off
* Option to display Bitcoin Change when in clock mode
* Sample rate is every 60 seconds when not printing
* Sample rate is every 10 seconds when printing
* Fully configurable from the web interface (not required to update Settings.h)
* Supports OTA (loading firmware over WiFi connection on same LAN)
* Basic Authentication to protect your settings
* Version 2.2 added the ability to update firmware through web interface from a compiled binary
* Can query the Octoprint [PSU Control plugin](https://plugins.octoprint.org/plugins/psucontrol/) to enter clock or blank mode when PSU is off

## Required Parts:
* [NodeMCU 1.0 Board](https://amzn.to/2HRklWy)
* [NodeMCU Breakout Board](https://amzn.to/2WH3Jo5)
* [1.44" TFT Display](https://amzn.to/2FPuGjI) (detailed dimensions of display see below)

Note: Using the links provided here help to support these types of projects. Thank you for the support.  

## Wiring for the NodeMCU to the SPI ST7735 TFT
<table border=1 align="center">
    <tr><td><b>Display</b></td><td><b>NodeMCU</b></td><td><b>ESP8266</b></td></tr>
    <tr><td>VCC</td><td>3V</td><td>3V</td></tr>
    <tr><td>LED</td><td>3V</td><td>3V</td></tr>
    <tr><td>GND</td><td>GND</td><td>GND</td></tr>
    <tr><td>RST</td><td>D3</td><td>GPIO0</td></tr>
    <tr><td>RS/DC</td><td>D2</td><td>GPIO4</td></tr>
    <tr><td>SDI</td><td>D7</td><td>GPIO13 (MOSI)</td></tr>
    <tr><td>CLK</td><td>D5</td><td>GPIO14 (CLK)</td></tr>
    <tr><td>CS</td><td>D8</td><td>GPIO15 (CS)</td></tr>
</table>

## 3D Printed Case by ninharp:  
https://www.thingiverse.com/thing:3532451

## Flashing precompiled binary
There is a precompiled binary for NodeMCU with ESP12 in the root of the repository which you can directly flash onto your NodeMCU

Version 2.2 introduced the ability to upgrade pre-compiled firmware from a binary file.  In version 2.3 and on you should find binary files that can be uploaded to your printer monitor via the web interface.  From the main menu in the web interface select "Firmware Update" and follow the prompts.
* **printermonitor.ino.nodemcu_0.9_ST7735.bin** - compiled for NodeMCU 0.9 ESP-12 for 1,44" ST7735 TFT (default)
* **printermonitor.ino.nodemcu_1.0_ST7735.bin** - compiled for NodeMCU 1.0 ESP-12E for 1,44" ST7735 TFT (default)

## Compiling and Loading to Wemos D1 Mini
It is recommended to use Arduino IDE.  You will need to configure Arduino IDE to work with the NodeMCU board and USB port and installed the required USB drivers etc.  
* USB CH340G drivers:  https://github.com/nodemcu/nodemcu-devkit/tree/master/Drivers
* Enter http://arduino.esp8266.com/stable/package_esp8266com_index.json into Additional Board Manager URLs field. You can add multiple URLs, separating them with commas.  This will add support for the NodeMCU to Arduino IDE.
* Open Boards Manager from Tools > Board menu and install esp8266 platform (and don't forget to select your ESP8266 board from Tools > Board menu after installation).
* Select Board:  "NodeMCU 0.9 (ESP-12 Module)" or "NodeMCU 1.0 (ESP-12E Module)"
* Set 1M SPIFFS -- this project uses SPIFFS for saving and reading configuration settings.

## Loading Supporting Library Files in Arduino
Use the Arduino guide for details on how to installing and manage libraries https://www.arduino.cc/en/Guide/Libraries  
**Packages** -- the following packages and libraries are used (download and install):  
ESP8266WiFi.h  
ESP8266WebServer.h   
WiFiManager.h --> https://github.com/tzapu/WiFiManager   
ESP8266mDNS.h  
ArduinoOTA.h  --> Arduino OTA Library   
Adafruit_GFX.h  
Adafruit_ST7735.h   
  
Note Version 2.5 and later include ArduinoJson (version 5.13.1).   

## Initial Configuration
All settings may be managed from the Web Interface, however, you may update the **config.h** file manually -- but it is not required.  There is also an option to display current weather when the print is off-line.  
* Your OctoPrint API Key from your OctoPrint -> User Settings -> Current API Key  
* Optional OpenWeatherMap API Key -- if you want current weather when not printing.  Get the api key from: https://openweathermap.org/  

NOTE: The settings in the config.h are the default settings for the first loading. After loading you will manage changes to the settings via the Web Interface. If you want to change settings again in the config.h, you will need to erase the file system on the NodeMCU or use the “Reset Settings” option in the Web Interface.  

## Web Interface
The Printer Monitor uses the **WiFiManager** so when it can't find the last network it was connected to 
it will become a **AP Hotspot** -- connect to it with your phone and you can then enter your WiFi connection information.

After connected to your WiFi network it will display the IP addressed assigned to it and that can be 
used to open a browser to the Web Interface.  **Everything** can be configured there.

<p align="center">
  <img src="/images/shot_01.png" width="200"/>
  <img src="/images/shot_02.png" width="200"/>
  <img src="/images/shot_03.png" width="200"/>
  <img src="/images/shot_04.png" width="200"/>
</p>

## Contributors
David Payne -- Principal developer and architect  
Daniel Eichhorn -- Author of the TimeClient class and OLEDDisplayUi  
Florian Schütte -- added flip display to web interface  
Owen Carter -- Added psu control setting (v2.4)   
Michael Sauer -- Ported to Adafruit GFX and ST7735 Display  

Contributing to this software is warmly welcomed. You can do this basically by
forking from master, committing modifications and then making a pulling requests to be reviewed (follow the links above
for operating guide).  Detailed comments are encouraged.  Adding change log and your contact into file header is encouraged.
Thanks for your contribution.


