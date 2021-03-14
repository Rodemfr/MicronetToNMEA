# MicronetToNMEA

MicronetToNMEA is an Arduino project aiming at converting data from Raymarine's wireless network called "Micronet" to an standard NMEA stream.

## Description

MicronetToNMEA utimate goal is to understand the Micronet protocol and enable various DIY project around's Raymarine Wireless system.

The project requires the following hardware :
- A boat with Raymarine Wireless system.
- Teendy 3.5 board. Any other arduino comptabile board should be fine.
- A small CC1101 based board. Once again, any board should be fine as long as you can connect its SPI bus to the MCU.

## Author

* **Ronan Demoment** - [Rodemfr](https://github.com/Rodemfr)

## License

I need to clarify all licence restrictions implied by CC1101 driver usage before publishing this code to the public.
If everything is OK, it will be released under a classic open source license.

## Compilation

The source code includes project files for Sloeber (i.e. Eclipse). Just import the project in Sloeber and compile it.

## Acknowledgments

* Thanks to the guys of YBW.com forum who started the work of investigating Micronet's protocol. The story continues...
