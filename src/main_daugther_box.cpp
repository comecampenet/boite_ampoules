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

// ### Declarations ###
bool codeRes[15] = {0};  // Correct code (received via BLE)

// ____BLE____
#define SERVICE_UUID  "19B10000-E8F2-537E-4F6C-D104768A1214"
#define CODE_RES_CHARACTERISTIC_UUID "19B10001-E8F2-537E-4F6C-D104768A1214"
#define BUTTON_STATUS_CHARACTERISTIC_UUID "19B10002-E8F2-537E-4F6C-D104768A1214"

//bool receivedData[15] = {false}; // Array to store received booleans
BLEClient* pClient;
BLERemoteCharacteristic* pRemoteCodeResCharacteristic;
BLERemoteCharacteristic* pRemoteButtonStatusCharacteristic;
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
// Multiplexer Pins
const int S0 = 5;
const int S1 = 17;
const int S2 = 16;

// Pins de sortie (lecture)
const int OUT1 = 18; // MUX 1 Output (Buttons 0-7)
const int OUT2 = 19; // MUX 2 Output (Buttons 8-15)
const int ENABLE = 4;

// Shift Register Pins (74HC595)
const int SER = 32;      // Serial Data Input (DS)
const int SRCLK = 26;    // Shift Register Clock (SHCP)
const int SRCLRbar = 27; // Shift Register Clear (MR - Active LOW)
const int RCLK = 25;     // Register Clock / Latch (STCP)
const int OEbar = 33;    // Output Enable (OE - Active LOW)

// Global state variable for LEDs (16 bits)
uint16_t leds = 0;
bool enteredCode[16] = {0};

// --- Debouncing Variables ---
const int NUM_BUTTONS = 16;
bool previousButtonState[NUM_BUTTONS] = {false}; // Store previous state (false = LOW/released)
unsigned long lastDebounceTime[NUM_BUTTONS] = {0}; // Store time of last transition
const unsigned long DEBOUNCE_DELAY = 50; // milliseconds



// ____ Fonctions ____
void setupBLE();
void readCodeResCharacteristic();
void writeButtonStatusCharacteristic();
void scanForServer();
bool connectToServer();
bool checkCode();
void unlockBox();

// Fonctions Shift-Register
void getButtons();
void clockPulseSRCLK();
void pulseRCLK();
void shiftBit(int bit);
void clearRegisters();
void setupMultiplexers();
void setupShiftRegister();
void shitfAndStoreTab();

void setup() {
    Serial.begin(115200);
    pinMode(unlockPin, OUTPUT);
    digitalWrite(unlockPin, LOW);

    setupBLE();
    
    Serial.println("ESP32 Shift Register & MUX Control Initialized");
    setupMultiplexers();
    setupShiftRegister();
}

long lastBLECheck = 0;
long waittime = 250;
// à changer
void loop() {
    if ((millis() - lastBLECheck) > waittime) {  
        lastBLECheck = millis();

        if (!connected) {
            Serial.println("Scanning for BLE server...");
            scanForServer();
            if (myDevice != nullptr) {
                connected = connectToServer();
            }
        }

        if (connected) {
            readCodeResCharacteristic();
            writeButtonStatusCharacteristic();
        }

        if (checkCode()) {
            unlockBox();
        }

    }

    // partie shift-register
    // Read buttons and update LED state
    getButtons();
    // Small delay to prevent busy-looping and allow serial buffer to empty
    delay(10);
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

    pRemoteCodeResCharacteristic = pRemoteService->getCharacteristic(CODE_RES_CHARACTERISTIC_UUID);
    pRemoteButtonStatusCharacteristic = pRemoteService->getCharacteristic(BUTTON_STATUS_CHARACTERISTIC_UUID);
    if (pRemoteCodeResCharacteristic == nullptr || pRemoteButtonStatusCharacteristic == nullptr) {
        Serial.println("Failed to find characteristic UUID");
        return false;
    }
    connected = true;
    return true;
}

