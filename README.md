# MicronetToNMEA

## Description

MicronetToNMEA is an Arduino project aiming at converting data from Raymarine's wireless network called "Micronet" to a standard NMEA0183 stream easily useable
by our laptop and tablet software.

The project requires the following hardware :
- A boat with Raymarine Wireless system. (The boat is not strictly required)
- A Teensy 3.5 board. Any other arduino comptabile board should be fine with minor adaptations of SW.
- A CC1101 based board. Any board should be fine as long as you can connect its SPI bus to the MCU.

The type of construction described here is fun and interesting to play with, but anywone with a little bit
of experience at sea knows that it will not last long in the wet, salty and brutal environment of a sailing boat.
MicronetToNMEA will abandon you just when you really need it.
If you want a robust, reliable and extensively tested Micronet device, you are at the wrong place. You would
better go to your nearest Raymarine/Tacktick reseller. 

## Author

* **Ronan Demoment** - [Rodemfr](https://github.com/Rodemfr)

## License

MicronetToNMEA is licensed under GPLv3. See LICENSE.txt file for more details.

## Compilation

The source code includes project files for Sloeber (i.e. Eclipse for Arduino). Just import the project in Sloeber and compile it.
If you prefer not to use Sloeber, you can create a new Arduino Sketch and import the all .h and .cpp inside. It will work.

## Acknowledgments

* Thanks to the guys of YBW.com forum who started the work of investigating Micronet's protocol.

## Setting up HW

The SW is configured by default to run on a Teensy 3.5 board and expects the following connection with CC1101 :

```
SI   <-- Pin 11 (MOSI0)
SO   --> Pin 12 (MISO0)
SCK  <-- Pin 14 (SCK0)
CS   <-- Pin 10 (CS0)
GD0  --> Pin 24
GND  <-> GND
3.3V <-> 3.3V
```

If you want to use a different board and/or pinout, you have to edit the related define statements at the beginning of Main.cpp file.

MicronetToNMEA is controlled through a console connected to USB/UART0. Just connect the USB connector of the board to your PC and
use a terminal like TeraTerm with the following configuration : 4800 bauds, 8 bit data, 1 stop bit, no parity.

## Quick Start & general guidance

Power up your Teensy/Arduino board, you will reach a menu on the serial console.

Power up your Micornet network.

As a first step, you need to attach MicronetToNMEA to your Micronet network, for this, you have to scan existing networks (menu 2). It will list
you all the detected network in your vincinity (20-30m range max), in decreasing order of reception power. Yours is very likely at the top.
Write down the identifier of your network and enter it in menu 3.
 
You are now ready to convert your Micronet data to NMEA0183 with menu 4.

That's it !

Some tips :

- Once you have attached MicronetToNMEA to a Micronet network, it will automatically enter in NMEA conversion mode at each power-up. If you want to come back to menu, just press "ESC" key to leave the conversion mode
- MicronetToNMEA listens to calibration values transiting on the network and will apply them to the converted values (wind speed factor, temperature offset, etc.).
- Be carefull that these calibration values are only intercepted in NMEA conversion mode and when you edit them on you Micronet display.
- If MicronetToNMEA has missed some of these calibration values, just edit them with one of your Micronet display while in NMEA conversion mode. It will be seen and remembered by MicronetToNMEA.
- Calibration values, as well as attached network ID are all saved in EEPROM so that you don't need to enter them again in the system at each power-up.
- There is an additionnal menu 5 allowing to scan all micronet traffic around you. This is useful to understand how devices are speaking to each other.
  