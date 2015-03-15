Inventory for moving timer away from Arduino Due to a custom board.

Initial parts list sourced from here:
http://www.jameco.com/Jameco/workshop/JamecoBuilds/arduinocircuit.html

###PARTS

####Main Board
|name|use|qty|misc|
|:----|:---:|:---:|:---:|
|[ATMega328](https://www.tinkersoup.de/en/arduino/arduino-supplies/atmega328-with-arduino-optiboot-uno/a-1267/)|-|1|-|
|[DIP Socket, 28-pin](https://www.tinkersoup.de/en/components/additional-components/dip-sockets-solder-tail-28-pin-0-3/a-1268/)|-|1|-|
|[DC Barrel Jack](https://www.tinkersoup.de/en/komponenten/kabel-draehte-verbinder/dc-barrel-power-jack-connector/a-1119/)|Board power|1|-|
|[Voltage Regulator, 5V TO-220](https://www.tinkersoup.de/en/components/additional-components/voltage-regulator-a-1-5-a-positive-to-220/a-1257/)|Board power|1|-|
|[External Crystal, 16Mhz, quartz](https://www.tinkersoup.de/en/components/resistors-capacitors-diodes/standard-quartz-fundamental-16-0-mhz/a-1262/)|ATMega clock|1|-|
|[Ceramic Cap, 22pF](https://www.tinkersoup.de/en/components/resistors-capacitors-diodes/ceramic-capacitor-22pf/a-1255/)|Just because|2|-|
|[Electrolytic Cap, 10 µF](https://www.tinkersoup.de/en/components/resistors-capacitors-diodes/electrolytic-capacitor-105-c-ls-2-0mm/a-1263/)|Decoupling|2|-|
|LED, green or red, 5mm standard|Power Status|1|-|
|Resistor, ~220Ohm|Current limiting, Power status LED|1|-|
|[Reset Button](https://www.tinkersoup.de/en/components/mini-push-button-switch/a-552/)|System reset|1|-|
|On/Off Power Switch (TODO: I'm not using currently - need a link to the big one we use)|Kill|1|-|
|[Piezo Buzzer, DC](https://www.tinkersoup.de/en/sound-audio/piezo-buzzer-speaker-a-pc-mount-12mm-2-048khz/a-1227/)|User Feedback|1|-|
|[7-Segment Display (TODO: update to large display?)](https://www.tinkersoup.de/en/lcd-oled-e-paper/7-segment-serial-display-red/a-812/)|Display|1|-|
|Arcade Button (TODO: I'm using mini-switches)|Control, Start/Stop/Reset|2|-|
|[Rotary Pot, Linear, 10k](https://www.tinkersoup.de/en/components/potentiometer/rotary-potentiometer-10k-ohm-linear/a-507/)|Sensor calibration|2|-|
|Voltage Meter, 0-5V, externally powered (TODO: need link)|Sensor calibration|2|-|

####TODO
- method to upload code? (USB / Xbee / socket for ATMega in-and-out)
- add LED + resistor to Pin 13 for basic board testing using blink sketch?

####Sensor Stations

|name|use|qty|misc|
|:----|:---:|:---:|:---:|
|Photo Sensor, CdS (TODO: need link)|Sensors|2|-|

####Xmas Tree

|name|use|qty|misc|
|:----|:---:|:---:|:---:|
|[Shift Register](https://www.tinkersoup.de/en/lcd-oled-e-paper/shift-register-8-bit-74hc595/a-1107/)|Xmas Tree Control|1|-|
|[Piezo Buzzer, DC](https://www.tinkersoup.de/en/sound-audio/piezo-buzzer-speaker-a-pc-mount-12mm-2-048khz/a-1227/)|UI|1|-|
|High-Power LED, green (TODO: need link)|UI|1|-|
|High-Power LED, red (TODO: need link)|UI|3|-|

####TODO
- power distro FETs
- resistors for distro
