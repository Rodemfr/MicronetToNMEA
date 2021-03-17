# MicronetToNMEA

MicronetToNMEA is an Arduino project aiming at converting data from Raymarine's wireless network called "Micronet" to a standard NMEA stream.

## Description

MicronetToNMEA utimate goal is to understand the Micronet protocol and enable various DIY project around's Raymarine Wireless system.

The project requires the following hardware :
- A boat with Raymarine Wireless system.
- Teendy 3.5 board. Any other arduino comptabile board should be fine.
- A CC1101 based board. Any board should be fine as long as you can connect its SPI bus to the MCU.

## Author

* **Ronan Demoment** - [Rodemfr](https://github.com/Rodemfr)

## License

MicronetToNMEA is licensed under GPLv3. See LICENSE.txt file for more details.

## Compilation

The source code includes project files for Sloeber (i.e. Eclipse). Just import the project in Sloeber and compile it.
If you prefer not to use Sloeber, you can create a new Arduino Sketch and import the all .h and .cpp inside. It will work.

## Acknowledgments

* Thanks to the guys of YBW.com forum who started the work of investigating Micronet's protocol.

