#include <Arduino.h>
#include <WiFi.h>
// #include <ArduinoBLE.h> not working on esp32 
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

/*
########################################
il faut tester comme ca la transmition puis voir comment on faire deux figures qui se composent pour donner la figure voulue
###############################
*/


// ##### pins ####

// pin to check if the box is closed
const uint8_t IN_CLOSED = 34;

// ------output-----
// pins for each lamp
const  uint8_t OUTPUTS[15] = {4, 13, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33, 5};

// #### declarations ####

// ------functions------
void setupPins();
void setupWifi();
void setupBLE();
void displayWebPage(WiFiClient _client);

// --------values----------
// _____wifi & query___

// ssid and pwd
const char* ssid = "La_Boite_Ampoules";
const char* password = "lejeulalejeu";

// create the wifi server on port 80
WiFiServer server(80);
String header; // store incoming the http header

//______BLE needs_____

// ----- UUIDs ----
const char* serviceUUID = "19B10000-E8F2-537E-4F6C-D104768A1214";
const char* lightStatusCharacteristicUUID = "19B10001-E8F2-537E-4F6C-D104768A1214";
bool deviceConnected = false;

const char* bleServerName = "Mother Light Box";

// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 30000;

//Setup callbacks onConnect and onDisconnect
class MyServerCallbacks: public BLEServerCallbacks {
	void onConnect(BLEServer* pServer) {
		deviceConnected = true;
	};
	void onDisconnect(BLEServer* pServer) {
		deviceConnected = false;
	}
};

// _______status variables ______

// memory state
bool lightStatus[15]; //keep each lamp state 

// #### main code ####

// main 
void setup() {
	Serial.begin(115200);
	setupPins();
	setupWifi();
	setupBLE();
}

void loop() {
	// check for new client and store it as a variable
	WiFiClient client = server.available();

	// if there's a client connected 
	if (client)
	{
    	// notify that there is a client connected
		Serial.println("New Client");
    	// store the current line of the request
		String currentLine = "";
    
		while (client.connected())
		{
    	  // check if there's data available to read
			if (client.available())
			{
    	    	char c = client.read(); // read a byte
				Serial.write(c);  
				header += c;  // add the byte to the header variable
				if (c == '\n') // if it's a newline, we know this is the end of the current line
				{
    	      		if  (currentLine.length() == 0) // if there is nothing in the currentLine
					{
    	        	// http headers start
    	        	client.println("HTTP/1.1 200 OK"); // send http status
    	        	client.println("Content-type:text/html"); // set the content type of the response
    	        	client.println("Connection: close"); // notify that the connection will be closed
    	       		client.println(); // end of the headers

    	        	// display figure
					for (int i = 0; i < 15; i++)
					{
    	          		if (header.indexOf("GET /" + String(i) + "/on") >= 0) // if the i button is asked to be on 
						{
    	            		Serial.println("lampe " + String(i) + " commanded to be on"); // log that it's on
    	            		lightStatus[i] = true; // save the status of the lamp
							transmitCode(); // to transmit the code only when changed
						}
    	          		else if (header.indexOf("GET /" + String(i) + "/off") >= 0) // if the i button is asked to be off
						{
    	            		Serial.println("lampe " + String(i) + "commanded to be off"); // log that it's off
    	            		lightStatus[i] = false; // save the status  of the lamp
							transmitCode(); // to transmit the code only when changed
						}
					}
    	          	// display the web page
					displayWebPage(client);

					// end of the http reponse
					client.println();
					break; // exit the while		
					}
					else 
					{
    	        	currentLine = ""; // clear the current line for the next iteration
					}
				}
    	    	else if (c != '\r') // add only bytes that are not carriage return to the current line
				{
					currentLine += c;
				}
			}
		}
    	// clear the header variable
		header = "";

    	// close connection 
		client.stop();
		Serial.println("Client disconnected");
	}

  	// turn on and off the lights
	if (digitalRead(IN_CLOSED) == HIGH)
	{
    for  (int i = 0; i < 15; i++)
    {
		if (lightStatus[i] == true)
		{
        	digitalWrite(OUTPUTS[i],LOW); // set on the pin for the lamp (inversion sue to the circuit)
			Serial.println("L" +  String(i) + " is on");
		}
		else if (lightStatus[i] == false)
		{
        	digitalWrite(OUTPUTS[i],HIGH); // set off the pin for the lamp (inversion sue to the circuit)
		}
		delay(3);
	}
	}
	else if (digitalRead(IN_CLOSED) == LOW)
	{
		for  (int i = 0; i < 15; i++)
		{
      		digitalWrite(OUTPUTS[i],HIGH); // set off the pin for the lamp
			delay(3);
		}
	}
	delay(200);
}

