/*
 * Continuous Wave Transmitter for SI4463
 * 
 * Transmits continuous wave (CW) beacon signal for radio direction finding.
 * Note: No receiver code is provided as CW detection was performed using
 * a commercial direction finder (DF500N).
 * 
 * Hardware: Arduino Nano ESP32-S3 / ESP32-C3 + SI4463 RF Module
 * Modulation: OOK (On-Off Keying) for CW generation
 */

#include <SPI.h>

#define RADIO_SHUTDOWN_PIN 7
#define RADIO_CHIP_SELECT_PIN 10


uint8_t radioConfig[] = {
    // Insert radio config array with an OOK modulation scheme only; frequency of your choice
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
    for (attemptIndex = 0; attemptIndex < 50; attemptIndex++) {
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
    digitalWrite(RADIO_SHUTDOWN_PIN, HIGH); delay(20);
    digitalWrite(RADIO_SHUTDOWN_PIN, LOW); delay(20);
    digitalWrite(RADIO_CHIP_SELECT_PIN, HIGH);
    for (configIndex = 0; configIndex < sizeof(radioConfig); ) {
    blockLength = radioConfig[configIndex++];
    if (blockLength == 0x00) break;
    sendCommand(&radioConfig[configIndex], blockLength);
    configIndex += blockLength;
    waitForCts();
    }
}

void transmitBegin(uint8_t data) {
  uint8_t fifo[] = { 0x66, data };  // 0x66 = WRITE_TX_FIFO
  sendCommand(fifo, sizeof(fifo));
  waitForCts();

  uint8_t tx_cmd[] = { 0x31, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00 }; // channel 0, 1-byte packet
  sendCommand(tx_cmd, sizeof(tx_cmd));
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
  Serial.println("Transmitter ready...");
}
  
uint8_t bit =  0xFF; // All 1's for OOK -> Continous Wave

void loop() {
  
  Serial.println(bit, BIN);
  transmitBegin(bit);
  delay(50); // Change delay to set burst period
}
