#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <LoRa.h>

// ----- LoRa and WiFi Configuration -----
#define LORA_SS    5    // Chip Select pin
#define LORA_RST   14   // Reset pin
#define LORA_DIO0  2    // DIO0 pin
#define LORA_FREQUENCY 433E6

#define WIFI_SSID "Redmi"
#define WIFI_PASSWORD ""

// ----- Firebase Configuration -----
// For Realtime Database, we assume your DB URL is https://smartmeter-699c3.firebaseio.com/
const char* projectId = "smartmeter-699c3";  
const char* apiKey = "AIzaSyC3qC9a0opEjr_ThRv56Dp86AoUCtP0n6A";
String idToken;

// ----- Binary Packet Definitions (shared with node) -----
struct __attribute__((packed)) MeasurementRecord {
  uint32_t timestamp;  // Unix epoch time (or offset in seconds)
  int16_t voltage;     // decivolts
  int16_t current;     // deciamps
  int16_t power;       // deciwatts
  int16_t rms;         // decivolts
};

#define MAX_RECORDS 10
#define PACKET_HEADER 0xAA

struct __attribute__((packed)) MeterDataPacket {
  uint8_t  header;       // Should equal PACKET_HEADER
  uint16_t meterID;
  uint16_t discomID;
  uint16_t customerID;
  uint8_t  numRecords;
  MeasurementRecord records[MAX_RECORDS];
  char     message[30];  // e.g., "RESPONSE" or "BROADCAST"
  uint8_t  checksum;     // Simple checksum over packet bytes (except checksum field)
};

// ----- Global variables for broadcast & data collection -----
bool nodeResponded = false;
bool collectingData = false;
unsigned long lastBroadcast = 0;
unsigned long lastPacketTime = 0;
const unsigned long BROADCAST_INTERVAL = 1000; // in milliseconds
const unsigned long DATA_FETCH_TIMEOUT = 5000; // no new packet for 5 sec => collection complete
const unsigned long BROADCAST_WAVE_DELAY = 10000; // delay after data collection (10 seconds)

// ----- Function Prototypes -----
void initWiFi();
bool firebaseAnonymousSignIn();
bool sendToFirebaseRTDB(const String& jsonData);
void processLoRaData(byte* data, int length);
uint8_t computeChecksum(uint8_t* data, size_t len);
void blinkLED(int times);

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);
  Serial.begin(115200);
  while (!Serial);  
  Serial.println("LoRa Gateway Initializing");
  
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  while (!LoRa.begin(LORA_FREQUENCY)) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nLoRa Initialized Successfully");
  
  initWiFi();

  // Authenticate with Firebase anonymously
  if (firebaseAnonymousSignIn()) {
    Serial.println("Authenticated with Firebase anonymously.");
    digitalWrite(BUILTIN_LED, HIGH);
    delay(500);
    digitalWrite(BUILTIN_LED, LOW);
  } else {
    Serial.println("Firebase authentication failed.");
    for (int i = 0; i < 3; i++) {
      digitalWrite(BUILTIN_LED, HIGH);
      delay(200);
      digitalWrite(BUILTIN_LED, LOW);
      delay(200);
    }
  }

  // Initially, set to receive mode.
  LoRa.receive();
  Serial.println("Listening for LoRa transmissions...");
}

void loop() {
  unsigned long currentTime = millis();

  // While no node response is detected, send BROADCAST periodically.
  if (!nodeResponded) {
    if (currentTime - lastBroadcast >= BROADCAST_INTERVAL) {
      LoRa.beginPacket();
      LoRa.print("BROADCAST");
      LoRa.endPacket();
      Serial.println("Broadcast sent.");
      lastBroadcast = currentTime;
    }
  }

  // Check for incoming LoRa packets.
  int packetSize = LoRa.parsePacket();
  if (packetSize > 0) {
    byte buffer[packetSize];
    for (int i = 0; i < packetSize; i++) {
      buffer[i] = LoRa.read();
    }
    // Process the incoming packet.
    processLoRaData(buffer, packetSize);

    // On receiving a valid response, mark node response and start data collection.
    if (!nodeResponded) {
      nodeResponded = true;
      collectingData = true;
      Serial.println("Node responded. Fetching transmitted data...");
    }
    lastPacketTime = currentTime;
  }

  // If in data collection mode and no new packet is received for the timeout, conclude.
  if (collectingData && (currentTime - lastPacketTime > DATA_FETCH_TIMEOUT)) {
    Serial.println("Data collection complete.");
    // Wait before starting next broadcast wave.
    delay(BROADCAST_WAVE_DELAY);
    // Reset state to allow a new round of broadcasting/data fetch.
    nodeResponded = false;
    collectingData = false;
  }

  // Refresh Firebase token every hour.
  static unsigned long lastTokenRefresh = 0;
  if (millis() - lastTokenRefresh > 3600000) {
    Serial.println("Refreshing Firebase token...");
    firebaseAnonymousSignIn();
    lastTokenRefresh = millis();
  }
}

void initWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi ..");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    Serial.print('.');
    delay(1000);
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Connected! IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Failed to connect to WiFi. Please check credentials.");
  }
}

