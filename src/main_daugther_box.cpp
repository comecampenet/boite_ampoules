#include <Arduino.h>
// #include <ArduinoBLE.h> not working on esp32
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// ### pins ###

// ### declarations ###

bool lightStatus[15]; //keep each lamp state 

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

// ____ functions ____

bool connectToServer(BLEAddress pAddress);

void setup()
{

}

void loop()
{

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