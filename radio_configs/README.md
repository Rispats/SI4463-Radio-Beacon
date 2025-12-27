# Radio Configuration Files

Pre-configured SI4463 radio settings generated using Silicon Labs Wireless Development Suite (WDS).

## Files

Configuration files for different frequencies and modulation schemes:

### 162 MHz
- `162_2fsk.txt` - 2FSK modulation
- `162_2gfsk.txt` - 2GFSK modulation
- `162_ook.txt` - OOK modulation

### 433 MHz
- `433_2fsk.txt` - 2FSK modulation
- `433_2gfsk.txt` - 2GFSK modulation
- `433_ook.txt` - OOK modulation

### 868 MHz
- `868_2fsk.txt` - 2FSK modulation
- `868_2gfsk.txt` - 2GFSK modulation
- `868_ook.txt` - OOK modulation

## Configuration Details

- **Transmit Power**: 20 dBm (maximum)
- **Data Rate**: 9.6 kbps
- **Crystal**: 26 MHz
- **SPI Mode**: Mode 0

## Usage

1. Open the desired config file in a text editor
2. Copy the configuration array
3. Paste into your Arduino sketch at the `radioConfig[]` declaration
4. Upload to your microcontroller

## Generating Custom Configurations

Use Silicon Labs WDS3 software to generate custom configurations:
1. Download WDS from Silicon Labs website
2. Select SI4463 device
3. Configure desired parameters (frequency, modulation, power, etc.)
4. Export configuration array
5. Copy to your Arduino sketch
