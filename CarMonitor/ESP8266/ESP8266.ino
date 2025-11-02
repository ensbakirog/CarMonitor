#include <ESP8266WiFi.h>
#include <WebSocketsServer.h>

#define ANSI_RESET   "\x1b[0m"  // Tüm renkleri ve stilleri sıfırlar
#define ANSI_RED     "\x1b[31m"  // Kırmızı renk
#define ANSI_GREEN   "\x1b[32m"  // Yeşil renk
#define ANSI_YELLOW  "\x1b[33m"  // Sarı renk
#define ANSI_BLUE    "\x1b[34m"  // Mavi renk
#define ANSI_MAGENTA "\x1b[35m"  // Macenta renk
#define ANSI_CYAN    "\x1b[36m"  // Camgöbeği (Cyan) renk

const char* HOTSPOT_SSID = "Alien";
const char* HOTSPOT_PASSWORD = "pK3169@31";
const char* WIFI_SSID = "Andromeda24";
const char* WIFI_PASSWORD = "AG?6<mE!8WAC0XgXq";
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
  delay(3000);
  Serial.println();
  Serial.println(ANSI_GREEN "Serial initialized" ANSI_RESET);
}

void setupWiFi() {
  Serial.print("Connecting to ");
  Serial.print(ANSI_MAGENTA);
  Serial.print(HOTSPOT_SSID);
  Serial.println(ANSI_RESET);
  Serial.println();

  WiFi.begin(HOTSPOT_SSID, HOTSPOT_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println(ANSI_YELLOW "<< ESP8266 connecting to WIFI >>" ANSI_RESET);
  }

  Serial.println();
  Serial.println(ANSI_GREEN "WiFi connected" ANSI_RESET);
  Serial.print("IP address: ");
  Serial.print(ANSI_CYAN);
  Serial.print(WiFi.localIP());
  Serial.println(ANSI_RESET);
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