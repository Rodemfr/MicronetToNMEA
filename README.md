# MicronetToNMEA

## Note to Teensy users

MicronetToNMEA has migrated to ESP32. For Teensy-based hardware, please refer to the legacy code in the `teensy` branch.

## Description

MicronetToNMEA is an Arduino project that converts data from Raymarine's wireless Micronet network into standard NMEA0183 sentences, making it readily compatible with laptop or tablet navigation software. Additionally, it can transmit NMEA navigation data from your device back to the Micronet network.

Required hardware:
- A Raymarine/Tacktick master device (one with an ON/OFF button)
- An ESP32-WROOM-32 based board (such as uPesy Wroom DevKit)
- A CC1101 based board operating in the 868/915MHz range (ensure you order one with the appropriate antenna)

Optional components:
- A NMEA GPS/GNSS module connected via UART to provide position, time, date, SOG, and COG data to Micronet displays. The UBlox M8N is fully supported and can be configured directly by MicronetToNMEA
- A LSM303DLH compass module connected via I2C to provide magnetic heading data

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

For the default uPesy Wroom DevKit configuration, connect the CC1101 module as follows:

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

NMEA GNSS device connection (UART 2):

```
GNSS     ESP32-WROOM-32
TXD  --> Pin 16  (RX1)
RXD  <-- Pin 17  (TX1)
GND  <-> GND
3.3V <-- 3.3V
```

LSM303DLH(C) compass connection:

```
LSM303DLH(C)    ESP32-WROOM-32
SCL         <-- Pin 22  (SCL1)
SDA         <-> Pin 21  (SDA1)
GND         <-> GND
3.3V        <-- 3.3V
```

The system supports LSM303DLH, LSM303DLHC, and LSM303AGR compass modules, automatically detecting and configuring the appropriate driver.

To customize the pinout or use different hardware, modify the definitions in BoardConfig.h. See the [User Manual](https://github.com/Rodemfr/MicronetToNMEA/blob/master/doc/user_manual/user_manual.md) for configuration details.

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