void readCodeResCharacteristic() {
    if (connected && pRemoteCodeResCharacteristic->canRead()) {
        std::string value = pRemoteCodeResCharacteristic->readValue();
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

void writeButtonStatusCharacteristic() {
    if (connected && pRemoteButtonStatusCharacteristic->canWrite()) {
        pRemoteButtonStatusCharacteristic->writeValue((uint8_t*) enteredCode, 15);
    }
}

// Deutsche Qualität
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



//------------------------------------------------------------------------------
void getButtons() {
    bool stateChanged = false;
  
    for (int i = 0; i < 8; i++) {
      // Select MUX channel
      digitalWrite(S0, (i >> 0) & 0x1);
      digitalWrite(S1, (i >> 1) & 0x1);
      digitalWrite(S2, (i >> 2) & 0x1);
      delayMicroseconds(50); // Allow MUX outputs to settle
  
      // Read raw states (Assuming HIGH means pressed)
      bool currentButtonState1 = digitalRead(OUT1);
      bool currentButtonState2 = digitalRead(OUT2);
  
      // --- Debounce and Process Button 1 (index i) ---
      if ((millis() - lastDebounceTime[i]) > DEBOUNCE_DELAY) {
          // Check for a press (transition from LOW to HIGH)
          if (currentButtonState1 == HIGH && previousButtonState[i] == LOW) {
              leds ^= (1 << i); // Toggle the i-th bit
              enteredCode[i] ^= 1;
              Serial.printf("Button %d pressed. LED mask: 0x%04X\n", i, leds);
              stateChanged = true;
              lastDebounceTime[i] = millis(); // Record time of press detection
          }
      }
      // Update previous state only if stable reading differs (or always update after check)
      // Simple approach: update previous state regardless after check
       previousButtonState[i] = currentButtonState1;
  
  
      // --- Debounce and Process Button 2 (index i + 8) ---
      int buttonIndex2 = i + 8;
       if ((millis() - lastDebounceTime[buttonIndex2]) > DEBOUNCE_DELAY) {
          // Check for a press (transition from LOW to HIGH)
          if (currentButtonState2 == HIGH && previousButtonState[buttonIndex2] == LOW) {
              leds ^= (1 << buttonIndex2); // Toggle the (i+8)-th bit
              enteredCode[buttonIndex2] ^= 1;
              Serial.printf("Button %d pressed. LED mask: 0x%04X\n", buttonIndex2, leds);
              stateChanged = true;
              lastDebounceTime[buttonIndex2] = millis(); // Record time of press detection
          }
       }
       // Update previous state
       previousButtonState[buttonIndex2] = currentButtonState2;
    }
  
    // If any button press was detected and processed, update the LEDs
    if (stateChanged) {
      // shiftAndStoreValue(leds);
      shitfAndStoreTab();
    }
}


// Partie pour le shift-register
// Shifts a single bit into the register
void shiftBit(int bit) {
    digitalWrite(SER, bit ? HIGH : LOW);
    clockPulseSRCLK(); // Pulse Shift Register Clock
}


// Pulses the Shift Register Clock (SRCLK / SHCP)
void clockPulseSRCLK() {
    digitalWrite(SRCLK, HIGH);
    delayMicroseconds(10); // Adjust timing if needed
    digitalWrite(SRCLK, LOW);
    delayMicroseconds(10);
}

// Pulses the Register Clock / Latch (RCLK / STCP)
void pulseRCLK() {
    digitalWrite(RCLK, HIGH);
    delayMicroseconds(10); // Adjust timing if needed
    digitalWrite(RCLK, LOW);
    delayMicroseconds(10);
}

// Clears the shift registers (sets all outputs LOW)
void clearRegisters() {
    digitalWrite(SRCLRbar, LOW);
    delayMicroseconds(10);
    digitalWrite(SRCLRbar, HIGH); // Keep HIGH for normal operation
    pulseRCLK(); // Latch the cleared state to the outputs
}



//------------------------------------------------------------------------------
// Shifts 16 bits (MSB first) and then latches them to the outputs
// Corrected: Latch only ONCE after all bits are shifted.
// Corrected: Comment matches code (MSB first)
// Corrected: Uses uint16_t for clarity
  
void shitfAndStoreTab() {
    for (int i =0; i < 16;i++) {
      if (enteredCode[i] == true) {
        shiftBit(1);
      } else {
        shiftBit(0);
      }
    }
  
    pulseRCLK();
}



void setupMultiplexers() {
    /* Init Multiplexers */
    pinMode(ENABLE, OUTPUT);
    pinMode(OUT1, INPUT); // Consider INPUT_PULLDOWN if buttons connect to VCC
    pinMode(OUT2, INPUT); // Consider INPUT_PULLDOWN if buttons connect to VCC
    pinMode(S0, OUTPUT);
    pinMode(S1, OUTPUT);
    pinMode(S2, OUTPUT);
    digitalWrite(ENABLE, LOW); // Enable MUX outputs
}

void setupShiftRegister() {
      /* Init Shift Registers */
      pinMode(SER, OUTPUT);
      pinMode(SRCLK, OUTPUT);
      pinMode(SRCLRbar, OUTPUT);
      pinMode(RCLK, OUTPUT);
      pinMode(OEbar, OUTPUT);
    
      // Initial Shift Register State
      digitalWrite(SRCLK, LOW);
      digitalWrite(RCLK, LOW);
      digitalWrite(OEbar, HIGH);    // Disable outputs initially
      digitalWrite(SRCLRbar, LOW);  // Pulse clear LOW
      delayMicroseconds(10);
      digitalWrite(SRCLRbar, HIGH); // Keep HIGH for normal operation
      digitalWrite(OEbar, LOW);     // Enable outputs
    
      Serial.println("Shift registers cleared and enabled.");
    
      // Optional: Run LED test once at startup
      // testLedsInitial(); 
}