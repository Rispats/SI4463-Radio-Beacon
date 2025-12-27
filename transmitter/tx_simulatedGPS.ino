/*
 * GPS-Enabled RF Beacon Transmitter (Simulated GPS Data)
 * 
 * Sends simulated GPS data in the same format as real GPS transmitter.
 * Includes intentional data corruption (~20% of packets) to test system
 * robustness and checksum validation.
 * 
 * Hardware: Arduino Nano ESP32-S3 / ESP32-C3 + SI4463
 * Packet Format: $TX_XX,DD/MM/YYYY,HH:MM:SS,LAT,LONG*[CS]
 */

#include <SPI.h>

#define PACKET_START '$'
#define PACKET_STOP  '*'
#define TX_ID_A1 0xA1
#define TX_ID_A2 0xA2
#define TX_ID_A3 0xA3
#define RADIO_SHUTDOWN_PIN 7
#define RADIO_CHIP_SELECT_PIN 10

uint8_t radioConfig[] = { 
   0x07, 0x02, 0x01, 0x00, 0x01, 0xC9, 0xC3, 0x80,
  0x08, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x08, 0x11, 0x00, 0x04, 0x00, 0x52, 0x00, 0x18, 0x30,
  0x05, 0x11, 0x01, 0x01, 0x00, 0x00,
  0x07, 0x11, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00,
  0x05, 0x11, 0x10, 0x01, 0x04, 0x31,
  0x08, 0x11, 0x11, 0x04, 0x01, 0xB4, 0x2B, 0x00, 0x00,
  0x0B, 0x11, 0x12, 0x07, 0x00, 0x04, 0x01, 0x08, 0xFF, 0xFF, 0x20, 0x02,
  0x09, 0x11, 0x12, 0x05, 0x0E, 0x07, 0x04, 0x80, 0x00, 0x80,
  0x0A, 0x11, 0x20, 0x06, 0x00, 0x01, 0x00, 0x07, 0x01, 0x86, 0xA0,
  0x06, 0x11, 0x20, 0x02, 0x0B, 0x00, 0x00,
  0x10, 0x11, 0x20, 0x0C, 0x18, 0x00, 0x00, 0x08, 0x02, 0x80, 0x00, 0x16, 0x20, 0x00, 0xE8, 0x00, 0x5E,
  0x10, 0x11, 0x20, 0x0C, 0x24, 0x05, 0x76, 0x1A, 0x02, 0xB9, 0x02, 0xC0, 0x00, 0x00, 0x12, 0x01, 0x06,
  0x0A, 0x11, 0x20, 0x06, 0x30, 0x0D, 0x04, 0xA0, 0x00, 0x00, 0x60,
  0x0F, 0x11, 0x20, 0x0B, 0x39, 0x15, 0x15, 0x80, 0x02, 0xFF, 0xFF, 0x00, 0x28, 0x0C, 0xA4, 0x20,
  0x0D, 0x11, 0x20, 0x09, 0x45, 0x01, 0x07, 0xFF, 0x01, 0x00, 0xFF, 0x06, 0x00, 0x18,
  0x06, 0x11, 0x20, 0x02, 0x50, 0x94, 0x0D,
  0x06, 0x11, 0x20, 0x02, 0x54, 0x0B, 0x07,
  0x09, 0x11, 0x20, 0x05, 0x5B, 0x40, 0x04, 0x20, 0x78, 0x20,
  0x10, 0x11, 0x21, 0x0C, 0x01, 0xC4, 0x30, 0x7F, 0xF5, 0xB5, 0xB8, 0xDE, 0x05, 0x17, 0x16, 0x0C, 0x03,
  0x09, 0x11, 0x21, 0x05, 0x0D, 0x00, 0x15, 0xFF, 0x00, 0x00,
  0x0C, 0x11, 0x40, 0x08, 0x00, 0x3F, 0x0A, 0x51, 0xEB, 0xCC, 0xCD, 0x20, 0xFA
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

// Compute 2-hex-digit checksum of given buffer (sum of bytes)
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
  totalPacketBytes = 2 + payloadLength; // Only TX_ID + payload (no separate start/stop)

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

void loop() {
  char content[96];
  char payload[128];
  char checksumHex[3];
  //char expectedChecksum[3];
  uint8_t transmitterId;
  int roll;
  const char* txLabel;
  int corruptRoll;
  int contentLen;
  int corruptIndex;

  // Generate a random time each transmission so the receiver shows updates
  int randHour = random(0, 24);
  int randMinute = random(0, 60);
  int randSecond = random(0, 60);

  roll = random(100);
  if (roll < 60) {
    transmitterId = TX_ID_A1;
    txLabel = "TX_A1";
    snprintf(content, sizeof(content), "%s,01/01/2025,%02d:%02d:%02d,29.979,31.134", txLabel, randHour, randMinute, randSecond);
  } else if (roll < 90) {
    transmitterId = TX_ID_A2;
    txLabel = "TX_A2";
    snprintf(content, sizeof(content), "%s,02/02/2025,%02d:%02d:%02d,25.195,55.274", txLabel, randHour, randMinute, randSecond);
  } else {
    transmitterId = TX_ID_A3;
    txLabel = "TX_A3";
    snprintf(content, sizeof(content), "%s,03/03/2025,%02d:%02d:%02d,40.008,28.980", txLabel, randHour, randMinute, randSecond);
  }

  contentLen = strlen(content);
  computeChecksumHex(content, contentLen, checksumHex);
  //strcpy(expectedChecksum, checksumHex); // Store the expected checksum

// Occasionally corrupt (about 20% chance)
  corruptRoll = random(100);
  if (corruptRoll < 20) {
    // 50%: corrupt data, 50%: corrupt checksum
    if (random(2) == 0) {
      // Corrupt a random character within content area
      if (contentLen > 5) {
        corruptIndex = 5 + random(contentLen - 5);
        if (content[corruptIndex] != ',') content[corruptIndex] = '#';
      }
      // Recalculate checksum after corruption
      computeChecksumHex(content, contentLen, checksumHex);
    } else {
      // Corrupt checksum digit (keep content unchanged)
      checksumHex[0] = (checksumHex[0] == 'F') ? '0' : 'F';
    }
  }


  // Final payload: "$content*[calc_cs|expec_cs]"
  snprintf(payload, sizeof(payload), "$%s*[%s]\r", content, checksumHex);

  sendPacket(payload, transmitterId);
  Serial.println(payload);
  delay(800);
}
