# Receiver Firmware

This folder contains Arduino firmware for the GPS beacon receiver.

## Files

- **rx_GPS.ino** - Receiver that validates and relays GPS data to serial port

## Usage

1. Open `rx_GPS.ino` in Arduino IDE
2. Insert your radio configuration array (generated from WDS) at the marked location
3. Select board: ESP32C3 Dev Module
4. Select correct COM port
5. Upload to your ESP32-C3-M1

## Hardware Requirements

- ESP32-C3-M1
- SI4463 RF Module (GNice RF4463PRO-868 or compatible)
- Appropriate antenna for your frequency

## Operation

The receiver continuously listens for packets, validates checksums, and outputs valid data to serial port at 115200 baud. Connect the C monitoring program to view formatted data.
