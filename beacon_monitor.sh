#!/bin/bash

# SI4463 Radio Beacon Simple Monitor
# Uses only basic CLI tools available on any Unix/Linux system

# Configuration
DEFAULT_PORT="/dev/ttyUSB0"
DEFAULT_BAUD="115200"
LOG_FILE="beacon_data.log"
FILTER_FILE="beacon_filter.tmp"

# Function to print plain output
print_status() {
    echo "[INFO] $1"
}

print_error() {
    echo "[ERROR] $1"
}

print_warning() {
    echo "[WARN] $1"
}

print_data() {
    echo "[DATA] $1"
}

# Function to show help
show_help() {
    cat << 'EOF'

SI4463 GPS BEACON TRACKER - Field Monitoring Tool

BASIC COMMANDS:
  ./beacon_monitor.sh start [port] [baud]     - Start GPS tracking (default: /dev/ttyUSB0 115200)
  ./beacon_monitor.sh stop                    - Stop tracking
  ./beacon_monitor.sh status                  - Show system status
  
VIEW GPS DATA:
  ./beacon_monitor.sh show                    - Show all GPS positions
  ./beacon_monitor.sh show BEACON_A1          - Show only Beacon A1 positions
  ./beacon_monitor.sh show BEACON_A2          - Show only Beacon A2 positions  
  ./beacon_monitor.sh tail                    - Watch live GPS data
  ./beacon_monitor.sh last [count]            - Show last N positions (default: 10)
  
FILTER BY BEACON:
  ./beacon_monitor.sh filter BEACON_A1        - Only track Beacon A1
  ./beacon_monitor.sh filter BEACON_A2        - Only track Beacon A2
  ./beacon_monitor.sh filter clear            - Track both beacons
  ./beacon_monitor.sh list                    - Show detected beacons
  
SAVE GPS DATA:
  ./beacon_monitor.sh export                  - Save all GPS data to CSV
  ./beacon_monitor.sh export BEACON_A1        - Save only Beacon A1 data
  ./beacon_monitor.sh count                   - Count total GPS positions
  ./beacon_monitor.sh count BEACON_A1         - Count positions from Beacon A1

FIELD EXAMPLES:
  ./beacon_monitor.sh start                   - Begin GPS tracking
  ./beacon_monitor.sh tail                    - Watch GPS positions live
  ./beacon_monitor.sh show BEACON_A1          - Check Beacon A1 locations
  ./beacon_monitor.sh export BEACON_A1 > a1_track.csv  - Save A1 path

EOF
}

# Function to check if monitoring is running
is_monitoring() {
    if [ -f "beacon_monitor.pid" ]; then
        PID=$(cat beacon_monitor.pid)
        if kill -0 "$PID" 2>/dev/null; then
            return 0
        else
            rm -f beacon_monitor.pid
            return 1
        fi
    fi
    return 1
}

# Function to start monitoring
start_monitoring() {
    local port=${1:-$DEFAULT_PORT}
    local baud=${2:-$DEFAULT_BAUD}
    
    if is_monitoring; then
        print_warning "Already monitoring. Use 'stop' first."
        return 1
    fi
    
    # Check if port exists
    if [ ! -e "$port" ]; then
        print_error "Serial port $port not found!"
        echo "Available serial ports:"
        ls /dev/tty{USB,ACM,S}* 2>/dev/null | head -10
        return 1
    fi
    
    # Check if we have permission to access the port
    if [ ! -r "$port" ] || [ ! -w "$port" ]; then
        print_error "No permission to access $port. Try: sudo chmod 666 $port"
        return 1
    fi
    
    print_status "Starting beacon monitor on $port at $baud baud..."
    
    # Configure serial port
    stty -F "$port" "$baud" cs8 -cstopb -parenb raw -echo
    
    if [ $? -ne 0 ]; then
        print_error "Failed to configure serial port"
        return 1
    fi
    
    # Start monitoring in background
    {
        while true; do
            # Read data from serial port and add timestamp
            while IFS= read -r line; do
                timestamp=$(date '+%Y-%m-%d %H:%M:%S')
                echo "[$timestamp] $line" >> "$LOG_FILE"
            done < "$port"
            sleep 0.1
        done
    } &
    
    # Save PID
    echo $! > beacon_monitor.pid
    
    print_status "Monitoring started (PID: $!)"
    print_status "Data logging to: $LOG_FILE"
    print_status "Use './beacon_monitor.sh tail' to see live data"
}

