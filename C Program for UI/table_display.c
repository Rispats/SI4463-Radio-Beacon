/*
Only three trasnmitter IDs have been added for the display testing.
Arguments can be passed while executing the C program through terminal for filtering the transmitter IDs.
*/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

void parse_gps_data(char* gps_string, char* date, char* time, char* lat, char* lon) {
    char* token;
    char temp_string[128];
    int field = 0;
   
    memcpy(temp_string, gps_string,128);
    
    token = strtok(temp_string, ",");
    while (token != NULL && field < 4) {
        switch(field) {
            case 0: 
                break;
            case 1: // Date
                strcpy(date, token);
                break;
            case 2: // Time
                strcpy(time, token);
                break;
            case 3: // Latitude
                strcpy(lat, token);
                break;
            case 4: // Longitude (if we get here)
                strcpy(lon, token);
                break;
        }
        token = strtok(NULL, ",");
        field++;
    }
    
    // If we have a 4th token, it might contain longitude
    if (token != NULL) {
        strcpy(lon, token);
    }
}

void print_transmitter_table( char* tx_name, char* gps_data) {

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
    printf("Lat,Long: %s,%s\n",lat,lon);
    printf("\n\n");
   
}

int main(void) {
    
    char t0[128];
    char last_str1[128] = "";
    char last_str2[128] = "";
    char last_str3[128] = "";
    char new_str1[128] = "";
    char new_str2[128] = "";
    char new_str3[128] = "";
    FILE *fs;
    
    printf("GPS Tracker Monitor Started...\n");
    printf("Press Shift+C to exit\n\n");
    
    fs = fopen("/dev/ttyUSB0", "r");
    if (fs == NULL) {
        printf("Serial port open error\n");
        return 1;
    }
    
    while (1) {
        if (fgets(t0, 128, fs) != NULL) {

            if (memcmp(t0, "$TX_A1", 6) == 0) {
                strcpy(last_str1, t0);
            }
            if (memcmp(t0, "$TX_A2", 6) == 0) {
                strcpy(last_str2, t0);
            }
            if (memcmp(t0, "$TX_A3", 6) == 0) {
                strcpy(last_str3, t0);
            }
            
            for (int i = 0; i < strlen(last_str1); i++) {
                if (last_str1[i] == '*') {
                    strncpy(new_str1, last_str1, i);
                    new_str1[i] = '\0';  
                    break;
                }
            }
            
            for (int i = 0; i < strlen(last_str2); i++) {
                if (last_str2[i] == '*') {  
                    strncpy(new_str2, last_str2, i);
                    new_str2[i] = '\0';  
                    break;
                }
            }
            
            for (int i = 0; i < strlen(last_str3); i++) {
                if (last_str3[i] == '*') {  
                    strncpy(new_str3, last_str3, i);
                    new_str3[i] = '\0';  
                    break;
                }
            }
     
            printf("\033[H\033[J");  // Clear screen
            
            // Print tables for each transmitter
            print_transmitter_table("TRANSMITTER A1", new_str1);
            print_transmitter_table("TRANSMITTER A2", new_str2);
            print_transmitter_table("TRANSMITTER A3", new_str3);
            printf("Last Update: %s", t0);
            fflush(stdout);
        }
        sleep(1);
    }
    
    fclose(fs);
    printf("Exit\n");
    return 0;
}
