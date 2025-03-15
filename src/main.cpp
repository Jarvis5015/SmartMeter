#include <SPI.h>
#include <LoRa.h>
#include <string.h>

// ----- Configuration -----
#define LORA_FREQUENCY 433E6   // Adjust frequency as needed
#define MAX_RECORDS 10         // Maximum number of measurement records
#define PACKET_HEADER 0xAA     // Start-of-packet marker

// ----- Data Structures -----
struct __attribute__((packed)) MeasurementRecord {
  uint32_t timestamp;  // Seconds since startup (simulated)
  int16_t voltage;     // decivolts
  int16_t current;     // deciamps
  int16_t power;       // deciwatts
  int16_t rms;         // decivolts
};

struct __attribute__((packed)) MeterDataPacket {
  uint8_t  header;       
  uint16_t meterID;
  uint16_t discomID;
  uint16_t customerID;
  uint8_t  numRecords;
  MeasurementRecord records[MAX_RECORDS];
  char     message[30];  
  uint8_t  checksum;     
};

// ----- Node Modes -----
enum NodeMode {
  NORMAL_MODE,
  RELAY_MODE
};
NodeMode nodeMode = NORMAL_MODE;

// ----- Global Variables -----
MeasurementRecord recordBuffer[MAX_RECORDS];
uint8_t recordCount = 1;
uint8_t bufferIndex = 0;

const uint16_t METER_ID = 1001;
const uint16_t DISCOM_ID = 2001;
const uint16_t CUSTOMER_ID = 3001;

unsigned long lastRecordTime = 0;
const unsigned long recordInterval = 5000; // 5 seconds

// ----- Helper Functions -----
uint8_t computeChecksum(uint8_t* data, size_t len) {
  uint16_t sum = 0;
  for (size_t i = 0; i < len; i++){
    sum += data[i];
  }
  return sum & 0xFF;
}

MeasurementRecord simulateMeasurement() {
  MeasurementRecord rec;
  rec.timestamp = millis() / 1000;  // simulated timestamp
  rec.voltage = 2300 + (recordCount * 10);
  rec.current = 150 + (recordCount * 5);
  rec.power   = 3500 + (recordCount * 20);
  rec.rms     = 2250 + (recordCount * 8);
  return rec;
}

void assemblePacket(MeterDataPacket &packet) {
  packet.header = PACKET_HEADER;
  packet.meterID = METER_ID;
  packet.discomID = DISCOM_ID;
  packet.customerID = CUSTOMER_ID;
  packet.numRecords = recordCount;
  for (uint8_t i = 0; i < recordCount; i++) {
    uint8_t idx = (bufferIndex + i) % MAX_RECORDS;
    packet.records[i] = recordBuffer[idx];
  }
  strncpy(packet.message, "RESPONSE", sizeof(packet.message));
}

void sendPacket(MeterDataPacket &packet) {
  packet.checksum = 0;
  uint8_t* dataPtr = (uint8_t*)&packet;
  size_t checksumLen = sizeof(MeterDataPacket) - sizeof(packet.checksum);
  packet.checksum = computeChecksum(dataPtr, checksumLen);
  
  LoRa.beginPacket();
  LoRa.write(dataPtr, sizeof(MeterDataPacket));
  LoRa.endPacket();
  
  Serial.println("Data packet sent:");
  Serial.print("Meter ID: "); Serial.println(packet.meterID);
  Serial.print("Message: "); Serial.println(packet.message);
  Serial.print("Records: "); Serial.println(packet.numRecords);
  for (uint8_t i = 0; i < packet.numRecords; i++) {
    MeasurementRecord &r = packet.records[i];
    Serial.print("Record "); Serial.print(i+1);
    Serial.print(" | TS: "); Serial.print(r.timestamp);
    Serial.print(" | Voltage: "); Serial.print(r.voltage / 10.0); Serial.print(" V");
    Serial.print(" | Current: "); Serial.print(r.current / 10.0); Serial.print(" A");
    Serial.print(" | Power: "); Serial.print(r.power / 10.0); Serial.print(" W");
    Serial.print(" | RMS: "); Serial.print(r.rms / 10.0); Serial.println(" V");
  }
  Serial.print("Checksum: 0x"); Serial.println(packet.checksum, HEX);
}

