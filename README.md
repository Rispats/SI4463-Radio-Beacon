# SI4463 Radio Beacon - Minimal Manual

## Overview
- Transmitter sends framed messages: `<START><ID><PAYLOAD><STOP><CS_H><CS_L>`
- Payload: `YYYY-MM-DD,HH:MM:SS,lat,lon`
- Checksum: 8-bit sum of payload bytes, appended as two ASCII hex digits after STOP.
- TX transmits every 2 seconds, 2 valid followed by 1 deliberately corrupted message.
- RX verifies checksum and prints a single parseable line per packet:
  - `RESULT,ID,DATE,TIME,LAT,LON,RECV_CS,CALC_CS,VALID|CORRUPT`

## Files
- `tx_optimised.c`: Arduino transmitter
- `rx_optimised.c`: Arduino receiver
- `beacon_monitor.sh`: Minimal terminal viewer for RX output

## Flashing
- Open each file in Arduino IDE or your toolchain targeting your SI4463 setup.
- Flash `tx_optimised.c` to the transmitter board.
- Flash `rx_optimised.c` to the receiver board.

## Terminal Viewer
Make executable:
```bash
chmod +x /workspace/beacon_monitor.sh
```

Basic run (auto table refresh):
```bash
/workspace/beacon_monitor.sh
```

Specify serial port and baud:
```bash
/workspace/beacon_monitor.sh --port /dev/ttyUSB0 --baud 115200
```

Filter by transmitter ID (e.g., A1):
```bash
/workspace/beacon_monitor.sh --id A1
```

Show last N rows (rolling buffer size):
```bash
/workspace/beacon_monitor.sh --rows 50
```

Combine options:
```bash
/workspace/beacon_monitor.sh --port /dev/ttyACM0 --baud 115200 --id A1 --rows 100
```

Positional compatibility (port baud):
```bash
/workspace/beacon_monitor.sh /dev/ttyUSB0 115200
```

Exit: Ctrl+C

## Output Columns
`ID  DATE       TIME      LATITUDE    LONGITUDE    RCS  CCS  STATUS`

- `ID`: Transmitter ID (hex from RX)
- `RCS`: Received checksum (decimal)
- `CCS`: Calculated checksum (decimal)
- `STATUS`: VALID or CORRUPT

## Troubleshooting
- List ports: `ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null`
- Permission error: `sudo chmod 666 /dev/ttyUSB0`
- No updates: ensure RX prints lines beginning with `RESULT,` (RX shows `RX_READY` once started)
- Garbled data: check baud matches RX serial (default 115200)
- Too many lines: increase `--rows` or use `--id` to filter

## Notes
- Code is designed to be frugal: fixed-size buffers, simple sum checksum, minimal prints.
- You can adapt payload content, but keep the checksum and framing consistent for RX and script.
