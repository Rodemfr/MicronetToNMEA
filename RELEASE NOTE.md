# MicronetToNMEA Release Note

## V2.4
- SOG and COG can now be filtered to get more stable values at low speed

## V2.3
- Decode SOG & COG from RMC sentence in addition of GGA

## V2.2
- Made NMEA decoder more robust to sentences with invalid `<CR><LF>` sequences
- Added support for GLL NMEA sentence

## V2.1
- Reorganized the code for better readability
- Added device network link quality information in RF quality menu
- Added the possibility to emulate SPD with SOG for system without water speed
- Added support for compilation with Visual Studio Code with PlatformIO plugin

## V2.0
- First official release of MicronetToNMEA