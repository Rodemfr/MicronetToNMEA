# MicronetToNMEA

## Note to Teensy users

MicronetToNMEA has now switched to ESP32. If you want the latest version of the Teensy based software, check the `teensy' branch.

## Description

MicronetToNMEA is a Arduino project aiming at converting data from Raymarine's wireless network called "Micronet" to a standard NMEA0183 stream, easily useable by your laptop or tablet software. Additionnaly, it can also transmit NMEA navigation data from your Tablet/PC to your Micronet network.

The project requires the following hardware :
- At least one Raymarine/Tacktick master device (i.e. with a ON/OFF button)
- A ESP32-WROOM-32 based board (e.g. uPesy Wroom DevKit).
- A CC1101 based board. Any board should be fine as long as you can connect its SPI bus to the MCU. Take care to order a board with an antenna for 868 or 915MHz operations, not 433MHz.

Optionally, you can add :
- A NMEA GPS/GNSS, connected through UART to add your position, time, date, SOG and COG to Micronet displays. The most frequently used one is UBlox M8N which can be directly configured by MicronetToNMEA.
- A LSM303DLH connected through I2C to add magnetic heading to Micronet displays

The type of construction described here is fun and interesting to play with, but anyone with a little bit
of experience at sea knows that may not last long in the wet, salty and brutal environment of a sailing boat.
MicronetToNMEA may abandon you just when you really need it. No garantee can of course be given that this software
will do what it has been designed for.
If you want a robust, reliable and extensively tested Micronet device, you should better go to your nearest Raymarine/Tacktick reseller. 

## Author & Contributors

* **Ronan Demoment** - [Rodemfr](https://github.com/Rodemfr) - Main author 
* **Dietmar Warning** - [dwarning](https://github.com/dwarning) - LSM303 Compass, bugfixes & testing.
* **[j-lang](https://github.com/j-lang)** - UBLOX M8N Initialization code & testing.

## License

MicronetToNMEA is licensed under GPLv3. See LICENSE.txt file for more details.

## Compilation

The source code compiles with [Visual Studio Code](https://code.visualstudio.com/) extended by [PlatformIO](https://platformio.org/) plugin.

Check the [User Manual](https://github.com/Rodemfr/MicronetToNMEA/blob/master/doc/user_manual/user_manual.md) for more details.


## Acknowledgments

* Thanks to the guys of YBW.com forum who started the work of investigating Micronet's protocol. The technical discussions around the protocol are in this thread : https://forums.ybw.com/index.php?threads/raymarines-micronet.539500/
* Thanks to all the users who are giving feedback and reporting problems to help improving the software

## Setting up HW

Supposing you use a uPesy Wroom DevKit board, the SW is configured by default to be connected via SPI bus to a CC1101 IC with the following scheme :

```
CC1101     ESP32-WROOM-32
SI     <-- Pin 23 (MOSI0)
SO     --> Pin 19 (MISO0)
SCK    <-- Pin 18 (SCK0)
CS     <-- Pin 5 (CS0)
GD0    --> Pin 35
GND    <-> GND
3.3V   <-- 3.3V
```

MicronetToNMEA can also collect sentences from an NMEA GNSS or an AIS connected to UART 2 of the board :

```
GNSS     ESP32-WROOM-32
TXD  --> Pin 16  (RX1)
RXD  <-- Pin 17  (TX1)
GND  <-> GND
3.3V <-- 3.3V
```

MicronetToNMEA can use a LSM303DLH(C) to provide magnetic heading on both Micronet and NMEA streams :

```
LSM303DLH(C)    ESP32-WROOM-32
SCL         <-- Pin 22  (SCL1)
SDA         <-> Pin 21  (SDA1)
GND         <-> GND
3.3V        <-- 3.3V
```

LSM303DLH, LSM303DLHC and LSM303AGR are supported. MicronetToNMEA will automatically recognize the version and select the appropriate driver.

If you want to use a different MCU board and/or pinout, you have to edit the related definitions at the beginning of BoardConfig.h file. [User Manual](https://github.com/Rodemfr/MicronetToNMEA/blob/master/doc/user_manual/user_manual.md) explains every configuration item.

## Quick Start & general guidance

Power up your ESP board through USB. Use a terminal software like [Tera Term](http://www.teraterm.org/) to reach the menu on the serial console. Baudrate is meaningless on a USB bridge.

Power up your Micronet network.

The first thing to do is to calibrate the RF frequency to ensure a good range performance of the system : enter menu "Calibrate RF frequency" and follow instructions. At the end of operations, save the calibration when asked. 

Now, you need to attach MicronetToNMEA to your Micronet network, for this, you have to scan existing networks
(menu "Scan Micronet networks"). It will list all the detected networks in your vincinity (20-30m range max), in decreasing
order of reception power. Yours is very likely at the top.
Write down the identifier of your network and attach MicronetToNMEA to it with menu "Attach converter to a network".
 
You are now ready to convert your Micronet data to NMEA0183 with menu "Start NMEA conversion".

That's it !

Some tips :

- Once you have attached MicronetToNMEA to a Micronet network, it will automatically enter in NMEA conversion mode at each power-up. You don't need a connect a console anymore unless you want to attach to another network.
- When in conversion mode, if you want to come back to the configuration menu in the console, just press "ESC" key
- MicronetToNMEA listens to calibration values transiting on the network and will apply them to the converted values (wind speed factor, temperature offset, etc.). So if you change your sensor calibration from your Micronet display, MicronetToNMEA will memorize the new value if it is in range. **/!\ Be careful that these calibration values are only processed in NMEA conversion mode**
- Calibration values, as well as attached network ID are all saved in EEPROM so that you don't need to enter them again in the system at each power-up.
- There is a menu "Scan surrounding Micronet traffic" allowing to scan all micronet traffic around you. This is useful to understand how devices are speaking to each other.
- There is also a menu "Test RF quality", useful to evaluate where to put MicronetoNMEA in your boat to maximize signal strength
  
## User manual

If you want more details on how to set-up and configure MicronetToNMEA, there is now a [User Manual](https://github.com/Rodemfr/MicronetToNMEA/blob/master/doc/user_manual/user_manual.md) in the doc directory.