# Function to stop monitoring
stop_monitoring() {
    if is_monitoring; then
        PID=$(cat beacon_monitor.pid)
        kill "$PID" 2>/dev/null
        rm -f beacon_monitor.pid
        print_status "Monitoring stopped"
    else
        print_warning "Not currently monitoring"
    fi
}

# Function to show status
show_status() {
    if is_monitoring; then
        PID=$(cat beacon_monitor.pid)
        print_status "Monitoring active (PID: $PID)"
        if [ -f "$LOG_FILE" ]; then
            total_lines=$(wc -l < "$LOG_FILE")
            print_status "Total data entries: $total_lines"
            if [ "$total_lines" -gt 0 ]; then
                print_status "Latest entry:"
                tail -n 1 "$LOG_FILE" | while read line; do
                    print_data "$line"
                done
            fi
        fi
    else
        print_warning "Not currently monitoring"
    fi
}

# Function to parse beacon data from log line
parse_beacon_data() {
    local line="$1"
    
    # Extract the data part (remove timestamp)
    data=$(echo "$line" | sed 's/^\[[^]]*\] //')
    
    # Handle the specific format from our RX code: BEACON_A1,lat,lng,rssi,status
    if echo "$data" | grep -q "^BEACON_A[12]"; then
        beacon_id=$(echo "$data" | cut -d',' -f1)
    elif echo "$data" | grep -q "^STATUS"; then
        beacon_id="STATUS"
    elif echo "$data" | grep -q "^RX_READY"; then
        beacon_id="SYSTEM"
    # Fallback to original parsing for other formats
    elif echo "$data" | grep -q ','; then
        beacon_id=$(echo "$data" | cut -d',' -f1)
    else
        beacon_id=$(echo "$data" | awk '{print $1}')
    fi
    
    echo "$beacon_id"
}

# Function to format beacon data for display
format_beacon_data() {
    local line="$1"
    local full_timestamp=$(echo "$line" | sed -n 's/^\[\([^]]*\)\].*/\1/p')
    local data=$(echo "$line" | sed 's/^\[[^]]*\] //')
    
    # Format specific data types
    if echo "$data" | grep -q "^BEACON_A[12]"; then
        # BEACON_A1,15.123456,73.456789,VALID
        local beacon=$(echo "$data" | cut -d',' -f1)
        local lat=$(echo "$data" | cut -d',' -f2)
        local lng=$(echo "$data" | cut -d',' -f3)
        local status=$(echo "$data" | cut -d',' -f4)
        
        # Extract only time part for simpler display
        local time_part=$(echo "$full_timestamp" | cut -d' ' -f2)
        
        # Format for easy reading with time only (no RSSI)
        printf "%-8s | %-8s | %10s,%11s | %-7s\n" \
               "$time_part" "$beacon" "$lat" "$lng" "$status"
    
    elif echo "$data" | grep -q "^STATUS"; then
        # STATUS,123,A1:45/50,A2:30/35,UPTIME:1234
        local total=$(echo "$data" | cut -d',' -f2)
        local a1_stats=$(echo "$data" | cut -d',' -f3)
        local a2_stats=$(echo "$data" | cut -d',' -f4)
        local uptime=$(echo "$data" | cut -d',' -f5 | cut -d':' -f2)
        
        echo "--- SYSTEM STATUS at $full_timestamp ---"
        echo "Total packets: $total"
        echo "Beacon A1: $a1_stats"
        echo "Beacon A2: $a2_stats"
        echo "Uptime: ${uptime}s"
        echo "----------------------------------------"
    
    elif echo "$data" | grep -q "^RX_READY"; then
        echo ">>> SYSTEM STARTED at $full_timestamp - Listening for beacons A1 and A2 <<<"
    
    else
        # Default format for other data
        echo "$line"
    fi
}

