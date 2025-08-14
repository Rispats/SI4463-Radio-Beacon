#!/usr/bin/env bash
set -euo pipefail

# serial_view.sh
# Read RESULT lines from an Arduino receiver and display a continuously updating table.
# Usage: ./serial_view.sh [/dev/ttyACM0] [115200]

PORT=${1:-/dev/ttyACM0}
BAUD=${2:-115200}

if [[ ! -e "$PORT" ]]; then
  echo "Error: serial port '$PORT' does not exist." >&2
  echo "Hint: try /dev/ttyUSB0 or /dev/ttyACM0" >&2
  exit 1
fi

# Configure serial port (8N1, raw, no flow control)
stty -F "$PORT" $BAUD cs8 -cstopb -parenb -ixon -ixoff -crtscts -echo -icanon min 1 time 1 || {
  echo "Failed to configure serial port $PORT" >&2
  exit 1
}

# Print header once, then stream rows as they arrive
# We repeat the header every 30 rows for readability
HEADER() {
  printf "%-10s  %-8s  %12s  %13s  %-4s  %-4s  %-7s\n" "Date" "Time" "Latitude" "Longitude" "Rx" "Calc" "Status"
  printf "%-10s  %-8s  %12s  %13s  %-4s  %-4s  %-7s\n" "----------" "--------" "------------" "-------------" "----" "----" "-------"
}

HEADER

ROW_COUNT=0

# Use stdbuf to ensure line-buffered behavior
stdbuf -oL -eL cat "$PORT" | \
awk -v header_interval=30 '
BEGIN {
  FS="|"
}

# Only process lines starting with RESULT|
/^RESULT\|/ {
  # Reset fields
  date=""; timev=""; lat=""; lon=""; rxcs=""; calccs=""; status="";

  # Iterate key=value segments
  for (i = 2; i <= NF; i++) {
    split($i, kv, "=")
    key = kv[1]
    val = substr($i, length(key) + 2) # support values containing '='
    if (key == "date") date = val
    else if (key == "time") timev = val
    else if (key == "lat") lat = val
    else if (key == "lon") lon = val
    else if (key == "rx_checksum") rxcs = val
    else if (key == "calc_checksum") calccs = val
    else if (key == "status") status = val
  }

  # Print row
  printf "%-10s  %-8s  %12s  %13s  %-4s  %-4s  %-7s\n", date, timev, lat, lon, rxcs, calccs, status

  row_count++
  if (row_count % header_interval == 0) {
    printf "%-10s  %-8s  %12s  %13s  %-4s  %-4s  %-7s\n", "Date", "Time", "Latitude", "Longitude", "Rx", "Calc", "Status"
    printf "%-10s  %-8s  %12s  %13s  %-4s  %-4s  %-7s\n", "----------", "--------", "------------", "-------------", "----", "----", "-------"
  }
  fflush(stdout)
}
'