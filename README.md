# GPS-Enabled RF Beacon

A GPS-enabled RF beacon system for transmitting and receiving real-time GPS coordinates (date, time, latitude, longitude) over customizable frequency bands in addition to replicating traditionaly RF beacon functionality of sending continous bursts of Continous Waves (tones). Designed using ESP32 microcontrollers, GNSS modules, and the Si4463 RF transceiver, the system supports multiple transmitters with unique IDs, checksum validation, and real-time monitoring via a C-based command-line interface.



## *FEATURES* 
- Real-time GPS acquisition using EVE GNSS L89.
- Wireless transmission via SI4463 RF transceiver (162 MHz / 433 MHz / 868 MHz).
- Continous Wave (106.725Mhz) trasnmission for traditional beacon functionality. 
- Multi-transmitter operation with unique IDs (TX_A1, TX_A2, …).
- Packet integrity ensured through checksum validation.
- Simple C program UI for live data monitoring & filtering.



## *COMPONENTS USED*
- Arduino Nano ESP32-S3 → Transmitter MCU.
- ESP32-C3-M1 → Receiver MCU.
- EVE GNSS L89 GPS Module.
- Si4463 Sub-GHz Transceiver (GNiceRF RFSi4463-868).
- Coiled & Extended Antennas (~2.2 cm / ~25 cm).



## *SYSTEM DESIGN*
### 1) Transmitter:
- Acquires GPS data in the form of NMEA strings and sends it to the microcontoller.
- The TinyGPS++ library (in MCU firmware) then parses these NMEA strings and extracts the date, time, lat and long values.
- A string containing this GPS data, start/stop bit and the transmitter ID is then constructed.
- Final string that is to be transmitted looks like:
```
$TX_A1,DD/MM/YYYY,HH:MM:SS,LAT,LONG*[CS]
```
- Transmits this string over RF using SPI-configured Si4463.
### 2) Receiver:
- Continuously listens for packets.
- Validates data with checksum & discards corrupted/invalid strings.
- Only valid strings are relayed to the serial port whihc will be read by the C program for display.
- Provides real-time monitoring via the C program on the terminal window.
- Each tranmitter ID is alloted its own dedicated space to dislay the only the latest data that has arrived.



## *WORK FLOW*
- GPS locks onto ≥4 satellites and outputs NMEA strings.
- TinyGPS++ parses NMEA data to extract date, time, latitude, and longitude.
- Transmitter forms a packet with Tx ID, date, time, latitude, and longitude.
- Start/stop bytes are added, checksum is calculated, and the packet is transmitted.
- Receiver detects incoming packets, checks framing, and verifies checksum.
- Invalid packets are discarded, valid packets are processed.
- A C program extracts Tx ID, date, time, latitude, and longitude from valid packets.
- Data is displayed in a clean table with dedicated space for each transmitter.
- Program supports filtering to show data from selected transmitters only.
