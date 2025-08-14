#!/bin/bash

# Minimal RX table viewer for Arduino receiver output with filtering and last-N view
# Expects lines like:
# RESULT,A1,2025-08-13,17:30:15,12.345678,77.654321,123,123,VALID

PORT=/dev/ttyUSB0
BAUD=115200
FILTER_ID=""
max_rows=20
SHOW_HELP=0

usage() {
    cat <<EOF
Beacon Monitor - Minimal Viewer

Usage:
  $0 [PORT [BAUD]] [--port /dev/ttyUSB0] [--baud 115200] [--id A1] [--rows N] [--help]

Examples:
  $0
  $0 --port /dev/ttyUSB0 --baud 115200
  $0 --id A1 --rows 50
  $0 /dev/ttyACM0 115200 --id A1

Columns:
  ID  DATE       TIME      LATITUDE    LONGITUDE    RCS  CCS  STATUS

Notes:
  - Input lines must start with: RESULT,ID,DATE,TIME,LAT,LON,RECV_CS,CALC_CS,STATUS
  - Use Ctrl+C to exit
EOF
}

# Backward-compatible positional args: PORT BAUD
# New flags: --port/-p, --baud/-b, --id, --rows, --help
parse_args() {
    local positional=()
    while [ $# -gt 0 ]; do
        case "$1" in
            -p|--port)
                PORT="$2"; shift 2;;
            -b|--baud)
                BAUD="$2"; shift 2;;
            --id)
                FILTER_ID="$2"; shift 2;;
            --rows)
                max_rows="$2"; shift 2;;
            -h|--help)
                SHOW_HELP=1; shift;;
            *)
                positional+=("$1"); shift;;
        esac
    done
    if [ ${#positional[@]} -ge 1 ]; then PORT="${positional[0]}"; fi
    if [ ${#positional[@]} -ge 2 ]; then BAUD="${positional[1]}"; fi
}

print_header() {
    echo "ID  DATE       TIME      LATITUDE    LONGITUDE    RCS  CCS  STATUS"
    echo "--- ---------- ---------- ----------- ----------- ---- ---- -------"
}

print_row() {
    local id="$1" date="$2" time="$3" lat="$4" lon="$5" rcs="$6" ccs="$7" status="$8"
    printf "%-3s %-10s %-10s %-11s %-11s %4s %4s %-7s\n" "$id" "$date" "$time" "$lat" "$lon" "$rcs" "$ccs" "$status"
}

clear_screen() {
    printf "\033[2J\033[H"
}

monitor() {
    if [ ! -e "$PORT" ]; then
        echo "Port not found: $PORT" >&2
        exit 1
    fi
    stty -F "$PORT" "$BAUD" cs8 -cstopb -parenb raw -echo || exit 1

    clear_screen
    print_header

    rows=()
    while IFS= read -r line; do
        if [[ "$line" =~ ^RESULT, ]]; then
            IFS=',' read -r tag id date time lat lon rcs ccs status <<< "$line"
            if [ -n "$FILTER_ID" ] && [ "$id" != "$FILTER_ID" ]; then
                continue
            fi
            row=$(printf "%s,%s,%s,%s,%s,%s,%s,%s" "$id" "$date" "$time" "$lat" "$lon" "$rcs" "$ccs" "$status")
            rows+=("$row")
            if [ ${#rows[@]} -gt $max_rows ]; then
                rows=("${rows[@]: -$max_rows}")
            fi
            clear_screen
            print_header
            for r in "${rows[@]}"; do
                IFS=',' read -r i d t la lo rc cc st <<< "$r"
                print_row "$i" "$d" "$t" "$la" "$lo" "$rc" "$cc" "$st"
            done
        fi
    done < "$PORT"
}

parse_args "$@"
if [ "$SHOW_HELP" -eq 1 ]; then
    usage
    exit 0
fi
monitor
