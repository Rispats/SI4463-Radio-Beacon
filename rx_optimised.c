#include <SPI.h>

#define START 0x8A
#define STOP 0x8B
#define SDN 7
#define CSN 10

uint8_t cfg[] = { 
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

static inline void send_cmd(uint8_t* d, uint8_t l) {
  digitalWrite(CSN, LOW);
  for (uint8_t i = 0; i < l; i++) SPI.transfer(d[i]);
  digitalWrite(CSN, HIGH);
}

static inline bool cts() {
  for (uint8_t i = 0; i < 100; i++) {
    digitalWrite(CSN, LOW);
    SPI.transfer(0x44);
    delayMicroseconds(20);
    uint8_t c = SPI.transfer(0x00);
    digitalWrite(CSN, HIGH);
    if (c == 0xFF) return true;
    delay(5);
  }
  return false;
}

static inline void init_radio() {
  digitalWrite(SDN, HIGH); delay(50);
  digitalWrite(SDN, LOW); delay(50);
  digitalWrite(CSN, HIGH);
  for (uint16_t i = 0; i < sizeof(cfg); ) {
    uint8_t l = cfg[i++];
    if (l == 0x00) break;
    send_cmd(&cfg[i], l);
    i += l;
    cts();
  }
}

static inline void read_rxfifo(uint8_t* buf, uint8_t max_len) {
  digitalWrite(CSN, LOW);
  SPI.transfer(0x77);
  for (uint8_t i = 0; i < max_len; i++) buf[i] = SPI.transfer(0x00);
  digitalWrite(CSN, HIGH);
}

static inline void clear_rxfifo() {
  uint8_t f[] = { 0x15, 0x03 };
  send_cmd(f, sizeof(f));
  cts();
}

static inline void start_rx() {
  uint8_t rx[] = { 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08 };
  send_cmd(rx, sizeof(rx));
  cts();
}

static inline uint8_t hex_to_nibble(char h) {
  if (h >= '0' && h <= '9') return (uint8_t)(h - '0');
  if (h >= 'A' && h <= 'F') return (uint8_t)(h - 'A' + 10);
  if (h >= 'a' && h <= 'f') return (uint8_t)(h - 'a' + 10);
  return 0;
}

static inline uint8_t hex_to_byte(char h1, char h2) {
  return (uint8_t)((hex_to_nibble(h1) << 4) | hex_to_nibble(h2));
}

static inline uint8_t compute_checksum_sum(const char* s) {
  uint8_t cs = 0;
  for (const char* p = s; *p; ++p) cs += (uint8_t)(*p);
  return cs;
}

void setup() {
  pinMode(SDN, OUTPUT);
  pinMode(CSN, OUTPUT);
  digitalWrite(CSN, HIGH);
  Serial.begin(115200);
  SPI.begin();
  SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
  init_radio();
  start_rx();
  Serial.println("RX_READY");
}

void loop() {
  uint8_t buf[96];
  read_rxfifo(buf, sizeof(buf));

  if (buf[0] == START) {
    uint8_t id = buf[1];
    int stop_pos = -1;
    for (int i = 2; i < (int)sizeof(buf); i++) {
      if (buf[i] == STOP) { stop_pos = i; break; }
    }

    if (stop_pos > 2 && stop_pos + 2 < (int)sizeof(buf)) {
      // Extract payload and checksum ASCII
      uint8_t payload_len = (uint8_t)(stop_pos - 2);
      char payload[64];
      if (payload_len >= sizeof(payload)) payload_len = sizeof(payload) - 1;
      memcpy(payload, &buf[2], payload_len);
      payload[payload_len] = '\0';

      char cs_hi = (char)buf[stop_pos + 1];
      char cs_lo = (char)buf[stop_pos + 2];
      uint8_t recv_cs = hex_to_byte(cs_hi, cs_lo);
      uint8_t calc_cs = compute_checksum_sum(payload);
      bool ok = (recv_cs == calc_cs);

      // Expected payload: YYYY-MM-DD,HH:MM:SS,lat,lon
      // Split minimally without heavy tokenizers
      char date[11] = {0};
      char time_s[9] = {0};
      char lat[16] = {0};
      char lon[16] = {0};

      // Parse by commas
      uint8_t cidx = 0; uint8_t p = 0;
      for (uint8_t i = 0; i <= payload_len; i++) {
        char ch = payload[i];
        if (ch == ',' || ch == '\0') {
          uint8_t len = i - cidx;
          if (p == 0 && len <= 10) { memcpy(date, &payload[cidx], len); date[len] = '\0'; }
          else if (p == 1 && len <= 8) { memcpy(time_s, &payload[cidx], len); time_s[len] = '\0'; }
          else if (p == 2 && len < sizeof(lat)) { memcpy(lat, &payload[cidx], len); lat[len] = '\0'; }
          else if (p == 3 && len < sizeof(lon)) { memcpy(lon, &payload[cidx], len); lon[len] = '\0'; }
          p++;
          cidx = i + 1;
        }
      }

      // Output a single parseable line
      Serial.print("RESULT,");
      Serial.print(id, HEX);
      Serial.print(',');
      Serial.print(date);
      Serial.print(',');
      Serial.print(time_s);
      Serial.print(',');
      Serial.print(lat);
      Serial.print(',');
      Serial.print(lon);
      Serial.print(',');
      Serial.print((int)recv_cs);
      Serial.print(',');
      Serial.print((int)calc_cs);
      Serial.print(',');
      Serial.println(ok ? "VALID" : "CORRUPT");
    }
  }

  clear_rxfifo();
  start_rx();
  delay(200);
}
