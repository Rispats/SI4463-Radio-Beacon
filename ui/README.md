# Terminal Monitoring UI

C program for displaying real-time GPS data from multiple beacon transmitters.

## Compilation

```bash
gcc table_display.c -o table_display
```

## Usage

```bash
# Display all transmitters
./table_display

# Display only specific transmitters
./table_display TX_A1 TX_A3
```

## Requirements

- Linux/Unix system with `/dev/ttyUSB0` access
- GCC compiler
- Receiver connected and outputting data to serial port

## Notes

- Default serial port: `/dev/ttyUSB0`
- Baud rate: 115200
- Update rate: 1 second
- Supports TX_A1, TX_A2, TX_A3 by default (easily expandable)
