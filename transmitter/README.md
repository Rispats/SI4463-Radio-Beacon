# Transmitter Firmware

This folder contains Arduino firmware for the GPS beacon transmitters.

## Files

- **tx_actualGPS.ino** - Transmitter using real GPS data from EVE GNSS L89 module
- **tx_simulatedGPS.ino** - Transmitter using simulated GPS data (includes corruption testing)
- **tx_continous_wave.ino** - Continuous wave (CW) beacon transmitter

## Usage

1. Open the desired `.ino` file in Arduino IDE
2. Insert your radio configuration array (generated from WDS) at the marked location
3. Select board: Arduino Nano ESP32-S3
4. Select correct COM port
5. Upload to your microcontroller

## Hardware Requirements

- Arduino Nano ESP32-S3
- SI4463 RF Module (GNice RF4463PRO-868 or compatible)
- EVE GNSS L89 GPS Module (for tx_actualGPS only)
- Appropriate antenna for your frequency
