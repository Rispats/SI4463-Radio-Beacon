/*
This code takes the NMEA strings from the GPS module, parses through them and then constucts the string as 
mentioned in the README.md file.
*/

#include <SPI.h>
#define PACKET_START '$'
#define PACKET_STOP '*'
#define TX_A1 0xA1
#define TX_A2 0xA2
#define TX_A3 0xA3
#define RADIO_SHUTDOWN_PIN 7
#define RADIO_CHIP_SELECT_PIN 10

uint8_t radioConfig[] = {
    // Insert radio config array
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

void readRxFifo(uint8_t* buffer, uint8_t maxLength) {
    uint8_t index;
    digitalWrite(RADIO_CHIP_SELECT_PIN, LOW);
    SPI.transfer(0x77);
    for (index = 0; index < maxLength; index++) buffer[index] = SPI.transfer(0x00);
    digitalWrite(RADIO_CHIP_SELECT_PIN, HIGH);
}

void clearRxFifo() {
    uint8_t fifoClearCommand[] = { 0x15, 0x03 };
    sendCommand(fifoClearCommand, sizeof(fifoClearCommand));
    waitForCts();
}
    void startReceive() {
    uint8_t startRxCommand[] = { 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08 };
    sendCommand(startRxCommand, sizeof(startRxCommand));
    waitForCts();
}
// Compute 2-hex-digit checksum of given buffer (sum of bytes)
void computeChecksumHex(char* data, int len, char outHex[3]) {
    unsigned char checksum;
    int i;
    checksum = 0;
    for (i = 0; i < len; i++) checksum += data[i];
    sprintf(outHex, "%02X", checksum);
}
void setup() {
    pinMode(RADIO_SHUTDOWN_PIN, OUTPUT);
    pinMode(RADIO_CHIP_SELECT_PIN, OUTPUT);
    digitalWrite(RADIO_CHIP_SELECT_PIN, HIGH);
    Serial.begin(115200);
    SPI.begin();
    SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
    initializeRadio();
    startReceive();
    Serial.println("RX Ready: listening for TX_A1/TX_A2/TX_A3");
}

uint8_t rxBuffer[64];
char payloadStr[64];
char content[64];
char fullMessage[128];
char receivedChecksum[3];
char contentForChecksum[64];
char calculatedChecksum[3];
int payloadLen = 0;
int stopPos = -1;
int bracketStart = -1;
int bracketEnd = -1;
int i;
bool isValid;

void loop() {
    
// Clear variables for each loop iteration
    payloadLen = 0;
    stopPos = -1;
    bracketStart = -1;
    bracketEnd = -1;
    readRxFifo(rxBuffer, sizeof(rxBuffer));

// Check if we have a valid TX_ID in the first byte
    if (rxBuffer[0] == 0xA1 || rxBuffer[0] == 0xA2 || rxBuffer[0] == 0xA3) {
// Convert the payload bytes to string (skip TX_ID byte)
    for (i = 1; i < 64; i++) {
    if (rxBuffer[i] == 0x00) break; // Stop at null terminator
    payloadStr[payloadLen++] = rxBuffer[i];
}
    payloadStr[payloadLen] = '\0';
// Now parse the payload string: "$TX_XX,DD/MM/YYYY,HH:MM:SS,Lat,Long*[CS]"
    if (payloadStr[0] == PACKET_START) {
// Find the PACKET_STOP position
    for (i = 1; i < payloadLen; i++) {
        if (payloadStr[i] == PACKET_STOP) {
            stopPos = i;
            break;
        }
    }
        if (stopPos > 0) {
// Extract content INCLUDING PACKET_START and PACKET_STOP (with $ and *)
            memcpy(content, &payloadStr[0], stopPos + 1);
            content[stopPos + 1] = '\0';
// Find checksum brackets [CS]
    for (i = stopPos; i < payloadLen; i++) {
        if (payloadStr[i] == '[') {
            bracketStart = i;
}     else if (payloadStr[i] == ']') {
            bracketEnd = i;
            break;
        }
    }

    if (bracketStart > 0 && bracketEnd > bracketStart && (bracketEnd -
bracketStart) == 3) {
// Extract received checksum (2 hex digits)
    receivedChecksum[0] = payloadStr[bracketStart + 1];
    receivedChecksum[1] = payloadStr[bracketStart + 2];
    receivedChecksum[2] = '\0';
// Calculate checksum on content WITHOUT $ and * (for checksum calculation)
    memcpy(contentForChecksum, &payloadStr[1], stopPos - 1);
    contentForChecksum[stopPos - 1] = '\0';
    computeChecksumHex(contentForChecksum, strlen(contentForChecksum),
    calculatedChecksum);
    isValid = (strcmp(calculatedChecksum, receivedChecksum) == 0);
// Build the full message WITH $ and * included in content
// Format: "$TX_XX,DD/MM/YYYY,HH:MM:SS,Lat,Long*[RX_CS] RX:[RX_CS]
    CALC:[CALC_CS] [status: VALID/INVALID]"
    snprintf(fullMessage, sizeof(fullMessage), "%s[%s] RX:[%s] CALC:[%s] [status:
%s]",
content, receivedChecksum, receivedChecksum, calculatedChecksum,
isValid ? "VALID" : "INVALID");
Serial.println(fullMessage);
        }
      }
    }
  }
    clearRxFifo();
    startReceive();
    delay(400);
}

