#include <ESP8266WiFi.h>
#include <WebSocketsServer.h>

#define ANSI_RESET   "\x1b[0m"
#define ANSI_RED     "\x1b[31m"
#define ANSI_GREEN   "\x1b[32m"
#define ANSI_YELLOW  "\x1b[33m"
#define ANSI_BLUE    "\x1b[34m"
#define ANSI_MAGENTA "\x1b[35m"
#define ANSI_CYAN    "\x1b[36m"

struct WifiNetwork {
  const char* ssid;
  const char* password;
};

const WifiNetwork knownNetworks[] = {
  {"Alien", "pK3169@31"}
  // {"Andromeda24", "AG?6<mE!8WAC0XgXq"}
};

const int numKnownNetworks = sizeof(knownNetworks) / sizeof(knownNetworks[0]);

const uint16_t WEBSOCKET_PORT = 1337;
const long SERIAL_BAUD_RATE = 115200;
const char PACKET_TERMINATOR = '\n';

WebSocketsServer webSocket(WEBSOCKET_PORT);
String stm32DataBuffer = "";
bool stm32DataReady = false;
unsigned long lastWifiCheckTime = 0;
const long wifiCheckInterval = 5000;

void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length);
void setupSerial();
void setupWiFi();
void setupWebSocket();
void handleSerialReception();
void broadcastSerialData();
void clearSerialBuffer();
void checkWiFiConnection();

void setup() {
  setupSerial();
  setupWiFi();
  setupWebSocket();
}

void loop() {
  checkWiFiConnection();
  webSocket.loop();
  handleSerialReception();
  broadcastSerialData();
}

void setupSerial() {
  Serial.begin(SERIAL_BAUD_RATE);
  unsigned long startTime = millis();

  while (!Serial || (millis() - startTime < 3000)) {}

  Serial.println();
  Serial.println(ANSI_GREEN "Serial initialized" ANSI_RESET);
}

void setupWiFi() {
  bool connected = false;
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  while (!connected) {
    Serial.println(ANSI_YELLOW "Scanning for WiFi networks..." ANSI_RESET);
    
    int numNetworks = WiFi.scanNetworks();

    if (numNetworks == 0) {
      Serial.println(ANSI_RED "No networks found. Retrying in 5 seconds..." ANSI_RESET);
      delay(5000);
      continue;
    }

    Serial.println();
    Serial.printf("%d networks found:\n", numNetworks);

    for (int i = 0; i < numNetworks; ++i)
      Serial.printf("  %d: %s\n", i + 1, WiFi.SSID(i).c_str());

    Serial.println();

    bool knownNetworkFound = false;

    for (int i = 0; i < numNetworks; i++) {
      String scannedSSID = WiFi.SSID(i);

      for (const auto& net : knownNetworks) { 
        
        if (scannedSSID == net.ssid) { 
          knownNetworkFound = true;
          Serial.print("Known network found: ");
          Serial.print(ANSI_CYAN);
          Serial.print(net.ssid);
          Serial.println(ANSI_RESET);
          Serial.println(ANSI_YELLOW "Attempting to connect..." ANSI_RESET);

          WiFi.begin(net.ssid, net.password);

          int timeoutCounter = 30;
          while (WiFi.status() != WL_CONNECTED && timeoutCounter > 0) {
            delay(500);
            Serial.println(ANSI_YELLOW "Trying to connect..." ANSI_RESET);
            timeoutCounter--;
          }
          Serial.println(); 

          if (WiFi.status() == WL_CONNECTED) {
            Serial.println(ANSI_GREEN "WiFi connected!" ANSI_RESET);
            Serial.print("IP address: ");
            Serial.print(ANSI_CYAN);
            Serial.print(WiFi.localIP());
            Serial.println(ANSI_RESET);
            connected = true; 
            return;
          } else {
            Serial.println(ANSI_RED "Connection failed." ANSI_RESET);
            WiFi.disconnect(); 
            break;
          }
        }
      }
      
      if(connected) break;
      
    }

    if (!connected) {
      if (knownNetworkFound) {
        Serial.println(ANSI_YELLOW "Found known networks, but failed to connect to any." ANSI_RESET);
      } else {
        Serial.println(ANSI_YELLOW "No known networks found in this scan." ANSI_RESET);
      }
      Serial.println("Retrying scan in 5 seconds...");
      delay(5000);
    }
  }
}

void setupWebSocket() {
  webSocket.begin();
  webSocket.onEvent(onWebSocketEvent);
  Serial.printf("WebSocket server started on port %d\n", WEBSOCKET_PORT);
  Serial.println("Waiting for STM32 data...");
  Serial.println();
}

void checkWiFiConnection() {
  unsigned long currentTime = millis();

  if (currentTime - lastWifiCheckTime > wifiCheckInterval) {
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println(ANSI_YELLOW "WiFi connection lost! Attempting to reconnect..." ANSI_RESET);
      WiFi.reconnect();
    }
    lastWifiCheckTime = currentTime;
  }
}

void handleSerialReception() {
  if (stm32DataReady) {
    return;
  }
  while (Serial.available() > 0) {
    char inChar = (char)Serial.read();
    if (inChar == PACKET_TERMINATOR) {
      stm32DataReady = true;
      break;
    } else if (inChar != '\r') {
      stm32DataBuffer += inChar;
    }
  }
}

void broadcastSerialData() {
  if (stm32DataReady && (WiFi.status() == WL_CONNECTED)) {
    webSocket.broadcastTXT(stm32DataBuffer);
    clearSerialBuffer();
  }
}

void clearSerialBuffer() {
  stm32DataBuffer = "";
  stm32DataReady = false;
}

void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        webSocket.sendTXT(num, "Welcome to ESP8266 OBD2 Server!");
        break;
      }
    case WStype_TEXT:
      Serial.printf("[%u] get Text: %s\n", num, payload);
      break;
    case WStype_BIN:
      break;
  }
}