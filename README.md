# MicronetToNMEA

## Description

MicronetToNMEA is a Teensy/Arduino project aiming at converting data from Raymarine's wireless network called "Micronet" to a standard NMEA0183 stream easily useable
by our laptop and tablet software.

The project requires the following hardware :
- A boat with Raymarine Wireless system. (The boat is not strictly required)
- A Teensy 3.5 board. Any other 32-bit Arduino compatible board should also work with some adaptations of SW.
- A CC1101 based board. Any board should be fine as long as you can connect its SPI bus to the MCU. Take care to order a board with an antenna for 868MHz operations, not 433MHz.

Optionally, you can add :
- A NMEA GPS/GNSS, connected through UART to add your position, time, date, SOG and COG to Micronet displays and to the NMEA output stream

The type of construction described here is fun and interesting to play with, but anywone with a little bit
of experience at sea knows that it will not last long in the wet, salty and brutal environment of a sailing boat.
MicronetToNMEA will abandon you just when you really need it.
If you want a robust, reliable and extensively tested Micronet device, you are at the wrong place. You should
better go to your nearest Raymarine/Tacktick reseller. 

## Author & Contributors

* **Ronan Demoment** - [Rodemfr](https://github.com/Rodemfr) - Main author 
* **Dietmar Warning** - [dwarning](https://github.com/dwarning) - LSM303 Compass and bugfixes 

## License

MicronetToNMEA is licensed under GPLv3. See LICENSE.txt file for more details.

## Compilation

The source code includes project files for Sloeber (i.e. Eclipse for Arduino). Just import the project in Sloeber and compile it.
If you prefer not to use Sloeber, you can create a new Arduino Sketch and import the all .h and .cpp inside. It will work.

## Acknowledgments

* Thanks to the guys of YBW.com forum who started the work of investigating Micronet's protocol. The technical discussions around the protocol are in this thread : https://forums.ybw.com/index.php?threads/raymarines-micronet.539500/

## Setting up HW

The SW is configured by default to run on a Teensy 3.5 board. It requires to be connected via SPI bus to a CC1101 IC with the following scheme :

```
CC1101     Teensy
SI     <-- Pin 11 (MOSI0)
SO     --> Pin 12 (MISO0)
SCK    <-- Pin 14 (SCK0)
CS     <-- Pin 10 (CS0)
GD0    --> Pin 24
GND    <-> GND
3.3V   <-- 3.3V
```

MicronetToNMEA can also collect sentences from an NMEA GPS/GNSS wich should be connected to UART 1 of the teensy board :

```
GNSS     Teensy
TXD  --> Pin 0  (RX1)
RXD  <-- Pin 1  (TX1)
GND  <-> GND
3.3V <-- 3.3V
```

Nothing is to be done on the SW side wether a GNSS is connected or not. If the GNSS is connected, it must however be configured to output a NMEA stream at 34800 baud. I use a Ublox NEO-M8N. Neo-M8N can be configured to output a NMEA stream at this baudrate by using U-Center software from U-Blox. If you order a U-Blox GNSS, check that your board has flash memory connected to the GPS to be able to save GNSS configuration once for all.

If you want to use a different MCU board and/or pinout, you have to edit the related define statements at the beginning of the BoardConfig.h file.

## Quick Start & general guidance

Power up your Teensy/Arduino board, you will reach a menu on the serial console.

Power up your Micornet network.

As a first step, you need to attach MicronetToNMEA to your Micronet network, for this, you have to scan existing networks (menu 2). It will list
all the detected networks in your vincinity (20-30m range max), in decreasing order of reception power. Yours is very likely at the top.
Write down the identifier of your network and attach MicronetToNMEA to it with menu 3.
 
You are now ready to convert your Micronet data to NMEA0183 with menu 4.

That's it !

Some tips :

- Once you have attached MicronetToNMEA to a Micronet network, it will automatically enter in NMEA conversion mode at each power-up. You don't need a connect a console to it.
- When in conversion mode, if you want to come back to the configuration menu, just press "ESC" key to leave the conversion mode
- MicronetToNMEA listens to calibration values transiting on the network and will apply them to the converted values (wind speed factor, temperature offset, etc.). So if you change your sensor calibration from your Micronet display, MicronetToNMEA will memorize the new value if it is in range. /!\ Be careful that these calibration values are only intercepted in NMEA conversion mode /!\
- Calibration values, as well as attached network ID are all saved in EEPROM so that you don't need to enter them again in the system at each power-up.
- There is an additionnal menu 5 allowing to scan all micronet traffic around you. This is useful to understand how devices are speaking to each other.
  
