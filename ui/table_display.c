/*
 * GPS Beacon Monitor - Terminal UI
 * 
 * Displays real-time GPS data from multiple beacon transmitters in a
 * formatted table on the terminal. Supports filtering by transmitter ID.
 * 
 * Usage: ./table_display [TX_A1] [TX_A2] [TX_A3]
 * If no arguments provided, displays all transmitters.
 * 
 * Compilation: gcc table_display.c -o table_display
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

// Function to parse GPS data and extract components
void parse_gps_data(char* gps_string, char* date, char* time, char* lat, char* lon) {
    char* token;
    char temp_string[128];
    int field = 0;
    
    // Initialize output strings
    memcpy(date, "N/A", 3);
    memcpy(time, "N/A", 3);
    memcpy(lat, "N/A", 3);
    memcpy(lon, "N/A", 3);
    
    // Make a copy of the string for tokenization
    memcpy(temp_string, gps_string, 128);
    
    // Parse the comma-separated values
    token = strtok(temp_string, ",");
    while (token != NULL && field < 4) {
        switch(field) {
            case 0: // Skip the $TX_A1 part
                break;
            case 1: // Date
                memcpy(date, token, 32);
                break;
            case 2: // Time
                memcpy(time, token, 32);
                break;
            case 3: // Latitude
                memcpy(lat, token, 32);
                break;
            case 4: // Longitude (if we get here)
                memcpy(lon, token, 32);
                break;
        }
        token = strtok(NULL, ",");
        field++;
    }
    
    // If we have a 4th token, it might contain longitude
    if (token != NULL) {
        memcpy(lon, token, 32);
    }
}

// Function to print a formatted table for a transmitter
void print_transmitter_table(const char* tx_name, char* gps_data) {
    char date[32], time[32], lat[32], lon[32];
    
    printf("%s\n", tx_name);
 
    
    if (strlen(gps_data) == 0) {
        printf("No data received\n");
        printf("\n");
        return;
    }
    
    parse_gps_data(gps_data, date, time, lat, lon);
    
    printf("Date: %s\n", date);
    printf("Time: %s\n", time);
    printf("Lat,Long:");
    printf(" %s,%s\n\n", lat, lon);
    
    
}

int main(int argc, char* argv[]) {
    char t0[128];
    char last_str1[128] = "";
    char last_str2[128] = "";
    char last_str3[128] = "";
    char new_str1[128] = "";
    char new_str2[128] = "";
    char new_str3[128] = "";
    FILE *fs;
    
        int show_a1 = 1, show_a2 = 1, show_a3 = 1;
    if (argc > 1) {
        show_a1 = show_a2 = show_a3 = 0;
        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], "TX_A1") == 0) show_a1 = 1;
            if (strcmp(argv[i], "TX_A2") == 0) show_a2 = 1;
            if (strcmp(argv[i], "TX_A3") == 0) show_a3 = 1;
        }
    }
    printf("GPS Tracker Monitor Started...\n");
    printf("Press Ctrl+C to exit\n\n");
    
    fs = fopen("/dev/ttyUSB0", "r");
    if (fs == NULL) {
        printf("Serial port open error\n");
        return 1;
    }
    
    while (1) {
        if (fgets(t0, 128, fs) != NULL) {
            // Store complete strings for each transmitter
            if (show_a1 && memcmp(t0, "$TX_A1", 6) == 0) {
                strcpy(last_str1, t0);
            }
            if (show_a2 && memcmp(t0, "$TX_A2", 6) == 0) {
                strcpy(last_str2, t0);
            }
            if (show_a3 && memcmp(t0, "$TX_A3", 6) == 0) {
                strcpy(last_str3, t0);
            }
            
            
            // Remove asterisk and everything after it for each string
            char* strings[] = {last_str1, last_str2, last_str3};
            char* new_strings[] = {new_str1, new_str2, new_str3};
            
            for (int j = 0; j < 3; j++) {
                for (int i = 0; i < strlen(strings[j]); i++) {
                    if (strings[j][i] == '*') {
                        memcpy(new_strings[j], strings[j], i);
                        new_strings[j][i] = '\0';  // Null terminate
                        break;
                    }
                }
            }
            
            // Clear screen and display formatted tables
            printf("\033[H\033[J");  // Clear screen
            
            // Print tables for each transmitter
            if (show_a1) print_transmitter_table("TRANSMITTER A1", new_str1);
            if (show_a2) print_transmitter_table("TRANSMITTER A2", new_str2);
            if (show_a3) print_transmitter_table("TRANSMITTER A3", new_str3);
            
            printf("Last Update: %s", t0);
            
            fflush(stdout);
        }
        sleep(1);
    }
    
    fclose(fs);
    printf("Exit\n");
    return 0;
}