bool firebaseAnonymousSignIn() {
  HTTPClient http;
  String url = "https://identitytoolkit.googleapis.com/v1/accounts:signUp?key=" + String(apiKey);
  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  DynamicJsonDocument doc(256);
  doc["returnSecureToken"] = true;
  String requestBody;
  serializeJson(doc, requestBody);
  
  int httpResponseCode = http.POST(requestBody);
  if (httpResponseCode == 200) {
    String response = http.getString();
    DynamicJsonDocument responseDoc(512);
    DeserializationError error = deserializeJson(responseDoc, response);
    if (!error) {
      idToken = responseDoc["idToken"].as<String>();
      Serial.println("Anonymous token obtained successfully");
      http.end();
      return true;
    } else {
      Serial.print("JSON deserialize error: ");
      Serial.println(error.c_str());
      http.end();
      return false;
    }
  } else {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    Serial.println(http.getString());
    http.end();
    return false;
  }
}

// ----- New Function: sendToFirebaseRTDB -----
// This function sends jsonData to the Firebase Realtime Database using HTTP POST.
bool sendToFirebaseRTDB(const String& jsonData) {
  if (idToken.length() == 0) {
    Serial.println("No auth token available. Signing in again...");
    if (!firebaseAnonymousSignIn()) {
      return false;
    }
  }
  
  HTTPClient http;
  // Construct the Realtime Database URL; adjust the path if needed.
  String databaseUrl = "https://smartmeter-699c3-default-rtdb.firebaseio.com/smartMeterData.json?auth=" + idToken;
  http.begin(databaseUrl);
  http.addHeader("Content-Type", "application/json");
  
  int httpResponseCode = http.POST(jsonData);
  if (httpResponseCode == 200 || httpResponseCode == 201) {
    String response = http.getString();
    Serial.println("Realtime Database response: " + response);
    http.end();
    return true;
  } else {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    Serial.println(http.getString());
    http.end();
    if (httpResponseCode == 401 || httpResponseCode == 403) {
      Serial.println("Auth token expired. Refreshing...");
      if (firebaseAnonymousSignIn()) {
        return sendToFirebaseRTDB(jsonData);
      }
    }
    return false;
  }
}

void processLoRaData(byte* data, int length) {
  // If the first character is '{', treat it as a JSON string.
  if (length > 0 && data[0] == '{') {
    String jsonString = "";
    for (int i = 0; i < length; i++) {
      jsonString += (char)data[i];
    }
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, jsonString);
    if (error) {
      Serial.print("JSON parsing failed: ");
      Serial.println(error.c_str());
      if (sendToFirebaseRTDB(jsonString)) {
        Serial.println("Raw JSON data sent to Firebase Realtime Database successfully");
      } else {
        Serial.println("Failed to send raw JSON data to Firebase Realtime Database");
      }
    } else {
      if (sendToFirebaseRTDB(jsonString)) {
        Serial.println("JSON data sent to Firebase Realtime Database successfully");
      } else {
        Serial.println("Failed to send JSON data to Firebase Realtime Database");
      }
    }
  }
  // Otherwise, if the length matches our binary packet size, process it as measurement data.
  else if (length == sizeof(MeterDataPacket)) {
    MeterDataPacket packet;
    memcpy(&packet, data, sizeof(MeterDataPacket));
    if (packet.header != PACKET_HEADER) {
      Serial.println("Invalid packet header in binary data");
      return;
    }
    uint8_t calcChecksum = computeChecksum(data, sizeof(MeterDataPacket) - sizeof(packet.checksum));
    if (calcChecksum != packet.checksum) {
      Serial.println("Checksum mismatch in binary data");
      return;
    }
    // Convert binary data into JSON format.
    DynamicJsonDocument doc(2048);
    doc["meterID"] = packet.meterID;
    doc["discomID"] = packet.discomID;
    doc["customerID"] = packet.customerID;
    doc["numRecords"] = packet.numRecords;
    JsonArray records = doc.createNestedArray("records");
    for (int i = 0; i < packet.numRecords; i++) {
      JsonObject rec = records.createNestedObject();
      rec["timestamp"] = packet.records[i].timestamp;
      rec["voltage"] = packet.records[i].voltage;
      rec["current"] = packet.records[i].current;
      rec["power"] = packet.records[i].power;
      rec["rms"] = packet.records[i].rms;
    }
    doc["message"] = packet.message;
    String jsonStr;
    serializeJson(doc, jsonStr);
    if (sendToFirebaseRTDB(jsonStr)) {
      Serial.println("Binary data sent to Firebase Realtime Database successfully");
      blinkLED(1);
    } else {
      Serial.println("Failed to send binary data to Firebase Realtime Database");
      blinkLED(2);
    }
  }
  else {
    Serial.print("Unexpected packet length: ");
    Serial.println(length);
  }
}

uint8_t computeChecksum(uint8_t* data, size_t len) {
  uint16_t sum = 0;
  for (size_t i = 0; i < len; i++){
    sum += data[i];
  }
  return sum & 0xFF;
}

void blinkLED(int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(BUILTIN_LED, HIGH);
    delay(100);
    digitalWrite(BUILTIN_LED, LOW);
    delay(100);
  }
}
