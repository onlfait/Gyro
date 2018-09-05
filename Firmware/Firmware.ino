#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsClient.h>
#include <Hash.h>

#define AP_SSID "skbamk.net"
#define AP_PASSWORD "1-deux-3-quatre-5-six-7"

#define SERVER_IP "192.168.1.112"
#define SERVER_PORT 4224

ESP8266WiFiMulti WiFiMulti;
WebSocketsClient webSocket;

#define USE_SERIAL Serial

#define MESSAGE_INTERVAL 30000
#define HEARTBEAT_INTERVAL 25000

uint64_t messageTimestamp = 0;
uint64_t heartbeatTimestamp = 0;

bool isConnected = false;

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
    USE_SERIAL.printf("[WSc] Disconnected!\n");
    isConnected = false;
    break;
    case WStype_CONNECTED:
    {
      USE_SERIAL.printf("[WSc] Connected to url: %s\n",  payload);
      isConnected = true;

      // send message to server when Connected
      // socket.io upgrade confirmation message (required)
      webSocket.sendTXT("5");
    }
    break;
    case WStype_TEXT:
    USE_SERIAL.printf("[WSc] get text: %s\n", payload);

    // send message to server
    // webSocket.sendTXT("message here");
    break;
    case WStype_BIN:
    USE_SERIAL.printf("[WSc] get binary length: %u\n", length);
    hexdump(payload, length);

    // send data to server
    // webSocket.sendBIN(payload, length);
    break;
  }
}

void setup() {
  USE_SERIAL.begin(115200);

  //Serial.setDebugOutput(true);
  USE_SERIAL.setDebugOutput(true);

  USE_SERIAL.println();
  USE_SERIAL.println();
  USE_SERIAL.println();

  for(uint8_t t = 4; t > 0; t--) {
    USE_SERIAL.printf("[SETUP] BOOT WAIT %d...\n", t);
    USE_SERIAL.flush();
    delay(1000);
  }

  WiFiMulti.addAP(AP_SSID, AP_PASSWORD);

  //WiFi.disconnect();
  while(WiFiMulti.run() != WL_CONNECTED) {
    delay(100);
  }

  webSocket.beginSocketIO(SERVER_IP, SERVER_PORT);
  webSocket.onEvent(webSocketEvent);
}

void loop() {
  webSocket.loop();

  if(isConnected) {
    uint64_t now = millis();

    if(now - messageTimestamp > MESSAGE_INTERVAL) {
      messageTimestamp = now;
      USE_SERIAL.println("send message...");
      webSocket.sendTXT("4HelloWorld");
    }
    if((now - heartbeatTimestamp) > HEARTBEAT_INTERVAL) {
      heartbeatTimestamp = now;
      // socket.io heartbeat message
      webSocket.sendTXT("2");
    }
  }
}
