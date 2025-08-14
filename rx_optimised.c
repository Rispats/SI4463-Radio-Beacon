#include <SPI.h>

#define START 0x8A
#define STOP 0x8B
#define SDN 7
#define CSN 10
#define MAX_ENTRIES 10

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

struct gps_data {
  char lat[12];
  char lon[12];
  bool valid;
};

struct tx_table {
  uint8_t tx_id;
  gps_data entries[MAX_ENTRIES];
  uint8_t count;
  uint16_t total_rx;
  uint16_t valid_rx;
};

tx_table tables[2]; // Support for A1 and A2
uint8_t num_tables = 0;
bool header_printed = false;
int row_count = 0;

void send_cmd(uint8_t* d, uint8_t l) {
  digitalWrite(CSN, LOW);
  for (uint8_t i = 0; i < l; i++) SPI.transfer(d[i]);
  digitalWrite(CSN, HIGH);
}

bool cts() {
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

void init_radio() {
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

void read_rxfifo(uint8_t* buf, uint8_t max_len) {
  digitalWrite(CSN, LOW);
  SPI.transfer(0x77);
  for (uint8_t i = 0; i < max_len; i++) buf[i] = SPI.transfer(0x00);
  digitalWrite(CSN, HIGH);
}

void clear_rxfifo() {
  uint8_t f[] = { 0x15, 0x03 };
  send_cmd(f, sizeof(f));
  cts();
}

void start_rx() {
  uint8_t rx[] = { 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08 };
  send_cmd(rx, sizeof(rx));
  cts();
}

uint8_t hex_to_byte(char h1, char h2) {
  uint8_t v = 0;
  if (h1 >= '0' && h1 <= '9') v += (h1 - '0') * 16;
  else if (h1 >= 'A' && h1 <= 'F') v += (h1 - 'A' + 10) * 16;
  if (h2 >= '0' && h2 <= '9') v += (h2 - '0');
  else if (h2 >= 'A' && h2 <= 'F') v += (h2 - 'A' + 10);
  return v;
}

bool checksum(char* msg) {
  int len = strlen(msg);
  if (len < 3) return false;
  
  unsigned char calc_cs = 0;
  for (int i = 0; i < len - 2; i++) {
    calc_cs += msg[i];
  }
  
  uint8_t recv_cs = hex_to_byte(msg[len-2], msg[len-1]);
  return (calc_cs == recv_cs);
}

int find_table(uint8_t tx_id) {
  for (int i = 0; i < num_tables; i++) {
    if (tables[i].tx_id == tx_id) return i;
  }
  return -1;
}

void print_header() {
  if (!header_printed) {
    Serial.println("========== GPS TRACKING LOG ==========");
    Serial.println("Row | TX_ID | Latitude  | Longitude | Status");
    Serial.println("----+-------+-----------+-----------+-------");
    header_printed = true;
  }
}

void append_row(int row, const char* lat, const char* lon, uint8_t tx_id, bool valid) {
  print_header();
  
  Serial.printf("%3d | TX_%02X | %-9s | %-9s | %s\n", 
                row, tx_id, lat, lon, valid ? "GOOD" : "BAD");
}

bool add_data_to_table(uint8_t tx_id, const char* lat, const char* lon, bool valid) {
  int table_idx = find_table(tx_id);
  
  if (table_idx == -1) {
    // Create new table
    if (num_tables >= 2) return false; // Max tables reached
    table_idx = num_tables++;
    tables[table_idx].tx_id = tx_id;
    tables[table_idx].count = 0;
    tables[table_idx].total_rx = 0;
    tables[table_idx].valid_rx = 0;
  }
  
  tx_table* table = &tables[table_idx];
  table->total_rx++;
  if (valid) table->valid_rx++;
  
  if (valid && table->count < MAX_ENTRIES) {
    strncpy(table->entries[table->count].lat, lat, sizeof(table->entries[table->count].lat) - 1);
    table->entries[table->count].lat[sizeof(table->entries[table->count].lat) - 1] = '\0';
    
    strncpy(table->entries[table->count].lon, lon, sizeof(table->entries[table->count].lon) - 1);
    table->entries[table->count].lon[sizeof(table->entries[table->count].lon) - 1] = '\0';
    
    table->entries[table->count].valid = true;
    table->count++;
    return true;
  }
  
  return false;
}

void print_summary() {
  Serial.println("\n========== SUMMARY ==========");
  for (int t = 0; t < num_tables; t++) {
    tx_table* table = &tables[t];
    Serial.printf("TX_%02X: %d total, %d valid (%.1f%% success)\n", 
                  table->tx_id, table->total_rx, table->valid_rx,
                  table->total_rx > 0 ? (table->valid_rx * 100.0 / table->total_rx) : 0.0);
  }
  Serial.println("============================\n");
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
  Serial.println("RX Ready - Listening for A1 and A2");
  Serial.println("====================================");
}

uint32_t last_summary = 0;

void loop() {
  uint8_t buf[64];
  read_rxfifo(buf, sizeof(buf));

  if (buf[0] == START && (buf[1] == 0xA1 || buf[1] == 0xA2)) {
    int stop_pos = -1;
    for (int i = 2; i < 64; i++) {
      if (buf[i] == STOP) {
        stop_pos = i;
        break;
      }
    }

    if (stop_pos > 4) {
      uint8_t tx_id = buf[1];
      uint8_t msg_len = stop_pos - 2;
      char msg[64];
      memcpy(msg, &buf[2], msg_len);
      msg[msg_len] = '\0';

      bool cs_valid = checksum(msg);
      
      if (cs_valid) {
        // Parse lat,lon format (remove checksum first)
        char temp_msg[64];
        strncpy(temp_msg, msg, msg_len - 2);
        temp_msg[msg_len - 2] = '\0';
        
        char* lat = strtok(temp_msg, ",");
        char* lon = strtok(NULL, ",");

        if (lat && lon) {
          row_count++;
          add_data_to_table(tx_id, lat, lon, true);
          append_row(row_count, lat, lon, tx_id, true);
        }
      } else {
        // Invalid checksum
        add_data_to_table(tx_id, "N/A", "N/A", false);
        row_count++;
        append_row(row_count, "N/A", "N/A", tx_id, false);
      }
    }
  }

  // Print summary every 30 seconds
  if (millis() - last_summary > 30000) {
    last_summary = millis();
    print_summary();
  }

  clear_rxfifo();
  start_rx();
  delay(250);
}
