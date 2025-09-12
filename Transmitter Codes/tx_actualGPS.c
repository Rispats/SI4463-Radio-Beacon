#include <SPI.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>

// GPS Configuration
#define GPS_RX_PIN 4  // Connect GPS TX to Arduino Nano pin 4
#define GPS_TX_PIN 3  // Connect GPS RX to Arduino Nano pin 3
#define GPS_BAUD 115200 

// Radio Configuration (unchanged)
#define PACKET_START '$'
#define PACKET_STOP  '*'
#define TX_ID_A1 0xA1
#define TX_ID_A2 0xA2
#define TX_ID_A3 0xA3
#define RADIO_SHUTDOWN_PIN 7
#define RADIO_CHIP_SELECT_PIN 10


// GPS Objects
TinyGPSPlus gps;
HardwareSerial gpsSerial(GPS_RX_PIN, GPS_TX_PIN);


uint8_t radioConfig[] = { 
    //Insert Radio Config Array Here
};

void sendCommand(uint8_t* data, uint8_t length) {
  uint8_t index;
  digitalWrite(RADIO_CHIP_SELECT_PIN, LOW);
  for (index = 0; index < length; index++) SPI.transfer(data[index]);
  digitalWrite(RADIO_CHIP_SELECT_PIN, HIGH);
}

bool waitForCts() {
  uint8_t attemptIndex;
  uint8_t response;
  for (attemptIndex = 0; attemptIndex < 100; attemptIndex++) {
    digitalWrite(RADIO_CHIP_SELECT_PIN, LOW);
    SPI.transfer(0x44);
    delayMicroseconds(20);
    response = SPI.transfer(0x00);
    digitalWrite(RADIO_CHIP_SELECT_PIN, HIGH);
    if (response == 0xFF) return true;
    delay(5);
  }
  return false;
}

void initializeRadio() {
  uint16_t configIndex;
  uint8_t blockLength;
  digitalWrite(RADIO_SHUTDOWN_PIN, HIGH); delay(50);
  digitalWrite(RADIO_SHUTDOWN_PIN, LOW); delay(50);
  digitalWrite(RADIO_CHIP_SELECT_PIN, HIGH);
  for (configIndex = 0; configIndex < sizeof(radioConfig); ) {
    blockLength = radioConfig[configIndex++];
    if (blockLength == 0x00) break;
    sendCommand(&radioConfig[configIndex], blockLength);
    configIndex += blockLength;
    waitForCts();
  }
}


void computeChecksumHex(const char* data, int len, char outHex[3]) {
  unsigned char checksum;
  int i;
  checksum = 0;
  for (i = 0; i < len; i++) checksum += data[i];
  sprintf(outHex, "%02X", checksum);
}

void sendPacket(char* payloadWithCs, uint8_t transmitterId) {
  uint8_t txFifoCommandAndData[64];
  uint8_t payloadLength;
  uint8_t totalPacketBytes;
  uint8_t startTxCommand[] = { 0x31, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

  payloadLength = strlen(payloadWithCs);
  totalPacketBytes = 2 + payloadLength;

  txFifoCommandAndData[0] = 0x66;
  txFifoCommandAndData[1] = transmitterId;
  memcpy(&txFifoCommandAndData[2], payloadWithCs, payloadLength);

  sendCommand(txFifoCommandAndData, totalPacketBytes + 1);
  waitForCts();

  startTxCommand[4] = totalPacketBytes;
  sendCommand(startTxCommand, sizeof(startTxCommand));
  waitForCts();
}

void setup() {
 
  Serial.begin(115200);
  gpsSerial.begin(GPS_BAUD);
  

  pinMode(RADIO_SHUTDOWN_PIN, OUTPUT);
  pinMode(RADIO_CHIP_SELECT_PIN, OUTPUT);
  digitalWrite(RADIO_CHIP_SELECT_PIN, HIGH);
  
 
  SPI.begin();
  SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
  initializeRadio();
  
  Serial.println("TX Ready with Real GPS");
  Serial.print("Transmitter ID: ");
  Serial.println(THIS_TX_LABEL);
  Serial.println("Waiting for GPS fix...");
}

void loop() {
  char content[96];
  char payload[128];
  char checksumHex[3];
  char expectedChecksum[3];
  char dateStr[11];  
  char timeStr[9];   
  char latStr[10];   
  char lonStr[10];   
  
  // Variables for GPS status
  static unsigned long lastTransmit = 0;
  static bool gpsFixed = false;
  
  // Feed GPS data to TinyGPS++
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }
  
  // Check if we have valid GPS data and enough time has passed
  if (millis() - lastTransmit > 1000) { // Transmit every 1 second
    
    if (gps.location.isValid() && gps.date.isValid() && gps.time.isValid()) {
      if (!gpsFixed) {
        Serial.println("GPS Fix acquired!");
        gpsFixed = true;
      }
      
      // Format date as DD/MM/YYYY
      snprintf(dateStr, sizeof(dateStr), "%02d/%02d/%04d", 
               gps.date.day(), gps.date.month(), gps.date.year());
      
      // Format time as HH:MM:SS
      snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d",
               gps.time.hour(), gps.time.minute(), gps.time.second());
      
      // Format latitude with 3 decimal places
      dtostrf(gps.location.lat(), 6, 3, latStr);
      // Format longitude with 3 decimal places  
      dtostrf(gps.location.lng(), 6, 3, lonStr);
      
      // Build content string: "TX_Ax,DD/MM/YYYY,HH:MM:SS,lat,long"
      snprintf(content, sizeof(content), "%s,%s,%s,%s,%s", 
               THIS_TX_LABEL, dateStr, timeStr, latStr, lonStr);
      
    } else {
      // No GPS fix - send placeholder data with current time
      if (gpsFixed) {
        Serial.println("N");
        gpsFixed = false;
      }
      
      // Use placeholder values when no GPS fix
      snprintf(content, sizeof(content), "%s,01/01/2025,00:00:00,0.000,0.000", 
               THIS_TX_LABEL);
    }
    
    // Calculate checksum
    int contentLen = strlen(content);
    computeChecksumHex(content, contentLen, checksumHex);
    strcpy(expectedChecksum, checksumHex);
    
    // Build final payload: "$content*[CS]"
    snprintf(payload, sizeof(payload), "$%s*[%s]\r",content, checksumHex);
    
    // Send packet
    sendPacket(payload, THIS_TX_ID);
    
    // Debug output
    Serial.print("TX: ");
    Serial.println(payload);
    
  /* Uncomment this part if needed for additional GPS info.  
  
    if (gps.location.isValid()) {
      Serial.print("Satellites: ");
      Serial.print(gps.satellites.value());
      Serial.print(", HDOP: ");
      Serial.println(gps.hdop.value());
    } else {
      Serial.println("Waiting for GPS fix...");
    }
  */
    lastTransmit = millis();
  }
}
