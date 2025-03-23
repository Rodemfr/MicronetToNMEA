# MicronetToNMEA

## Description

MicronetToNMEA is a Teensy/Arduino project aiming at converting data from Raymarine's TackTick wireless network called "Micronet" to a standard NMEA0183 stream, easily useable by your laptop or tablet software. Additionnaly, it can also transmit NMEA navigation data from your Tablet/PC to your Micronet network.

The project requires the following hardware :
- At least one Raymarine Wireless/Tacktick display device.
- A Teensy 4.0 development board.
- A CC1101 based board. Any board should be fine as long as you can connect its SPI bus to Teensy's MCU. Take care to order a board with an antenna for 868 or 915MHz operations, not 433MHz.

Optionally, you can add :
- A UBlox GPS/GNSS, connected through UART to add your position, time, date, SOG and COG to Micronet displays. The most frequently used one is UBlox M8N which can be directly configured by MicronetToNMEA.
- A LSM303AGR/LSM303DLHC/LSM303DLH, connected through I2C to add magnetic heading to Micronet displays

The type of construction described here is fun and interesting to play with, but anyone with a little bit
of experience at sea knows how aggressive is the wet, salty and brutal environment of a sailing boat.
MicronetToNMEA may abandon you just when you really need it. No garantee can of course be given that this software
will do what it has been designed for.

## Author & Contributors

* **Ronan Demoment** - [Rodemfr](https://github.com/Rodemfr) - Main author 
* **Dietmar Warning** - [dwarning](https://github.com/dwarning) - LSM303 Compass, bugfixes & testing.
* **[j-lang](https://github.com/j-lang)** - UBLOX M8N Initialization code & testing.

## License

MicronetToNMEA is licensed under GPLv3. See LICENSE.txt file for more details.

## Compilation

The source code compiles with [Arduino IDE](https://www.arduino.cc/en/software) extended by [Teensyduino](https://www.pjrc.com/teensy/td_download.html) software package. You just have to configure the right Teensy board and to import the required libraries (TeensyTimerTool). If you plan to develop/extend MicronetToNMEA, you probably should use [Visual Studio Code](https://code.visualstudio.com/) associated to [PlatformIO](https://platformio.org/) plugin. It is way beyond Arduino IDE in term of productivity but is harder to set up.

Check the [User Manual](https://github.com/Rodemfr/MicronetToNMEA/blob/master/doc/user_manual/user_manual.md) for more details.


## Acknowledgments

* Thanks to the guys of YBW.com forum who started the work of investigating Micronet's protocol. The technical discussions around the protocol are in this thread : https://forums.ybw.com/index.php?threads/raymarines-micronet.539500/
* Thanks to all the users who are giving feedback and reporting problems to help improving the software

## Setting up HW

MicronetToNMEA SW is configured by default to be connected via SPI bus to a CC1101 IC with the following scheme :

```
CC1101     Teensy
SI     <-- Pin 11 (MOSI0)
SO     --> Pin 12 (MISO0)
SCK    <-- Pin 14 (SCK0)
CS     <-- Pin 10 (CS0)
GD0    --> Pin 9
GND    <-> GND
3.3V   <-- 3.3V
```

MicronetToNMEA can also collect sentences from an NMEA GPS/GNSS connected to UART 3 of the Teensy :

```
GNSS     Teensy
TXD  --> Pin 15  (RX3)
RXD  <-- Pin 14  (TX3)
GND  <-> GND
3.3V <-- 3.3V
```

If you have a Ublox M8N or M6N GNSS, nothing is to be done to configure it. MicronetToNMEA will do it by itself. You can use a non UBlox GNSS but you will then have to configure it to output a NMEA stream with the same baudrate that you specified in `BoardConfig.h`.

MicronetToNMEA can use a LSM303 to provide magnetic heading on both Micronet and NMEA streams :

```
LSM303          Teensy
SCL         <-- Pin 19  (SCL0)
SDA         <-> Pin 18  (SDA0)
GND         <-> GND
3.3V        <-- 3.3V
```

LSM303AGR,LSM303DLHC or LSM303DLH can be used. MicronetToNMEA will automatically recognize it and select the appropriate driver.

If you want to use a different connection layout, you have to edit the related definitions at the beginning of `BoardConfig.h` file. The [User Manual](https://github.com/Rodemfr/MicronetToNMEA/blob/master/doc/user_manual/user_manual.md) explains every configuration item.

## Quick Start & general guidance

Compile MicronetToNMEA SW and flash it into your Teensy 4.0. See [Teensy 4.0 documentation](https://www.pjrc.com/store/teensy40.html) if you are not confortable with this board.

Power up your Teensy board through USB. Use a terminal software like [Tera Term](http://www.teraterm.org/) to reach the menu on the serial console.

Power up your Micronet network.

The first thing to do is to calibrate the RF frequency to ensure a good range performance of the system : enter menu "Calibrate RF frequency" and follow instructions. At the end of operations, save the calibration when asked. 

Now, you need to attach MicronetToNMEA to your Micronet network, for this, you have to select the
dedicated "Attach converter to closest network" menu. It will scan all the Micronet networks in your vincinity (20-30m range max) and attach to the closest one. Ensure your network is up and running for attachement to be successful.
 
You are now ready to convert your Micronet data to NMEA0183 with menu "Start NMEA conversion".

That's it !

Some tips :

- Once you have attached MicronetToNMEA to a Micronet network, it will automatically enter in NMEA conversion mode at each power-up. You don't need a connect a console anymore unless you want to attach to another network.
- When in conversion mode, if you want to come back to the configuration menu in the console, just press "ESC" key
- MicronetToNMEA listens to calibration values travelling on the network and will apply them to the converted values (wind speed factor, temperature offset, etc.). So if you change your sensor calibration from your Micronet display, MicronetToNMEA will memorize the new value. Be careful however that these calibration values are only processed when in NMEA conversion mode.
- Calibration values are all saved in EEPROM so that you don't need to enter them again in the system at each power-up.
- There is a menu "Test RF quality", useful to evaluate where to put MicronetoNMEA in your boat to maximize signal strength
  
## User manual

If you want more details on how to set-up and configure MicronetToNMEA, there is now a [User Manual](https://github.com/Rodemfr/MicronetToNMEA/blob/master/doc/user_manual/user_manual.md) in the doc directory.

## Make your own hardware

