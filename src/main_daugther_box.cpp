#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLEClient.h>
#include <BLERemoteService.h>
#include <BLERemoteCharacteristic.h>

// ### Pins ###
const int unlockPin = 22;
const int buttonPins[15] = {13, 14, 15, 16, 17, 18, 19, 21, 23, 25, 26, 27, 32, 33, 34};

// ### Declarations ###
bool codeRes[15] = {0};  // Correct code (received via BLE)
int enteredCode[15] = {0};   // Code entered by the user

// ____BLE____
#define SERVICE_UUID  "19B10000-E8F2-537E-4F6C-D104768A1214"
#define CHARACTERISTIC_UUID "19B10001-E8F2-537E-4F6C-D104768A1214"


//bool receivedData[15] = {false}; // Array to store received booleans
BLEClient* pClient;
BLERemoteCharacteristic* pRemoteCharacteristic;
bool connected = false;
BLEScan* pBLEScan;
BLEAdvertisedDevice* myDevice = nullptr;

class MyClientCallback : public BLEClientCallbacks {
    void onConnect(BLEClient* pclient) {
        Serial.println("Connected to server");
    }

    void onDisconnect(BLEClient* pclient) {
        Serial.println("Disconnected from server");
        connected = false;
    }
};

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        if (advertisedDevice.haveServiceUUID() && advertisedDevice.getServiceUUID().equals(BLEUUID(SERVICE_UUID))) {
            Serial.println("Found matching server!");
            myDevice = new BLEAdvertisedDevice(advertisedDevice);
            advertisedDevice.getScan()->stop();
        }
    }
};


// variables pour le bouton appuyé
// Pins de sélection (entrée)
const int S0 = 17;
const int S1 = 18;
const int S2 = 19;

// Pins de sortie (lecture)
const int OUT1 = 20;
const int OUT2 = 21;

// Flag drapeau - Cela permet d'éviter que la fonction "appuye" 2 fois sur le même bouton à cause de sa vitesse d'exécution
int flag_button = 1; 

// Tableau des états des boutons
bool buttonStates[15] = {0};



// ____ Fonctions ____
void setupBLE();
void readCharacteristic();
void scanForServer();
bool connectToServer();
void updateButtonStatus();
bool checkCode();
void unlockBox();
void setupPins();
int scanButtons();

void setup() {
    Serial.begin(115200);
    pinMode(unlockPin, OUTPUT);
    digitalWrite(unlockPin, LOW);

    for (int i = 0; i < 15; i++) {
        pinMode(buttonPins[i], INPUT_PULLUP);
    }

    setupBLE();
    Serial.begin(115200);
    setupPins();
}

void loop() {
    
    if (!connected) {
        Serial.println("Scanning for BLE server...");
        scanForServer();
        if (myDevice != nullptr) {
            connected = connectToServer();
        }
    }

    if (connected) {
        readCharacteristic();
    }

    updateButtonStatus();

    if (checkCode()) {
        unlockBox();
    }

    delay(250);
    flag_button = scanButtons();
    if (flag_button == 1){
        delay(20);
    }
    delay(50); // 20 fois par seconde
}

void setupBLE() {
    BLEDevice::init("ESP32_BLE_Client");
    pClient = BLEDevice::createClient();
    pClient->setClientCallbacks(new MyClientCallback());
}

void scanForServer() {
    pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true);
    pBLEScan->start(10, false);
}

bool connectToServer() {
    if (myDevice == nullptr) {
        Serial.println("No BLE server found");
        return false;
    }
    Serial.println("Connecting to server...");
    pClient->connect(myDevice);
    BLERemoteService* pRemoteService = pClient->getService(SERVICE_UUID);
    if (pRemoteService == nullptr) {
        Serial.println("Failed to find service UUID");
        return false;
    }

    pRemoteCharacteristic = pRemoteService->getCharacteristic(CHARACTERISTIC_UUID);
    if (pRemoteCharacteristic == nullptr) {
        Serial.println("Failed to find characteristic UUID");
        return false;
    }
    connected = true;
    return true;
}

void readCharacteristic() {
    if (connected && pRemoteCharacteristic->canRead()) {
        std::string value = pRemoteCharacteristic->readValue();
        if (value.length() >= 15) {
            for (int i = 0; i < 15; i++) {
                codeRes[i] = value[i] != 0; // Convert byte to boolean
            }
            Serial.println("Received data:");
            for (int i = 0; i < 15; i++) {
                Serial.print(codeRes[i]);
                Serial.print(" ");
            }
            Serial.println();
        }
    }
}


// Deutsche Qualität
void updateButtonStatus() {
    for (int i = 0; i < 15; i++) {
        enteredCode[i] = digitalRead(buttonPins[i]) == LOW ? 1 : 0;
    }

    // Debugging: Print enteredCode
    Serial.print("Entered code: ");
    for (int i = 0; i < 15; i++) {
        Serial.print(enteredCode[i]);
        Serial.print(" ");
    }
    Serial.println();
}

bool checkCode() {
    for (int i = 0; i < 15; i++) {
        if (enteredCode[i] != codeRes[i]) {
            return false;
        }
    }
    return true;
}

void unlockBox() {
    Serial.println("Code correct, ouverture de la boîte !");
    digitalWrite(unlockPin, HIGH);
    delay(5000);
    digitalWrite(unlockPin, LOW);
}

void setupPins() {
    pinMode(S0, OUTPUT);
    pinMode(S1, OUTPUT);
    pinMode(S2, OUTPUT);
    pinMode(OUT1, INPUT);
    pinMode(OUT2, INPUT);
}

int scanButtons() {
    flag_button = 0;
    for (int i = 0; i < 8; i++) {
        digitalWrite(S0, i & 0x01);
        digitalWrite(S1, (i >> 1) & 0x01);
        digitalWrite(S2, (i >> 2) & 0x01);
        
        delayMicroseconds(10); // Petit délai pour la stabilité
        
        if (digitalRead(OUT1) == HIGH) {
            buttonStates[i] ^= buttonStates[i];
            flag_button = 1;            // bouton appuyé, attendre que le flag_button repasse à 0 pour appuyer.
        }
        
        if (digitalRead(OUT2) == HIGH) {
            buttonStates[8 + i] ^= buttonStates[8 + i];
            flag_button = 1;           // bouton appuyé, attendre que le flag_button repasse à 0 pour appuyer.
        }
    }
    return flag_button;
}

