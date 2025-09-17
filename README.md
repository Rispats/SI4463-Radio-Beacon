# GPS-Enabled RF Beacon

A GPS-enabled RF beacon system for transmitting and receiving real-time GPS coordinates (date, time, latitude, longitude) over user-customizable frequency bands (142-1050Mhz) in addition to replicating traditional RF beacon functionality of transmitting periodic bursts of continous waves (tones).

Designed using Arduino/ESP32 microcontrollers, GNSS module, and the SI4463 RF transceiver, the system supports multiple transmitters with unique IDs, checksum validation, and real-time monitoring via a C-based command-line interface.



## *FEATURES* 
- Real-time GPS data (NEAM strings) acquisition using the EVE GNSS L89.
- Wireless transmission via SI4463 RF transceiver (162 MHz, 433 MHz and 868 MHz; all were tested using 2GFSK, 2FSK and OOK).
- Continous Wave (160.725Mhz) trasnmission for traditional beacon functionality (Detectable via deficated receiver such as DF500N). 
- Multi-transmitter operation with unique IDs (TX_A1, TX_A2, …).
- Packet robustness and integrity ensured through start/stop bits and checksum validation.
- Simple C program UI for live data monitoring & filtering.



## *COMPONENTS USED*
- **Arduino Nano ESP32-S3** → Transmitter MCU.
- **ESP32-C3-M1** → Receiver MCU.
- **EVE GNSS L89** → GPS Module.
- **GNice RF4463PRO-868** → Sub-GHz Transceiver.
- A generic copper wire as antenna (Coiled ~2.2 cm / Extended ~25 cm).

   <img width="200" height="200" alt="Screenshot from 2025-09-16 21-53-05" src="https://github.com/user-attachments/assets/00c07dfb-4116-4019-940d-fc2f95081034"/>




## *SYSTEM DESIGN*
### 1) Transmitter:
- Acquires GPS data in the form of NMEA strings and sends it to the microcontoller.
- The TinyGPS++ library (in MCU firmware) then parses these NMEA strings and extracts the date, time, lat and long values.
- A string containing this GPS data, start/stop bit and the transmitter ID is then constructed.
- Format of the final string that is to be transmitted looks like:
```
$TX_XX,DD/MM/YYYY,HH:MM:SS,LAT,LONG*[CS]
```
- Transmits this string over RF using SPI-configured SI4463.
  
### 2) Receiver:
- Continuously listens for packets.
- Validates data with checksum & discards corrupted/invalid strings.
- Only VALID strings are relayed to the serial port whihc will be read by the C program for display.
- Provides real-time monitoring via the C program on the terminal window.
- Each tranmitter ID is alloted its own dedicated space to dislay the only the latest data that has arrived.
- Format of the received signal looks like:
```
&TX_XX,DD/MM/YYYY,HH:MM:SS,LAT,LONG*[calc_cs|recv_cs][STATUS: status]
```

### 3) C Program:
- When executed, it reads the invalid stings relayed to the serial port, parses them to extract transmitted ID, Date, Time Lat and Long.
- Allots each transmitted ID and the data assiciated with it some space on the terminal window in a table like format.
- This table is updated for each transmitted ID as the latest values for it arrives.

<p align="center">
<img width="500" height="764" alt="Screenshot from 2025-09-17 21-53-54" src="https://github.com/user-attachments/assets/a786933e-c164-491d-9a76-c8ab44eb8e8e" />
</p>

## *WORK FLOW*
- GPS locks onto sufficient number of satellites and outputs NMEA strings.
- TinyGPS++ parses NMEA data to extract date, time, latitude, and longitude.
- Transmitter forms a packet with Tx ID, date, time, latitude, and longitude.
- Start/stop bytes are added, checksum is calculated, and the packet is transmitted.
- Receiver detects incoming packets, checks framing, and verifies checksum.
- Invalid packets are discarded, valid packets are processed.
- A C program extracts Tx ID, date, time, latitude, and longitude from valid packets.
- Data is displayed in a clean table like manner with dedicated space for each transmitter.
- Program supports filtering to show data from selected transmitters only.

## *RESULTS*


### 162Mhz:

<img width="1600" height="1000" alt="fin162" src="https://github.com/user-attachments/assets/9433d3ca-148a-4d97-85b7-f7919ccb19d8"/>






### 433Mhz:

<img width="1600" height="1000" alt="Figure_1" src="https://github.com/user-attachments/assets/47b7742d-3fb4-48fa-b8a2-348c0191283d"/>






### 868Mhz:

<img width="1600" height="1000" alt="fin868" src="https://github.com/user-attachments/assets/36c6332c-4638-49e4-9834-1acca6eec348"/>


### **OVERALL COMBINED RESULT:**


<img width="1600" height="1000" alt="ovr" src="https://github.com/user-attachments/assets/7813f49a-2e01-4124-a6d1-4c4560e984bd" />


### NOTE: 
All the radio config files are for max transmitter power(20dbm) and a data rate of 9.6kbps.
