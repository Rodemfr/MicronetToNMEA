# MicronetToNMEA

## Note to Teensy users

MicronetToNMEA has migrated to ESP32. For Teensy-based hardware, please refer to the legacy code in the [`teensy`](https://github.com/Rodemfr/MicronetToNMEA/tree/teensy) branch.

## Description

MicronetToNMEA is an Arduino project that converts data from Raymarine's wireless Micronet network into standard NMEA0183 sentences, making it readily compatible with laptop or tablet navigation software. Additionally, it can transmit NMEA navigation data from your device back to the Micronet network.

Required hardware:
- A Raymarine/Tacktick master device (one with an ON/OFF button)
- An ESP32 WROOM DevKitC based board ([such as uPesy Wroom DevKit](https://www.upesy.fr/products/upesy-esp32-wroom-devkit-board))
- A CC1101 based board operating in the 868/915MHz range (ensure you order one with the appropriate 868MHz antenna, not the 433MHz one)

Optional components:
- A NMEA GPS/GNSS module connected via UART to provide position, time, date, SOG, and COG data to Micronet displays. The UBlox M8N is fully supported and can be configured directly by MicronetToNMEA
- A LSM303 compass module connected via I2C to provide magnetic heading data

Important Notice: While this DIY solution is an interesting project, please be aware that it may not withstand the harsh marine environment (salt, moisture, vibrations) as well as commercial equipment. MicronetToNMEA comes with no guarantees and should not be relied upon as your sole navigation system. For mission-critical applications, we recommend purchasing certified Raymarine/Tacktick equipment from authorized dealers.

## Authors & Contributors

* **Ronan Demoment** - [Rodemfr](https://github.com/Rodemfr) - Project creator and maintainer 
* **Dietmar Warning** - [dwarning](https://github.com/dwarning) - LSM303 Compass integration, bug fixes & testing
* **[j-lang](https://github.com/j-lang)** - UBLOX M8N initialization code & testing

## License

MicronetToNMEA is released under the GPLv3 license. See LICENSE.txt for complete details.

## Building the Project

The project can be built using [Visual Studio Code](https://code.visualstudio.com/) with the [PlatformIO](https://platformio.org/) extension installed.

For detailed instructions, please refer to the [User Manual](https://github.com/Rodemfr/MicronetToNMEA/blob/master/doc/user_manual/user_manual.md).

## Acknowledgments

* Special thanks to the YBW.com forum members who initiated the investigation of the Micronet protocol. Technical discussions can be found here: https://forums.ybw.com/index.php?threads/raymarines-micronet.539500/
* We appreciate all users who provide feedback and report issues, helping improve the software

## Hardware Setup

For the default WROOM DevKitC configuration, connect the CC1101 module as follows:

```
CC1101     WROOM DevKitC
SI     <-- Pin 23 (VSPI_MOSI)
SO     --> Pin 19 (VSPI_MISO)
SCK    <-- Pin 18 (VSPI_SCK)
CS     <-- Pin 5 (VSPI_CS)
GD0    --> Pin 35
GND    <-> GND
3.3V   <-- 3.3V
```

NMEA GNSS device connection (UART 2):

```
GNSS     WROOM DevKitC
TXD  --> Pin 16  (RX2)
RXD  <-- Pin 17  (TX2)
GND  <-> GND
3.3V <-- 3.3V
```

LSM303DLH(C) compass connection:

```
LSM303DLH(C)    WROOM DevKitC
SCL         <-- Pin 22  (I2C_SCL)
SDA         <-> Pin 21  (I2C_SDA)
GND         <-> GND
3.3V        <-- 3.3V
```

The system supports LSM303DLH, LSM303DLHC, and LSM303AGR compass modules, automatically detecting and configuring the appropriate driver.

To customize the pinout or use different hardware, modify the definitions in `BoardConfig.h`. See the [User Manual](https://github.com/Rodemfr/MicronetToNMEA/blob/master/doc/user_manual/user_manual.md) for configuration details.

## Quick Start Guide

1. Connect your ESP board via USB
2. Open a terminal (e.g., [Tera Term](http://www.teraterm.org/)) to access the configuration menu
3. Power up your Micronet network
4. Calibrate RF frequency (use "Calibrate RF frequency" menu and follow instructions)
5. Attach to the closest network (use "Attach to closest network" menu)
6. Start the NMEA conversion using "Start NMEA conversion"

Tips and Features:

- After initial setup, MicronetToNMEA automatically enters conversion mode on power-up
- Press ESC during conversion to access the configuration menu
- The system captures and applies network calibration values (wind speed, temperature, etc.)
- All settings (calibration, network ID) are stored in EEPROM
- Use "Scan surrounding Micronet traffic" to monitor network communications
- "Test RF quality" helps optimize converter placement for best signal reception

## Documentation

For comprehensive setup and configuration details, consult the [User Manual](https://github.com/Rodemfr/MicronetToNMEA/blob/master/doc/user_manual/user_manual.md) in the doc directory.