# Function to show data
show_data() {
    local beacon_filter="$1"
    
    if [ ! -f "$LOG_FILE" ]; then
        print_warning "No data file found. Start monitoring first."
        return 1
    fi
    
    echo "GPS BEACON TRACKING DATA"
    echo "================================================================="
    echo "TIME     | BEACON   | COORDINATES (Lat,Lng)   | STATUS"
    echo "---------|----------|-------------------------|---------"
    
    if [ -n "$beacon_filter" ]; then
        echo "Filtering for beacon: $beacon_filter"
        echo "================================================================="
        
        while IFS= read -r line; do
            beacon_id=$(parse_beacon_data "$line")
            if [ "$beacon_id" = "$beacon_filter" ]; then
                format_beacon_data "$line"
            fi
        done < "$LOG_FILE"
    else
        # Show all data with formatting
        while IFS= read -r line; do
            beacon_id=$(parse_beacon_data "$line")
            # Skip status messages unless specifically requested
            if [ "$beacon_id" != "STATUS" ] && [ "$beacon_id" != "SYSTEM" ]; then
                format_beacon_data "$line"
            fi
        done < "$LOG_FILE"
    fi
    
    echo "================================================================="
}

# Function to show last N entries
show_last() {
    local count=${1:-10}
    
    if [ ! -f "$LOG_FILE" ]; then
        print_warning "No data file found. Start monitoring first."
        return 1
    fi
    
    echo "LAST $count GPS POSITIONS"
    echo "================================================================="
    echo "TIME     | BEACON   | COORDINATES (Lat,Lng)   | STATUS"
    echo "---------|----------|-------------------------|---------"
    
    tail -n "$count" "$LOG_FILE" | while IFS= read -r line; do
        beacon_id=$(parse_beacon_data "$line")
        # Only show actual beacon data, not system messages
        if echo "$beacon_id" | grep -q "BEACON_A"; then
            format_beacon_data "$line"
        fi
    done
    
    echo "================================================================="
}

# Function to tail live data
tail_data() {
    if [ ! -f "$LOG_FILE" ]; then
        print_warning "No data file found. Start monitoring first."
        return 1
    fi
    
    if ! is_monitoring; then
        print_warning "Not currently monitoring. Showing existing data only."
    fi
    
    echo "LIVE GPS TRACKING (Press Ctrl+C to stop)"
    echo "================================================================="
    echo "TIME     | BEACON   | COORDINATES (Lat,Lng)   | STATUS"
    echo "---------|----------|-------------------------|---------"
    
    tail -f "$LOG_FILE" | while IFS= read -r line; do
        format_beacon_data "$line"
    done
}

# Function to list all beacon IDs
list_beacons() {
    if [ ! -f "$LOG_FILE" ]; then
        print_warning "No data file found. Start monitoring first."
        return 1
    fi
    
    echo "DETECTED BEACON IDs"
    echo "----------------------------------------"
    
    # Extract and count unique beacon IDs
    while IFS= read -r line; do
        parse_beacon_data "$line"
    done < "$LOG_FILE" | grep -v '^$' | sort | uniq -c | while read count beacon_id; do
        echo "$beacon_id: $count entries"
    done
}

# Function to count entries
count_entries() {
    local beacon_filter="$1"
    
    if [ ! -f "$LOG_FILE" ]; then
        print_warning "No data file found."
        return 1
    fi
    
    if [ -n "$beacon_filter" ]; then
        count=0
        while IFS= read -r line; do
            beacon_id=$(parse_beacon_data "$line")
            if [ "$beacon_id" = "$beacon_filter" ]; then
                count=$((count + 1))
            fi
        done < "$LOG_FILE"
        echo "Beacon $beacon_filter: $count entries"
    else
        total=$(wc -l < "$LOG_FILE")
        echo "Total entries: $total"
    fi
}

