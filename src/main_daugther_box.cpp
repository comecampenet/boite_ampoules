#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// ### Pins ###
const int unlockPin = 22;
const int buttonPins[15] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};

// ### Declarations ###
bool lightStatus[15] = {0};  // Correct code (received via BLE)
int enteredCode[15] = {0};   // Code entered by the user

// ____BLE____
const char* serviceUUID = "19B10000-E8F2-537E-4F6C-D104768A1214";
const char* lightStatusCharacteristicUUID = "19B10001-E8F2-537E-4F6C-D104768A1214";

static boolean doConnect = false;
static boolean connected = false;
static BLEAddress* pServerAddress = nullptr;
static BLERemoteCharacteristic* lightStatusCharacteristic = nullptr;
bool newStatus = false;
const char* bleServerName = "Mother Light Box";

// Activation des notifications
const uint8_t notificationOn[] = {0x1, 0x0};
const uint8_t notificationOff[] = {0x0, 0x0};

// Callback de scan BLE
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        if (advertisedDevice.getName() == bleServerName) {
            advertisedDevice.getScan()->stop();
            pServerAddress = new BLEAddress(advertisedDevice.getAddress());
            doConnect = true;
            Serial.println("Device found. Connecting!");
        }
    }
};

// Callback de notification BLE (updates lightStatus)
static void lightStatusNotifyCallBack(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
    if (length == 15) {
        memcpy(lightStatus, pData, length);
        newStatus = true;

        // Debugging: Print updated lightStatus array
        Serial.print("Updated lightStatus: ");
        for (int i = 0; i < 15; i++) {
            Serial.print(lightStatus[i]);
            Serial.print(" ");
        }
        Serial.println();
    }
}


// variables pour le bouton appuyé
// Pins de sélection (entrée)
const int S0 = 17;
const int S1 = 18;
const int S2 = 19;

// Pins de sortie (lecture)
const int OUT1 = 20;
const int OUT2 = 21;

// Flag drapeau - Cela permet d'éviter que la fonction "appuye" 2 fois sur le même bouton à cause de sa vitesse d'exécution
const int flag_button = 1; 

// Tableau des états des boutons
bool buttonStates[15] = {0};



// ____ Fonctions ____
void setupBLE();
void updateLightStatus();
bool connectToServer(BLEAddress pAddress);
void updateButtonStatus();
bool checkCode();
void unlockBox();
void setupPins();
void scanButtons();

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
    updateLightStatus();
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
    BLEDevice::init("");
    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true);
    pBLEScan->start(30);
}

void updateLightStatus() {
    if (doConnect && pServerAddress != nullptr) {
        if (connectToServer(*pServerAddress)) {
            lightStatusCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t*)notificationOn, 2, true);
            connected = true;
        } else {
            Serial.println("Failed to connect to the server, restart the device.");
        }
        doConnect = false;
    }
}

bool connectToServer(BLEAddress pAdress) {
    BLEClient* pClient = BLEDevice::createClient();
    if (!pClient->connect(pAdress)) {
        Serial.println("Failed to connect to BLE server.");
        return false;
    }

    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
        Serial.println("Can't find the service.");
        return false;
    }

    lightStatusCharacteristic = pRemoteService->getCharacteristic(lightStatusCharacteristicUUID);
    if (lightStatusCharacteristic == nullptr) {
        Serial.println("Failed to find characteristic UUID.");
        return false;
    }

    Serial.println(" - Found our characteristics");
    lightStatusCharacteristic->registerForNotify(lightStatusNotifyCallBack);
    return true;
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
        if (enteredCode[i] != lightStatus[i]) {
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