void setupPins()
{
  	//set modes for each pins and initialize output states
	pinMode(IN_CLOSED,INPUT);

	for (int i = 0; i < 15; i++) {
		pinMode(OUTPUTS[i], OUTPUT);
		delay(3);
    	digitalWrite(OUTPUTS[i],HIGH); // set off the pin for the lamp
		lightStatus[i] = false;
	}
}

void setupWifi()
{
    // start the WiFi access point
    Serial.print("Setting AP");
    WiFi.softAP(ssid, password);

    // get and  print the IP adress of the AP 
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);
    
    //  start the server
    server.begin();
}

void setupBLE()
{
	BLEDevice::init(bleServerName);
	BLEServer *pServer = BLEDevice::createServer();
	BLEService *pService = pServer->createService(serviceUUID);
	BLECharacteristic *pCharacteristic = pService->createCharacteristic(serviceUUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE );
	pCharacteristic->setValue(lightStatus);
	pService->start();

	BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
	pAdvertising->addServiceUUID(serviceUUID);
	pAdvertising->setScanResponse(true);
	BLEDevice::startAdvertising();
	Serial.println("Characteristic defined");

}

void transmitCode()
{
	if (deviceConnected)
	{
		if ((millis - lastTime) > timerDelay)
		{
			lightStatusCharacteristic.setValue(lightStatus) // a tester le passage par valeur directe, sinon il faudra passer par string ###################
			lightStatusCharacteristic.notify();
			lastTime = millis();
		}
	}
}

void displayWebPage(WiFiClient _client)
{
  // ## head ##
	_client.println("<!DOCTYPE html><html lang=\"fr\">");
	_client.println("<head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">");
	_client.println("<title>Le jeu des ampoules commande</title>");
  // __style css__
	_client.println("<style> body { display: flex; flex-direction: column; justify-content: center; align-items: center; height: 100vh; margin: 0; font-family: Arial, sans-serif; }");
	_client.println("h1 { font-size: 2em; color: #333; margin: 10px 0;}");
	_client.println("p { font-size: 1.2em; color: #555; text-align: center; margin: 0 0 20px 0; }");
	_client.println(".grid { display: grid; grid-template-columns: repeat(3, 100px); grid-template-rows: repeat(5, 100px); gap: 10px; }");
	_client.println("a { text-decoration: none; }");
	_client.println("button { width: 100px; height: 100px; color: white; border: none; border-radius: 40px; cursor: pointer; font-size: 16px; transition: 0.5s; }");
	_client.println("button:hover { background: #DAA520 !important; }</style></head>");
  // __body page__
	_client.println("<body><h1>Le jeu des ampoules</h1>");
	_client.println("<p>Cliquez sur les boutons pour allumer les lampes</p>");
  // button grid
	_client.println("<div class=\"grid\">");
	for (int i = 0; i < 15; i++)
	{
		if(lightStatus[i] == true)
		{
			_client.println("<a href=\"/" + String(i) + "/off\"><button style=\"background: #FFD700;\">L" + String(i) + "</button></a>");
		}
		else if (lightStatus[i] == false)
		{
			_client.println("<a href=\"/" + String(i) + "/on\"><button style=\"background: #555555;\">L" + String(i) + "</button></a>");
		}
	}
	_client.println("</div>");
	_client.println("</body></html>");
}