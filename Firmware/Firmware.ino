#include <FS.h>               // File System
#include <ESP8266WiFi.h>      // ESP8266 Core WiFi Library
#include <DNSServer.h>        // Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h> // Local WebServer used to serve the configuration portal
#include <WiFiManager.h>      // https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <WebSocketsClient.h> // https://github.com/Links2004/arduinoWebSockets WebSocket communication

#define LED_PIN 2
#define SETUP_PIN 0
#define AP_SSID "ESP-GYRO"
#define AP_TIMEOUT 180 // seconds
#define SERVER_TXT "/server.txt"

char server[40] = "esp-gyro.local";
char port[6] = "4242";

bool shouldSaveConfig = false;

WebSocketsClient webSocket;

void onWSConnect (uint8_t * payload, size_t length)  {
  Serial.printf("[WS] Connected to %s!\n", payload);
  webSocket.sendTXT("Connected");
}

void onWSDisconnect (uint8_t * payload, size_t length)  {
  Serial.printf("[WS] Disconnected from %s\n", payload);
}

void onWSText (uint8_t * payload, size_t length)  {
  Serial.printf("[WS] Text : %s\n", payload);
}

void onWSBin (uint8_t * payload, size_t length)  {
  Serial.printf("[WS] Bin : %u\n", length);
  hexdump(payload, length);
}

void webSocketEvent (WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED: onWSDisconnect(payload, length); break;
    case WStype_CONNECTED: onWSConnect(payload, length); break;
    case WStype_TEXT: onWSText(payload, length); break;
    //case WStype_BIN: onWSBin(payload, length); break;
  }
}

// initialization
void setup () {
  Serial.begin(115200);
  Serial.println();
  Serial.println();

  // set led pin mode
  pinMode(LED_PIN, OUTPUT);

  // init file system
  initConfig();
}

// init file system
void initConfig () {
  SPIFFS.begin();
  if (SPIFFS.exists(SERVER_TXT)) {
    loadConfig();
  } else {
    saveConfig();
  }
}

void saveConfig () {
  File file = SPIFFS.open(SERVER_TXT, "w+");
  if (file) {
    file.println(server);
    file.println(port);
    file.close();
  }
}

void loadConfig () {
  File file = SPIFFS.open(SERVER_TXT, "r");
  if (file) {
    strcpy(server, file.readStringUntil('\n').c_str());
    strcpy(port, file.readStringUntil('\n').c_str());
    file.close();
  }
}

// void listFile () {
//   Serial.println("listFile");
//   Dir dir = SPIFFS.openDir("/");
//   while (dir.next()) {
//     Serial.println(dir.fileName());
//     File file = dir.openFile("r");
//     Serial.println(file.readStringUntil('\n'));
//     Serial.println(file.readStringUntil('\n'));
//     file.close();
//   }
// }

// main loop
void loop () {
  webSocket.loop();

  // handle the number of (sucessive) clicks
  uint8_t clicks = clickCount();

  // auto connect
  if (clicks == 1) {
    autoConnect();
  }

  else if (clicks == 2) {
    webSocketBegin();
  }

  // DEBUG
  else if (clicks == 3) {
    Serial.print(F("server: "));
    Serial.println(server);
    Serial.print(F("port: "));
    Serial.println(port);
  }

  // reset settings
  else if (clicks == 5) {
    resetSettings();
    SPIFFS.format();
  }
}

// auto connect
void autoConnect () {
  // ...
  WiFiManager wifiManager;

  // id/name placeholder/prompt default length
  WiFiManagerParameter custom_server("server", "server", server, 40);
  WiFiManagerParameter custom_port("port", "port", port, 6);

  wifiManager.addParameter(&custom_server);
  wifiManager.addParameter(&custom_port);

  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.setTimeout(AP_TIMEOUT);

  if (!wifiManager.autoConnect(AP_SSID)) {
    return;
  }

  // read updated parameters
  strcpy(server, custom_server.getValue());
  strcpy(port, custom_port.getValue());

  Serial.print(F("Server: "));
  Serial.println(server);

  Serial.print(F("Port: "));
  Serial.println(port);

  // save config ?
  if (shouldSaveConfig) {
    Serial.println(F("Save config!"));
    shouldSaveConfig = false;
    saveConfig();
  }
}

void webSocketBegin () {
  // connect webSocket server
  Serial.print(F("webSocketBegin before"));
	webSocket.begin(server, atoi(port), "/");
  webSocket.setReconnectInterval(5000);
  webSocket.onEvent(webSocketEvent);
  Serial.print(F("webSocketBegin after"));
}

// reset WiFi settings
void resetSettings () {
  WiFiManager wifiManager;
  wifiManager.resetSettings();
}

// on connect and settings change
void saveConfigCallback () {
  shouldSaveConfig = true;
}

// blink the led
void blink (uint8_t count, uint16_t millis) {
  for (uint8_t i = 0; i < count; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(millis);
    digitalWrite(LED_PIN, LOW);
    delay(millis);
  }
}

// return the number of (sucessive) clicks
uint8_t clickCount () {
  static uint8_t clicks = 0;
  static bool lastValue = HIGH;
  static uint8_t debounce = 100;
  static uint16_t timeout = 1000;
  static unsigned long lastClick = 0;

  // get elapsed time
  unsigned long now = millis();
  unsigned long elapsed = now - lastClick;

  // read current value
  bool value = digitalRead(SETUP_PIN);

  // sync LED value
  digitalWrite(LED_PIN, value);

  // multiple clicks timeout
  if (clicks > 0 && elapsed > timeout) {
    uint8_t ret = clicks;
    blink(clicks, 200);
    clicks = 0;
    return ret;
  }

  // button up (click)
  if (value == HIGH && lastValue == LOW && elapsed > debounce) {
    lastClick = now;
    clicks++;
  }

  // update last value
  lastValue = value;
  return 0;
}
