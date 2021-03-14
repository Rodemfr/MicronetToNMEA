**********************************************
*** Common header of all Micronet messages ***
**********************************************

General format :

|03 77 37 02 03 90 87 02 01 07 5A 18 18 xx xx xx xx ...| -> Example from a wind transducer
|  NID   |DT|  DID   |MI|SO|DE|CS| LEN |    Payload ...|

NID = Network ID, seems to be equal to the Device ID of the network main display

DT = Device Type (1 byte) - When sending a message, a device uses this byte to tell who it is.
     0x01 Hull Transmitter
     0x02 Wind Transducer
     0x03 NMEA Converter
     0x04 Mast Rotation Sensor
     0x05 MOB
     0x83 Analog Wind Display
     0x81 Dual Display

DID = Device ID (3 bytes) - Unique number identifying the device

MI = Message ID (1 bytes) - Type of the message. Necessary for the receiver to properly decode it. 
     0x01 Request Data
     0x02 Send Data
     0x06 Set Calibration
     0x0a ?
     0x0b ?

SO = Source
     0x01 Measurement device
     0x09 Display

DE = Destination ? Purpose ?
     0x00 Broadcast / All devices
     0x09 Display

CS = Looks like a checksum but I did not find the way to calculate it

LEN = Length-1 of the complete message in bytes, including payload. Value duplicated on two bytes, probably for robustness

********************************
*** Message 0x02 (Send Data) ***
********************************

This message is used to send data from one device to one another. It is a response to "Request Data" message sent by display to
collect data from all sensors. It sends a variable number of fields depending on the Source Device. Each field is coded the
same way but with a variable size. The number of fields can be dynamically discovered during decoding by
comparing each field size to the complete message size.

Two messages from wind transducers in two different networks:
|03 77 37|02|03 90 87|02|01|09|5C|18 18|04 05 05 00 2C 3A|04 06 05 FF FB 09|
|09 ca 2e|02|0a ca 2e|02|01|09|13|18 18|04 05 05 00 00 0e|04 06 05 00 32 41|
|  NID   |DT|  DID   |MI|SO|DE|CS| LEN |FF|FI|FP| AWS |FC|FF|FI|FP| AWA |FC|
|          MESSAGE HEADER              |      FIELD1     |      FIELD2     |

Message sent by an analog wind display
|03 77 37|83|03 77 37|02|09|00|73|1A 1A|05 21 05 00 00 09 34|05 22 05 00 64 09 99|
|  NID   |DT|  DID   |MI|SO|DE|CS| LEN |FF|FI|FP| TWS |DE|FC|FF|FI|FP| TWA |DE|FC|
|          MESSAGE HEADER              |       FIELD1       |       FIELD2       |

Message sent by a dual display
|03 77 37|81|03 70 82|02|09|00|B5|1A 1A|05|21|05|00 00|09|34|05 22 05 00 33|09|68|
|  NID   |DT|  DID   |MI|SO|DE|CS| LEN |FF|FI|FP| TWS |DE|FC|FF|FI|FP| TWA |DE|FC|
|          MESSAGE HEADER              |       FIELD1       |       FIELD2       |

Message sent by a hull transmitter
|03 77 37|01|0B C0 22|02|01|09|2E|49 49|04|04|05|13 89|A9|04|1B|05|00 89|AD|05|21|05|00 00|06|31|05|22|05|FF F5|06|26|04|01|05|00 BB|C5|0A|02|05|00 00 00 95|00 00 00 1C|C2|03|03|05|25|30|04|05|03|00 00|0C|04|06|03|FF F5|01|
|  NID   |DT|  DID   |MI|SO|DE|CS| LEN |FF|FI|FP| DPT |FC|FF|FI|FP| VCC |FC|FF|FI|FP| AWS |DE|FC|FF|FI|FP| AWA |DE|FC|FF|FI|FP| SPD |FC|FF|FI|FP|   TRIP    |    LOG    |FC|FF|FI|FP|??|FC|FF|FI|FP| AWS |FC|FF|FI|FP| AWA |FC|
|          MESSAGE HEADER              |      FIELD1     |      FIELD2     |       FIELD3       |       FIELD4       |     FIELD5      |              FIELD6               |    FIELD7    |      FIELD8     |     FIELD9      |


FT = Field format
     0x04 Total field description is on 6 bytes, including CRC. Value is on 16-bits
     0x05 Total field description is on 7 bytes, including CRC. In this case there is an extra byte
                after the value which seems to be the destination of the data.

FI = Field ID
     0x01 SPD*100(KT)
     0x02 TRIP*100(NM) + LOG*10(NM)
     0x04 DEPTH*10(M)?? A value of 5001 (0x1389) is given by the hull transmitter when depth is not available.
     0x05 AWS*10(KT) 
     0x06 AWA(DEG)
     0x1b VCC*10(V)
     0x21 AWS*10(KT)  Seems the same than 0x05 ?
     0x22 AWA(DEG)    Seems the same than 0x06 ?

FP = Field Property ?
     0x05 It seems to be the value used when the data comes from the measuring sensor
     0x03 It seems to be the value used when the data comes from a repeater (i.e. the main display or hull transmitter)

FC = Field checksum. Sum of the all the byte of the field.

**************************************
*** Message 0x06 (Set Calibration) ***
**************************************

Example : Wind direction calibration +10 deg
|03 77 37 83 03 77 37 06 09 09 80 13 13 FF 07 02 0A 00 04 16|
|  NID   |DT|  DID   |MI|SO|DE|CS| LEN |??|PI|PL| VA  |SQ|CS|
|          MESSAGE HEADER              |     CORRECTION     |

Example : Wind Speed Calibration +7 deg
|03 77 37|83|03 77 37|06|09|00|77|12 12|FF 06 01 07 0B 18|
|  NID   |DT|  DID   |MI|SO|DE|CS| LEN |??|PI|PL|VA|SQ|CS|
|          MESSAGE HEADER              |    CORRECTION   |

Example : Speed filtering level MED
|03 77 37 81 03 70 82 06 09 09 C2 12 12 FF 04 01 20 0E 32|
|  NID   |DT|  DID   |MI|SO|DE|CS| LEN |??|PI|PL|VA|SQ|CS|
|          MESSAGE HEADER              |    CORRECTION   |

PI Parameter ID
   0x04 Speed filtering level
      VA = 0x00 : AUTO
      VA = 0x10 : SLOW
      VA = 0x20 : MED
      VA = 0x30 : FAST
   0x06 Wind speed factor
      VA = Speed correction as signed 8bit interger in degrees
   0x07 Wind direction offset
      VA = An signed 16-bit integer in degrees /!\ LITTLE ENDIAN VALUE /!\

PL Parameter Length ? Seems to be the number of bytes on wich VA is coded.
