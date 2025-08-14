#!/bin/bash

# Minimal RX table viewer for Arduino receiver output
# Expects lines like:
# RESULT,A1,2025-08-13,17:30:15,12.345678,77.654321,123,123,VALID

PORT=${1:-/dev/ttyUSB0}
BAUD=${2:-115200}

if [ ! -e "$PORT" ]; then
    echo "Port not found: $PORT" >&2
    exit 1
fi

stty -F "$PORT" "$BAUD" cs8 -cstopb -parenb raw -echo || exit 1

print_header() {
    echo "DATE       TIME      LATITUDE    LONGITUDE    RCS  CCS  STATUS"
    echo "---------- ---------- ----------- ----------- ---- ---- -------"
}

print_row() {
    local date="$1" time="$2" lat="$3" lon="$4" rcs="$5" ccs="$6" status="$7"
    printf "%-10s %-10s %-11s %-11s %4s %4s %-7s\n" "$date" "$time" "$lat" "$lon" "$rcs" "$ccs" "$status"
}

clear_screen() {
    printf "\033[2J\033[H"
}

clear_screen
print_header

# Maintain a small rolling buffer in memory (last 20 rows)
max_rows=20
rows=()

while IFS= read -r line; do
    # Only handle RESULT lines
    if [[ "$line" =~ ^RESULT, ]]; then
        # RESULT,ID,DATE,TIME,LAT,LON,RECV_CS,CALC_CS,STATUS
        IFS=',' read -r tag id date time lat lon rcs ccs status <<< "$line"
        row=$(printf "%s,%s,%s,%s,%s,%s,%s" "$date" "$time" "$lat" "$lon" "$rcs" "$ccs" "$status")
        rows+=("$row")
        # Trim to max_rows
        if [ ${#rows[@]} -gt $max_rows ]; then
            rows=("${rows[@]: -$max_rows}")
        fi
        # Redraw
        clear_screen
        print_header
        for r in "${rows[@]}"; do
            IFS=',' read -r d t la lo rc cc st <<< "$r"
            print_row "$d" "$t" "$la" "$lo" "$rc" "$cc" "$st"
        done
    fi
# Read from serial port
done < "$PORT"
