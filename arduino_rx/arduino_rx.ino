#include <SoftwareSerial.h>

// Receiver sketch
// - Receives messages in the format: ^<PAYLOAD>$<CHECKSUM>\r\n
// - PAYLOAD: date,time,latitude,longitude
// - CHECKSUM: two-digit uppercase hex of XOR across PAYLOAD bytes
// - Prints processed results to USB serial as a single line per message:
//   RESULT|date=YYYY-MM-DD|time=HH:MM:SS|lat=xx.xxxxxx|lon=yy.yyyyyy|rx_checksum=XX|calc_checksum=YY|status=VALID|raw=...

// CONFIGURE: Pins connected to the radio module on the receiver board
static const uint8_t RADIO_RX_PIN = 8;   // Module TX -> this RX
static const uint8_t RADIO_TX_PIN = 9;   // Module RX <- this TX (not used but required by SoftwareSerial)

// Baud rates
static const unsigned long RADIO_BAUD = 9600;     // Match transmitter
static const unsigned long USB_BAUD   = 115200;   // For USB serial output

SoftwareSerial RadioSerial(RADIO_RX_PIN, RADIO_TX_PIN); // RX, TX

// Parser state
enum ParserState {
  WAIT_START,
  READ_PAYLOAD,
  READ_CHECKSUM
};

static ParserState parserState = WAIT_START;
static char payloadBuffer[160];
static size_t payloadLen = 0;
static char checksumBuffer[3]; // 2 hex chars + NUL
static size_t checksumLen = 0;

static uint8_t computeXorChecksum(const char* payload) {
  uint8_t checksum = 0;
  for (const char* p = payload; *p != '\0'; ++p) {
    checksum ^= static_cast<uint8_t>(*p);
  }
  return checksum;
}

static int hexDigitToValue(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return -1;
}

static bool parseTwoHexToByte(const char* twoHex, uint8_t& out) {
  int hi = hexDigitToValue(twoHex[0]);
  int lo = hexDigitToValue(twoHex[1]);
  if (hi < 0 || lo < 0) return false;
  out = static_cast<uint8_t>((hi << 4) | lo);
  return true;
}

static void byteToHex(uint8_t value, char out[3]) {
  const char hexChars[] = "0123456789ABCDEF";
  out[0] = hexChars[(value >> 4) & 0x0F];
  out[1] = hexChars[value & 0x0F];
  out[2] = '\0';
}

static void resetParser() {
  parserState = WAIT_START;
  payloadLen = 0;
  checksumLen = 0;
  payloadBuffer[0] = '\0';
  checksumBuffer[0] = '\0';
}

static void emitResultLine(const char* payload, uint8_t rxChecksum, bool rxChecksumOk, uint8_t calcChecksum) {
  // Copy payload to safely tokenize
  char work[160];
  strncpy(work, payload, sizeof(work) - 1);
  work[sizeof(work) - 1] = '\0';

  // Extract fields
  const char* empty = "";
  char* token;

  char* date = nullptr;
  char* timeStr = nullptr;
  char* lat = nullptr;
  char* lon = nullptr;

  token = strtok(work, ",");
  if (token) { date = token; token = strtok(nullptr, ","); }
  if (token) { timeStr = token; token = strtok(nullptr, ","); }
  if (token) { lat = token; token = strtok(nullptr, ","); }
  if (token) { lon = token; }

  char rxHex[3];
  byteToHex(rxChecksum, rxHex);
  char calcHex[3];
  byteToHex(calcChecksum, calcHex);

  // Output single parseable line
  Serial.print(F("RESULT|date="));
  Serial.print(date ? date : empty);
  Serial.print(F("|time="));
  Serial.print(timeStr ? timeStr : empty);
  Serial.print(F("|lat="));
  Serial.print(lat ? lat : empty);
  Serial.print(F("|lon="));
  Serial.print(lon ? lon : empty);
  Serial.print(F("|rx_checksum="));
  Serial.print(rxHex);
  Serial.print(F("|calc_checksum="));
  Serial.print(calcHex);
  Serial.print(F("|status="));
  Serial.print(rxChecksumOk ? F("VALID") : F("CORRUPT"));
  Serial.print(F("|raw=^"));
  Serial.print(payload);
  Serial.print(F("$"));
  Serial.print(rxHex);
  Serial.println();
}

void setup() {
  Serial.begin(USB_BAUD);
  while (!Serial) { ; }
  Serial.println(F("Receiver starting..."));

  RadioSerial.begin(RADIO_BAUD);
  resetParser();
}

void loop() {
  while (RadioSerial.available() > 0) {
    char c = static_cast<char>(RadioSerial.read());

    switch (parserState) {
      case WAIT_START:
        if (c == '^') {
          payloadLen = 0;
          checksumLen = 0;
          payloadBuffer[0] = '\0';
          parserState = READ_PAYLOAD;
        }
        break;

      case READ_PAYLOAD:
        if (c == '$') {
          payloadBuffer[payloadLen] = '\0';
          checksumLen = 0;
          parserState = READ_CHECKSUM;
        } else {
          if (payloadLen + 1 < sizeof(payloadBuffer)) {
            payloadBuffer[payloadLen++] = c;
          } else {
            // Overflow: reset
            resetParser();
          }
        }
        break;

      case READ_CHECKSUM:
        if (c == '\r' || c == '\n') {
          // Ignore line endings, wait for exactly 2 hex digits
          // If we already collected 2 digits, we can finish on EOL
          if (checksumLen == 2) {
            uint8_t rxChecksum = 0;
            bool hexOk = parseTwoHexToByte(checksumBuffer, rxChecksum);
            uint8_t calcChecksum = computeXorChecksum(payloadBuffer);
            bool isValid = hexOk && (rxChecksum == calcChecksum);
            emitResultLine(payloadBuffer, rxChecksum, isValid, calcChecksum);
            resetParser();
          }
        } else {
          if (((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f')) && checksumLen < 2) {
            checksumBuffer[checksumLen++] = c;
            if (checksumLen == 2) {
              checksumBuffer[2] = '\0';
              // We could emit immediately; wait for EOL for cleanliness
            }
          } else {
            // Invalid char in checksum; reset
            resetParser();
          }
        }
        break;
    }
  }
}