# Function to export data
export_data() {
    local beacon_filter="$1"
    
    if [ ! -f "$LOG_FILE" ]; then
        print_warning "No data file found."
        return 1
    fi
    
    # Simple CSV header with time only
    echo "time,beacon_id,latitude,longitude,status,raw_data"
    
    if [ -n "$beacon_filter" ]; then
        while IFS= read -r line; do
            beacon_id=$(parse_beacon_data "$line")
            if [ "$beacon_id" = "$beacon_filter" ]; then
            timestamp=$(echo "$line" | sed -n 's/^\[\([^]]*\)\].*/\1/p')
            data=$(echo "$line" | sed 's/^\[[^]]*\] //')
            time_part=$(echo "$timestamp" | cut -d' ' -f2)
            
            # Parse GPS data if it's a beacon entry (time only)
            if echo "$data" | grep -q "^BEACON_A[12]"; then
                lat=$(echo "$data" | cut -d',' -f2)
                lng=$(echo "$data" | cut -d',' -f3)
                status=$(echo "$data" | cut -d',' -f4)
                echo "$time_part,$beacon_id,$lat,$lng,$status,\"$data\""
            else
                echo "$time_part,$beacon_id,,,\"$data\""
            fi
            fi
        done < "$LOG_FILE"
    else
        while IFS= read -r line; do
            beacon_id=$(parse_beacon_data "$line")
        timestamp=$(echo "$line" | sed -n 's/^\[\([^]]*\)\].*/\1/p')
        data=$(echo "$line" | sed 's/^\[[^]]*\] //')
        time_part=$(echo "$timestamp" | cut -d' ' -f2)
        
        # Parse GPS data if it's a beacon entry (time only)
        if echo "$data" | grep -q "^BEACON_A[12]"; then
            lat=$(echo "$data" | cut -d',' -f2)
            lng=$(echo "$data" | cut -d',' -f3)
            status=$(echo "$data" | cut -d',' -f4)
            echo "$time_part,$beacon_id,$lat,$lng,$status,\"$data\""
        else
            echo "$time_part,$beacon_id,,,\"$data\""
        fi
        done < "$LOG_FILE"
    fi
}

# Function to set filter
set_filter() {
    local beacon_id="$1"
    
    if [ "$beacon_id" = "clear" ]; then
        rm -f "$FILTER_FILE"
        print_status "Filter cleared. Showing all beacons."
    elif [ -n "$beacon_id" ]; then
        echo "$beacon_id" > "$FILTER_FILE"
        print_status "Filter set to beacon: $beacon_id"
        print_status "Use 'filter clear' to remove filter"
    else
        if [ -f "$FILTER_FILE" ]; then
            current_filter=$(cat "$FILTER_FILE")
            print_status "Current filter: $current_filter"
        else
            print_status "No filter set (showing all beacons)"
        fi
    fi
}

# Main script logic
case "${1:-help}" in
    "start")
        start_monitoring "$2" "$3"
        ;;
    "stop")
        stop_monitoring
        ;;
    "status")
        show_status
        ;;
    "show")
        # Check if filter is set
        filter_beacon=""
        if [ -f "$FILTER_FILE" ]; then
            filter_beacon=$(cat "$FILTER_FILE")
        fi
        # Use command argument or filter
        beacon_id="${2:-$filter_beacon}"
        show_data "$beacon_id"
        ;;
    "last")
        show_last "$2"
        ;;
    "tail")
        tail_data
        ;;
    "list")
        list_beacons
        ;;
    "count")
        count_entries "$2"
        ;;
    "export")
        export_data "$2"
        ;;
    "filter")
        set_filter "$2"
        ;;
    "help"|"--help"|"-h")
        show_help
        ;;
    *)
        echo "Unknown command: $1"
        show_help
        exit 1
        ;;
esac