void sendBroadcast() {
  MeterDataPacket broadcastPacket;
  broadcastPacket.header = PACKET_HEADER;
  broadcastPacket.meterID = 0;
  broadcastPacket.discomID = 0;
  broadcastPacket.customerID = 0;
  broadcastPacket.numRecords = 0;
  strncpy(broadcastPacket.message, "BROADCAST", sizeof(broadcastPacket.message));
  broadcastPacket.checksum = 0;
  uint8_t* dataPtr = (uint8_t*)&broadcastPacket;
  size_t checksumLen = sizeof(MeterDataPacket) - sizeof(broadcastPacket.checksum);
  broadcastPacket.checksum = computeChecksum(dataPtr, checksumLen);
  
  LoRa.beginPacket();
  LoRa.write(dataPtr, sizeof(MeterDataPacket));
  LoRa.endPacket();
  
  Serial.println("Broadcast command re-sent.");
}

void handleBroadcastMessage() {
  Serial.println("Broadcast command received!");
  // If no data is stored, simulate one measurement record.
  if (recordCount == 0) {
    MeasurementRecord rec = simulateMeasurement();
    recordBuffer[bufferIndex] = rec;
    bufferIndex = (bufferIndex + 1) % MAX_RECORDS;
    recordCount = 1;
    Serial.println("No stored data, simulated one record.");
  }
  MeterDataPacket outgoingPacket;
  assemblePacket(outgoingPacket);
  sendPacket(outgoingPacket);
  // Clear the circular buffer.
  recordCount = 0;
  bufferIndex = 0;
  sendBroadcast();
  nodeMode = RELAY_MODE;
  Serial.println("Node switched to RELAY_MODE.");
}

void setup() {
  Serial.begin(9600);
  while (!Serial);
  Serial.println("LoRa Establishing");
  while (!LoRa.begin(433E6)) {
    Serial.println(".");
    delay(500);
  }
  LoRa.setSyncWord(0xF3);
  Serial.println("LoRa Initializing OK!");
}

void loop() {
  unsigned long currentTime = millis();
  // Record measurement data every recordInterval.
  if (currentTime - lastRecordTime >= recordInterval) {
    lastRecordTime = currentTime;
    MeasurementRecord rec = simulateMeasurement();
    recordBuffer[bufferIndex] = rec;
    bufferIndex = (bufferIndex + 1) % MAX_RECORDS;
    if (recordCount < MAX_RECORDS) {
      recordCount++;
    }
    Serial.print("Recorded sample. Total stored: ");
    Serial.println(recordCount);
  }
  
  // Read incoming LoRa packet into a byte buffer.
  int packetSize = LoRa.parsePacket();
  if (packetSize > 0) {
    byte buffer[packetSize];
    int index = 0;
    while (LoRa.available() && index < packetSize) {
      buffer[index++] = LoRa.read();
    }
    // Look for the "BROADCAST" command (even if extra binary data is present).
    bool isBroadcast = false;
    for (int i = 0; i < packetSize; i++) {
      if (buffer[i] == 'B') { 
        if (packetSize - i >= 9 && strncmp((char*)&buffer[i], "BROADCAST", 9) == 0) {
          isBroadcast = true;
          break;
        }
      }
    }
    if (isBroadcast) {
      handleBroadcastMessage();
      return;
    }
    // If the packet size exactly matches a full MeterDataPacket, process it.
    if (packetSize == sizeof(MeterDataPacket)) {
      MeterDataPacket incomingPacket;
      memcpy(&incomingPacket, buffer, sizeof(MeterDataPacket));
      if (incomingPacket.header != PACKET_HEADER) {
        Serial.println("Invalid packet header");
        return;
      }
      uint8_t calcChecksum = computeChecksum(buffer, sizeof(MeterDataPacket) - sizeof(incomingPacket.checksum));
      if (calcChecksum != incomingPacket.checksum) {
        Serial.println("Checksum mismatch");
        return;
      }
      Serial.println("Received structured packet:");
      Serial.print("Meter ID: "); Serial.println(incomingPacket.meterID);
      Serial.print("Message: "); Serial.println(incomingPacket.message);
      Serial.print("Records: "); Serial.println(incomingPacket.numRecords);
      if (strcmp(incomingPacket.message, "BROADCAST") == 0) {
        handleBroadcastMessage();
      }
    } else {
      Serial.print("Unexpected packet size: ");
      Serial.println(packetSize);
    }
  }
}
