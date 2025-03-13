#include <Arduino.h>
#include <WiFi.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// ##### Pins #####
const uint8_t IN_CLOSED = 34;
const uint8_t OUTPUTS[15] = {4, 13, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33, 5};

// ##### WiFi Credentials #####
const char* ssid = "La_Boite_Ampoules";
const char* password = "lejeulalejeu";

WiFiServer server(80);
String header;

// ##### BLE Declarations #####
const char* serviceUUID = "19B10000-E8F2-537E-4F6C-D104768A1214";
const char* characteristicUUID = "19B10001-E8F2-537E-4F6C-D104768A1214";
const char* bleServerName = "Mother Light Box";

BLECharacteristic *lightStatusCharacteristic;
bool deviceConnected = false;

// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 500;

// Memory state for lights
bool lightStatus[15] = {false};

// ##### BLE Callbacks #####
class MyServerCallbacks : public BLEServerCallbacks {
	void onConnect(BLEServer* pServer) { deviceConnected = true; }
	void onDisconnect(BLEServer* pServer) { deviceConnected = false; }
};

// ##### Function Declarations #####
void setupPins();
void setupWifi();
void setupBLE();
void transmitCode();
void displayWebPage(WiFiClient _client);

// ##### Main Setup #####
void setup() {
	Serial.begin(115200);
	setupPins();
	setupWifi();
	setupBLE();
}

void loop() {
  WiFiClient client = server.available();
  if (client) {
    Serial.println("New Client");
    String currentLine = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        header += c;

        if (c == '\n') {
          if (currentLine.length() == 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            for (int i = 0; i < 15; i++) {
              if (header.indexOf("GET /" + String(i) + "/on") >= 0) {
                Serial.println("Lamp " + String(i) + " ON");
                lightStatus[i] = true;
                transmitCode();
              } 
              else if (header.indexOf("GET /" + String(i) + "/off") >= 0) {
                Serial.println("Lamp " + String(i) + " OFF");
                lightStatus[i] = false;
                transmitCode();
              }
            }

            displayWebPage(client);
            client.println();
            break;
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }

    header = "";
    client.stop();
    Serial.println("Client disconnected");
  }

  if (digitalRead(IN_CLOSED) == HIGH) {
    for (int i = 0; i < 15; i++) {
      digitalWrite(OUTPUTS[i], lightStatus[i] ? LOW : HIGH);
      Serial.println("L" + String(i) + (lightStatus[i] ? " is ON" : " is OFF"));
    }
  } else {
    for (int i = 0; i < 15; i++) {
      digitalWrite(OUTPUTS[i], HIGH);
    }
  }

  delay(200);
}

// ##### Setup Functions #####
void setupPins() {
  pinMode(IN_CLOSED, INPUT);
  for (int i = 0; i < 15; i++) {
    pinMode(OUTPUTS[i], OUTPUT);
    digitalWrite(OUTPUTS[i], HIGH);
  }
}

void setupWifi() {
  Serial.println("Setting up WiFi AP...");
  WiFi.softAP(ssid, password);
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
  server.begin();
}

void setupBLE() {
  BLEDevice::init(bleServerName);
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(serviceUUID);
  lightStatusCharacteristic = pService->createCharacteristic(
    characteristicUUID, 
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY
  );

  lightStatusCharacteristic->setValue((uint8_t*)lightStatus, sizeof(lightStatus));
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(serviceUUID);
  pAdvertising->setScanResponse(true);
  BLEDevice::startAdvertising();

  Serial.println("BLE setup complete.");
}

// ##### Transmit BLE Data #####
void transmitCode() {
  if (deviceConnected && (millis() - lastTime > timerDelay)) {
    lightStatusCharacteristic->setValue((uint8_t*)lightStatus, sizeof(lightStatus));
    lightStatusCharacteristic->notify();
    lastTime = millis();
  }
}

// ##### Web Page Display #####
void displayWebPage(WiFiClient _client) {
  _client.println("<!DOCTYPE html><html lang=\"fr\">");
  _client.println("<head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">");
  _client.println("<title>Le jeu des ampoules</title>");
  _client.println("<style>body { display: flex; flex-direction: column; justify-content: center; align-items: center; height: 100vh; margin: 0; font-family: Arial, sans-serif; }");
  _client.println("h1 { font-size: 2em; color: #333; margin: 10px 0;}");
  _client.println(".grid { display: grid; grid-template-columns: repeat(3, 100px); grid-template-rows: repeat(5, 100px); gap: 10px; }");
  _client.println("button { width: 100px; height: 100px; color: white; border: none; border-radius: 40px; cursor: pointer; font-size: 16px; transition: 0.5s; }");
  _client.println("button:hover { background: #DAA520 !important; }</style></head>");
  _client.println("<body><h1>Le jeu des ampoules</h1>");
  _client.println("<p>Cliquez sur les boutons pour allumer les lampes</p>");
  _client.println("<div class=\"grid\">");

  for (int i = 0; i < 15; i++) {
    String color = lightStatus[i] ? "#FFD700" : "#555555";
    _client.println("<a href=\"/" + String(i) + (lightStatus[i] ? "/off" : "/on") + "\">");
    _client.println("<button style=\"background: " + color + ";\">L" + String(i) + "</button></a>");
  }

  _client.println("</div></body></html>");
}
