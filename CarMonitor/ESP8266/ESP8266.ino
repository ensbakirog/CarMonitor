#include <ESP8266WiFi.h>
#include <WebSocketsServer.h>

// --- Renk Kodları (Terminal Çıktısı İçin) ---
#define ANSI_RESET   "\x1b[0m"
#define ANSI_RED     "\x1b[31m"
#define ANSI_GREEN   "\x1b[32m"
#define ANSI_YELLOW  "\x1b[33m"
#define ANSI_BLUE    "\x1b[34m"
#define ANSI_MAGENTA "\x1b[35m"
#define ANSI_CYAN    "\x1b[36m"

// --- AP (Erişim Noktası) Ayarları ---
// ESP8266'nın oluşturacağı ağın adı ve şifresi
const char* AP_SSID = "ESP8266_OBD2_Network";
const char* AP_PASS = "12345678"; // Şifre en az 8 karakter olmalı

// ESP8266'nın kendi IP adresi (Bağlanan cihazlar bu IP'ye bağlanacak)
IPAddress local_IP(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

// --- WebSocket ve Seri Port Ayarları ---
const uint16_t WEBSOCKET_PORT = 1337;
const long SERIAL_BAUD_RATE = 115200;
const char PACKET_TERMINATOR = '\n';

WebSocketsServer webSocket(WEBSOCKET_PORT);

// --- Veri İşleme Değişkenleri ---
String stm32DataBuffer = "";
bool stm32DataReady = false;

// --- Fonksiyon Tanımları ---
void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length);
void setupSerial();
void setupWiFiAP(); // AP kurulumu için yeni fonksiyon
void setupWebSocket();
void handleSerialReception();
void broadcastSerialData();
void clearSerialBuffer();

void setup() {
  setupSerial();
  setupWiFiAP(); // WiFi tarama yerine AP başlatıyoruz
  setupWebSocket();
}

void loop() {
  // AP modunda "checkWiFiConnection" gerekmez çünkü ağı biz yayıyoruz.
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

void setupWiFiAP() {
  Serial.println(ANSI_MAGENTA "Setting up Access Point..." ANSI_RESET);

  // Modu Access Point olarak ayarla
  WiFi.mode(WIFI_AP);
  
  // IP Ayarlarını yapılandır (SoftAPConfig)
  Serial.print("Configuring AP IP to: ");
  Serial.println(local_IP);
  
  if (!WiFi.softAPConfig(local_IP, gateway, subnet)) {
    Serial.println(ANSI_RED "AP IP Configuration Failed!" ANSI_RESET);
  }

  // Ağı başlat (SSID ve Şifre ile)
  bool result = WiFi.softAP(AP_SSID, AP_PASS);

  if (result) {
    Serial.println(ANSI_GREEN "Access Point Started!" ANSI_RESET);
    Serial.print("Network Name (SSID): ");
    Serial.println(ANSI_CYAN + String(AP_SSID) + ANSI_RESET);
    Serial.print("IP Address: ");
    Serial.println(ANSI_CYAN + WiFi.softAPIP().toString() + ANSI_RESET);
    Serial.printf("Connect to this network and use IP %s on port %d\n", WiFi.softAPIP().toString().c_str(), WEBSOCKET_PORT);
  } else {
    Serial.println(ANSI_RED "Access Point Creation Failed!" ANSI_RESET);
  }
}

void setupWebSocket() {
  webSocket.begin();
  webSocket.onEvent(onWebSocketEvent);
  Serial.printf("WebSocket server started on port %d\n", WEBSOCKET_PORT);
  Serial.println("Waiting for STM32 data...");
  Serial.println();
}

void handleSerialReception() {
  if (stm32DataReady) return;
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
  // Not: AP modunda WiFi.status() kontrolüne gerek yoktur, 
  // ancak en az 1 bağlı istemci olup olmadığına bakabiliriz (webSocket.connectedClients() > 0).
  // Şimdilik sadece veri hazırsa gönderiyoruz.
  if (stm32DataReady) {
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
    case WStype_CONNECTED: {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        webSocket.sendTXT(num, "Welcome to ESP8266 OBD2 Server (AP Mode)!");
        break;
      }
    case WStype_TEXT:
      Serial.printf("[%u] get Text: %s\n", num, payload);
      break;
    case WStype_BIN: break;
  }
}