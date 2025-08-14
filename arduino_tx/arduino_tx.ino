#include <SoftwareSerial.h>

// Transmitter sketch
// - Sends messages in the format: ^<PAYLOAD>$<CHECKSUM>\r\n
// - PAYLOAD: date,time,latitude,longitude
// - CHECKSUM: two-digit uppercase hex of XOR across PAYLOAD bytes
// - Sends every 2000 ms
// - Deliberately corrupts 1 of every 3 messages by altering the payload after checksum is computed

// CONFIGURE: Pins connected to the radio module on the transmitter board
// RX pin is unused here but required by SoftwareSerial. Keep a free pin.
static const uint8_t RADIO_RX_PIN = 8;   // Not used (module TX -> this RX) - can be left unconnected on pure TX
static const uint8_t RADIO_TX_PIN = 9;   // Module RX <- this TX

// Baud rates
static const unsigned long RADIO_BAUD = 9600;     // Typical for many serial RF modules
static const unsigned long USB_BAUD   = 115200;   // For debug over USB

// Send interval in milliseconds
static const unsigned long SEND_INTERVAL_MS = 2000;

SoftwareSerial RadioSerial(RADIO_RX_PIN, RADIO_TX_PIN); // RX, TX

unsigned long lastSendMillis = 0;
unsigned long messageCounter = 0;

// Dummy but realistic GPS-like payload values
// You can change these if desired. They are kept static for test repeatability.
static const char* BASE_DATE = "2025-08-14";
static const char* BASE_TIME = "12:34:56";
static const char* BASE_LAT  = "37.4219999";
static const char* BASE_LON  = "-122.0840575";

// Compute XOR checksum across all bytes of the payload
static uint8_t computeXorChecksum(const char* payload) {
  uint8_t checksum = 0;
  for (const char* p = payload; *p != '\0'; ++p) {
    checksum ^= static_cast<uint8_t>(*p);
  }
  return checksum;
}

static void checksumToHex(uint8_t checksum, char out2Hex[3]) {
  const char hexChars[] = "0123456789ABCDEF";
  out2Hex[0] = hexChars[(checksum >> 4) & 0x0F];
  out2Hex[1] = hexChars[checksum & 0x0F];
  out2Hex[2] = '\0';
}

// Create the payload string: date,time,lat,lon
static void buildPayload(char* outBuffer, size_t outBufferSize) {
  // Keep the example deterministic
  snprintf(outBuffer, outBufferSize, "%s,%s,%s,%s", BASE_DATE, BASE_TIME, BASE_LAT, BASE_LON);
}

// Introduce a deterministic corruption into the payload: flip one digit near the end
static void corruptPayload(char* payload) {
  size_t len = strlen(payload);
  if (len == 0) return;

  // Find a character near the end that is a digit and change it
  for (int i = static_cast<int>(len) - 1; i >= 0; --i) {
    if (payload[i] >= '0' && payload[i] <= '9') {
      // Change digit in a reversible but deterministic way
      payload[i] = (payload[i] == '9') ? '0' : static_cast<char>(payload[i] + 1);
      return;
    }
  }

  // Fallback: flip the last character if no digit was found
  payload[len - 1] = (payload[len - 1] == 'X') ? 'Y' : 'X';
}

static void sendMessage() {
  // Build payload
  char payload[128];
  buildPayload(payload, sizeof(payload));

  // Compute checksum on correct payload
  uint8_t checksum = computeXorChecksum(payload);
  char checksumHex[3];
  checksumToHex(checksum, checksumHex);

  bool shouldCorrupt = ((messageCounter + 1) % 3 == 0); // Corrupt every 3rd message

  char payloadToSend[128];
  strncpy(payloadToSend, payload, sizeof(payloadToSend) - 1);
  payloadToSend[sizeof(payloadToSend) - 1] = '\0';

  if (shouldCorrupt) {
    corruptPayload(payloadToSend); // Corrupt AFTER computing checksum to force mismatch
  }

  // Build framed message: ^PAYLOAD$CS\r\n
  char framed[160];
  snprintf(framed, sizeof(framed), "^%s$%s\r\n", payloadToSend, checksumHex);

  // Send over radio serial
  RadioSerial.print(framed);

  // Optional debug to USB serial
  Serial.print(F("TX #"));
  Serial.print(messageCounter + 1);
  Serial.print(F(" sent: "));
  Serial.print(framed);
  if (shouldCorrupt) {
    Serial.println(F("(CORRUPTED PAYLOAD)"));
  }

  messageCounter++;
}

void setup() {
  Serial.begin(USB_BAUD);
  while (!Serial) {
    ; // wait for serial on boards with native USB
  }
  Serial.println(F("Transmitter starting..."));

  RadioSerial.begin(RADIO_BAUD);
  delay(50);
}

void loop() {
  unsigned long now = millis();
  if (now - lastSendMillis >= SEND_INTERVAL_MS) {
    lastSendMillis = now;
    sendMessage();
  }
}