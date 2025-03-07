#include <Arduino.h>
// #include <ArduinoBLE.h> not working on esp32
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// ### pins ###
// Pin qui actionne l'ouverture de la boîte (faudra mettre le bon pin du coup)
const int unlockPin = 42;
// Tableau des numéros de pins associés aux boutons (jsp exactement si c'est bien ça les bons pins t'y as capté)
const int buttonPins[15] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};

// ### declarations ###
bool lightStatus[15]; //keep each lamp state
int enteredCode[15]; // Stocke le code entré par l'utilisateur
const int correctCode[15] = {1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1}; // Code attendu

// ____BLE____
const char* serviceUUID = "19B10000-E8F2-537E-4F6C-D104768A1214";
const char* lightStatusCharacteristicUUID = "19B10001-E8F2-537E-4F6C-D104768A1214";

//Flags stating if should begin connecting and if the connection is up
static boolean doConnect = false;
static boolean connected = false;

//Address of the peripheral device. Address will be found during scanning...
static BLEAddress *pServerAddress;

// remote characteristics
static BLERemoteCharacteristic* lightStatusCharacteristic

//Activate notify
const uint8_t notificationOn[] = {0x1, 0x0};
const uint8_t notificationOff[] = {0x0, 0x0};

bool newStatus = false;
const char* bleServerName = "Mother Light Box";

//Callback function that gets called, when another device's advertisement has been received
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
	void onResult(BLEAdvertisedDevice advertisedDevice) {
	  if (advertisedDevice.getName() == bleServerName) { //Check if the name of the advertiser matches
		advertisedDevice.getScan()->stop(); //Scan can be stopped, we found what we are looking for
		pServerAddress = new BLEAddress(advertisedDevice.getAddress()); //Address of advertiser is the one we need
		doConnect = true; //Set indicator, stating that we are ready to connect
		Serial.println("Device found. Connecting!");
		}
	}
};

// ____ functions ____

void setupBLE();
void updateLightStatus();
bool connectToServer(BLEAddress pAddress);
static void lightStatusNotifyCallBack(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify);
void updateButtonStatus();
void compareButtonAndLightStatus();
bool checkCode();
void unlockBox();


void setup()
{
	Serial.begin(115200);

	pinMode(unlockPin, OUTPUT);
    	digitalWrite(unlockPin, LOW); // S'assurer que le pin est bas au départ
	
	setupBLE();

}

void loop()
{
	updateLightStatus();
	updateButtonStatus();
	compareButtonAndLightStatus();
	if (checkCode()) {
        	unlockBox();
    	}
}

void setupBLE()
{
	BLEDevice::init("");
	BLEScan* pBLEScan = BLEDevice::getScan();
	pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
	pBLEScan->setActiveScan(true);
	pBLEScan->start(30);
}

void updateLightStatus()
{
	if (doConnect == true)
	{
		if (connectToServer(*pServerAddress))
		{
			lightStatusCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t*)notificationOn, 2, true);
			connected = true;
		}
		else
		{
			Serial.println("failed to connect to the server, restart the device");
		}
		doConnect = false;
	}
}

bool connectToServer(BLEAddress pAdress)
{
	BLEClient pClient = BLEDevice::createClient();
	pClient.connect(pAdress);
	BLERemoteService* pRemoteService->getService(serviceUUID);
	if (pRemoteService == nullptr)
	{
		Serial.println("can't find the service");
		return false;
	}
	lightStatusCharacteristic = pRemoteService->getCharacteristic(lightStatusCharacteristicUUID)
	if (temperatureCharacteristic == nullptr || humidityCharacteristic == nullptr) {
		Serial.print("Failed to find our characteristic UUID");
		return false;
	}
	Serial.println(" - Found our characteristics");
	lightStatusCharacteristic->registerForNotify(lightStatusNotifyCallback);
	return true;
}

static void lightStatusNotifyCallBack(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify)
{
	lightStatus = pData;
	newStatus = true;
}



/* Deutsche Qualität */
void updateButtonStatus() {
    for (int i = 0; i < 15; i++) {
		// Met à jour le statut du bouton, càd si c'est égal à HIGH ça vaut 1, sinon 0. (normalement ça devrait marcher)
        lightStatus[i] = (digitalRead(buttonPins[i]) == HIGH);
    }
}

void compareButtonAndLightStatus() {
    for (int i = 0; i < 15; i++) {
        if (lightStatus[i] != (digitalRead(buttonPins[i]) == HIGH)) {
		/* jsp si ça marche ça */
            	Serial.println("Incohérence à l'index %d",i);
		
		/* ça revient à faire ça normalement
		Serial.print("Incohérence à l'index");
		Serial.println(i);
		*/
        }
    }
}

bool checkCode() {
    for (int i = 0; i < 15; i++) {
        if (enteredCode[i] != correctCode[i]) {
            return false;
        }
    }
    return true;
}

void unlockBox() {
    Serial.println("Code correct, ouverture de la boîte !");
    digitalWrite(unlockPin, HIGH);
    delay(5000); // Maintenir l'ouverture 5 secondes
    digitalWrite(unlockPin, LOW);
}
