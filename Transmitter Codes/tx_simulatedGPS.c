/*
This code sends a simulated data string which follows the same format as mentioned in README.md file. This is done
because the simulated code occasionally sends corrupted data to test the functionality and robustness of the system.
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
    //Insert radio config array
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
    sendCommand(&radioConfig[configIndex], blockLength); // Configures radio
    configIndex += blockLength;
    waitForCts();
}
}

// Compute 2-hex-digit checksum of given buffer (sum of bytes)
void computeChecksumHex( char* data, int len, char outHex[3]) {
    unsigned char checksum;
    int i;
    checksum = 0;
    for (i = 0; i < len; i++) checksum += data[i]; // Summing ASCII values
    sprintf(outHex, "%02X", checksum);
}

void sendPacket(char* payloadWithCs, uint8_t transmitterId) {
    uint8_t txFifoCommandAndData[64];
    uint8_t payloadLength;
    uint8_t totalPacketBytes;
    uint8_t startTxCommand[] = { 0x31, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    payloadLength = strlen(payloadWithCs);
    totalPacketBytes = 2 + payloadLength; // Only TX_ID + payload (no separate
    start/stop)
    txFifoCommandAndData[0] = 0x66; // WRITE_TX_FIFO
    txFifoCommandAndData[1] = transmitterId;
    memcpy(&txFifoCommandAndData[2], payloadWithCs, payloadLength);
    sendCommand(txFifoCommandAndData, totalPacketBytes + 1);
    waitForCts();
    startTxCommand[4] = totalPacketBytes;
    sendCommand(startTxCommand, sizeof(startTxCommand));
    waitForCts();
}


void setup() {
    pinMode(RADIO_SHUTDOWN_PIN, OUTPUT);
    pinMode(RADIO_CHIP_SELECT_PIN, OUTPUT);
    digitalWrite(RADIO_CHIP_SELECT_PIN, HIGH);
    Serial.begin(115200);
    SPI.begin();
    SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
    initializeRadio();
    randomSeed(micros());
    Serial.println("TX Ready");
}

uint8_t transmitterId;
char content[96];
char payload[128];
char checksumHex[3];
char* txLabel;
int roll;
int corruptRoll;
int contentLen;
int corruptIndex;
int randHour;
int randMinute;
int randSecond;

void loop() {
// Generates a random time each transmission so the receiver shows updates
    randHour = random(0, 24);
    randMinute = random(0, 60);
    randSecond = random(0, 60);
// Simulates incoming signals from 3 different transmitters
    roll = random(100);
    if (roll < 33) {
    transmitterId = TX_A1;
    txLabel = "TX_A1";
    snprintf(content, sizeof(content), "%s,01/01/2025,%02d:%02d:%02d,29.979,31.134",
    txLabel, randHour, randMinute, randSecond);
} else if (roll < 66) {
    transmitterId = TX_A2;
    txLabel = "TX_A2";
    snprintf(content, sizeof(content), "%s,02/02/2025,%02d:%02d:%02d,25.195,55.274",
    txLabel, randHour, randMinute, randSecond);
} else {
    transmitterId = TX_A3;
    txLabel = "TX_A3";
    snprintf(content, sizeof(content), "%s,03/03/2025,%02d:%02d:%02d,40.008,28.980",
    txLabel, randHour, randMinute, randSecond);
}
    contentLen = strlen(content);
    computeChecksumHex(content, contentLen, checksumHex);
// Occasionally corrupt (about 30% chance)
    corruptRoll = random(100);
    if (corruptRoll < 30) {
// 50%: corrupt data, 50%: corrupt checksum
    if (random(2) == 0) {
// Corrupt a random character within content area (DON'T recalculate checksum)
    if (contentLen > 5) {
    corruptIndex = 5 + random(contentLen - 5);
    if (content[corruptIndex] != ',') content[corruptIndex] = '#';
}
// Don't recalculate checksum - this creates a mismatch for testing
}   else {
// Corrupt checksum digit
    checksumHex[0] = (checksumHex[0] == 'F') ? '0' : 'F';
}
}
// Final payload: $TX_XX,DD/MM/YYYY,HH:MM:SS,Lat,Long*[CS]
    snprintf(payload, sizeof(payload), "$%s*[%s]", content, checksumHex);
    sendPacket(payload, transmitterId); // Begins data transmission
    Serial.println(payload);
    delay(800);
}
