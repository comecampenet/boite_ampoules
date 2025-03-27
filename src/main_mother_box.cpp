#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

// Memory state for lights
typedef bool CODE[15];
CODE codeRes = {false};
CODE encodedCodeA = {false};
CODE encodedCodeB = {false};
CODE buttonStatus = {false};
String gameMode = "playing";

// ##### Pins #####
const uint8_t IN_CLOSED = 34;
const uint8_t OUTPUTS[15] = {4, 13, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33, 5};

// ##### WiFi Credentials #####
const char* ssid = "La_Boite_Ampoules";
const char* password = "lejeulalejeu";

WebServer server(80);

const char statusPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="fr">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Contrôleur Jeu des Ampoules</title>
    <style>
        body { font-family: Arial, sans-serif; text-align: center; /*background: rgb(251,247,63); background: radial-gradient(circle, rgba(251,247,63,1) 0%, rgba(252,70,185,1) 100%) */}
        select {background-color: #DAA520; /* Match the button background color */color: white; /* Text color */font-weight: bold; /* Bold text */padding: 0.8em; /* Increased padding for better readability */font-size: 1.1em; /* Increased font size for better readability */border: none; /* Remove default border */border-radius: 5px; /* Rounded corners */box-shadow: 2px 2px 5px rgba(0, 0, 0, 0.2); /* Shadow */cursor: pointer; /* Pointer cursor */transition: background-color 0.3s; /* Transition effect */}
        select:hover {background-color: #CE2029; /* Darker shade on hover */}
        .container { display: flex; justify-content: center; gap: 2em; margin-top: 2em; border-radius: 25px; overflow: hidden; }
        .code-table { border-collapse: collapse;}
        .code-table td { width: 2.5em; height: 2.5em; text-align: center; border: 0.5em solid #ccc;}
        .code-table .on { background-color: gold; 
-webkit-box-shadow:0px 0px 61px 0px rgba(255,217,0,1);
-moz-box-shadow: 0px 0px 61px 0px rgba(255,217,0,1);
box-shadow: 0px 0px 61px 0px rgba(255,217,0,1);}
        .code-table .off { background-color: gray; }
        .side-section {display: flex; flex-direction: column;}
        .main-section {display: flex; flex-direction: column; margin-top: 5em;}
        .option-button {margin-top:1em; background-color: #20cec5;  color: white; /* Text color */ font-weight: bold;   padding: 0.6em 0; /* Padding */    font-size: 1.1em; /* Font size */    border: none; /* Remove default border */    border-radius: 5px; /* Rounded corners */    box-shadow: 2px 2px 5px rgba(0, 0, 0, 0.2); /* Shadow */    cursor: pointer; /* Pointer cursor */    text-align: center; /* Center text */ transition: background-color 0.3s; /* Transition effect */}
        .option-button:hover {background-color: #CE2029; /* Darker shade on hover */}
        .link-to-setup { background-color: #DAA520;  color: white; /* Text color */ font-weight: bold;   padding: 0.6em 0; /* Padding */    font-size: 1.1em; /* Font size */    border: none; /* Remove default border */    border-radius: 5px; /* Rounded corners */    box-shadow: 2px 2px 5px rgba(0, 0, 0, 0.2); /* Shadow */    cursor: pointer; /* Pointer cursor */    text-align: center; /* Center text */ transition: background-color 0.3s; /* Transition effect */ text-decoration: none;}
        .link-to-setup:hover {background-color: #CE2029;}
        /*
        #is-box-open.open {border: 5px solid #45A049; border-radius: 10px;}
        #is-box-open.closed {border: 5px solid #CE2029; border-radius: 10px;}
        */
    </style>
</head>
<body>

    <h1>Contrôleur Jeu des Ampoules</h1>

    <div class="container">
        <div class="side-section">
            <div>
                <h3>Code Entré</h3>
                <table id="buttonStatus" class="code-table"></table>
            </div>
            <div>
                <h3>Code Solution</h3>
                <table id="currentCode" class="code-table"></table>
            </div>
        </div>
        <div class="main-section">
            <!--
                <h2 id="is-box-open" class="open">Boite Ouverte</h2>
                <button id="open-box" class="option-button" style="margin-top: 0;">Ouvrir Boite</button>
            
            <h2 id="displayed-code" >Code affiché : A</h2>
        -->
            <a href="\changeCode" class="link-to-setup">Modifier Code Solution</a>
            <h2>Mode de fonctionnement</h2>
                <select name="displayMode" id="displayMode" >
                    <option value="playing">Jeu Normal</option>
                    <optgroup label="Modes Alternatifs">
                        <option value="codeRes">Affichage Code Solution</option>
                        <option value="A">Affichage Code A</option>
                        <option value="B">Affichage Code B</option>
                    </optgroup>
                </select>
        </div>
        <div class="side-section">
            <h3>Code A</h3>
            <table id="A" class="code-table"></table>
            <h3>Code B</h3>
            <table id="B" class="code-table"></table>
        </div>
    </div>

    <script>
        const rows = 5, cols = 3;
        let buttonStatus = new Array(rows * cols).fill(false);
        let currentCode = new Array(rows * cols).fill(false);
        let encodedCodeA = new Array(rows * cols).fill(false);
        let encodedCodeB = new Array(rows * cols).fill(false);
        let mode = "playing";

        const modeChoice = document.getElementById("displayMode");

        function index(i, j) {
            return i * cols + j;
        }

        function generateTable(id, data) {
            const table = document.getElementById(id);
            table.innerHTML = "";
            for (let i = 0; i < rows; i++) {
                const tr = document.createElement("tr");
                for (let j = 0; j < cols; j++) {
                    const td = document.createElement("td");
                    const idx = index(i, j);
                    td.className = data[idx] ? "on" : "off";
                    tr.appendChild(td);
                }
                table.appendChild(tr);
            }
        }

        async function fetchButtonStatus() {
            try {
                const response = await fetch("/getButtonStatus");
                if (!response.ok) throw new Error("Failed to fetch button status");
            
                const data = await response.json();
                buttonStatus = data.buttonStatus;  // Assign received array
                generateTable("buttonStatus", buttonStatus);
            } catch (error) {
                console.error("Error fetching button status:", error);
            }
        }

        async function fetchCodeRes() {
            try {
                const response = await fetch("/getCodeRes");
                if (!response.ok) throw new Error("Failed to fetch codeRes");
            
                const data = await response.json();
                currentCode = data.codeRes;  // Assign received array
                console.log("Received codeRes:", currentCode);
                generateTable("currentCode", currentCode);
                

                //updateUI();  // Call function to update UI if needed
            } catch (error) {
                console.error("Error fetching codeRes:", error);
            }
        }

        async function fetchEncodedCodes() {
            try {
                const response = await fetch("/getEncodedCodes");
                if (!response.ok) throw new Error("Failed to fetch encoded codes");
            
                const data = await response.json();
                encodedCodeA = data.encodedCodeA;  // Assign received array
                encodedCodeB = data.encodedCodeB;  // Assign received array
                generateTable("A", encodedCodeA);
                generateTable("B", encodedCodeB);
            } catch (error) {
                console.error("Error fetching encoded codes:", error);
            }
        }

        function postGameMode() {
            if (modeChoice.value !== mode)
            {
                mode = modeChoice.value;
                const data = {
                    mode: mode
                };
            
                fetch("/postGameMode", {
                    method: "POST",
                    headers: { "Content-Type": "application/json" },
                    body: JSON.stringify(data)
                })
                .then(response => response.text())
                .then(response => {
                    console.log("ESP32 Response:", response);
                    alert("Mode envoyé !");
                })
                .catch(error => console.error("Error:", error));
            }
        }
/*
        async function fetchBoxOpen() {
            try {
                const response = await fetch("/getBoxOpen");
                if (!response.ok) throw new Error("Failed to fetch box open");
                const data = await response.json();
                const isBoxOpen = data.isBoxOpen;  // Assign received array
                const element = document.getElementById("isBoxOpen");
                // à competer pour update le display
                if (isBoxOpen)
                {
                    element.innerHTML = "Boite Ouverte";
                    element.className = "open";
                }
                else
                {
                    element.innerHTML = "Boite Fermée";
                    element.className = "closed";
                }
            } catch (error) {
                console.error("Error fetching box open:", error);
            }
        }
*/
        // Call the functions on page load
        fetchButtonStatus();
        fetchCodeRes();
        fetchEncodedCodes();
        postGameMode();
        
        setInterval(refreshCurrentCodeHandler, 1000);  // Refresh every second
        function refreshCurrentCodeHandler(){
            fetchButtonStatus();
            fetchCodeRes();
            fetchEncodedCodes();
            postGameMode();
        }

        generateTable("buttonStatus",buttonStatus);
        generateTable("currentCode",currentCode);
        generateTable("A",encodedCodeA);
        generateTable("B",encodedCodeB);

    </script>

</body>
</html>

)rawliteral";

const char commandPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="fr">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Contrôleur Jeu des Ampoulles</title>
    <style>
        body { font-family: Arial, sans-serif; text-align: center; /*background: rgb(251,247,63); background: radial-gradient(circle, rgba(251,247,63,1) 0%, rgba(252,70,185,1) 100%) */}
        .container { display: flex; justify-content: center; gap: 2em; margin-top: 2em; border-radius: 25px; overflow: hidden; }
        .code-table { border-collapse: collapse;}
        .code-table td { width: 2.5em; height: 2.5em; text-align: center; border: 0.5em solid #ccc;}
        .code-table .on { background-color: gold; 
-webkit-box-shadow:0px 0px 61px 0px rgba(255,217,0,1);
-moz-box-shadow: 0px 0px 61px 0px rgba(255,217,0,1);
box-shadow: 0px 0px 61px 0px rgba(255,217,0,1);}
        .code-table .off { background-color: gray; }
        .code-table-editable { border-collapse: collapse;}
        .code-table-editable td { width: 6em; height: 6em; text-align: center; cursor: pointer; border: 1em solid #ccc; transition: background-color 0.3s;}
        .code-table-editable td:hover { background: #DAA520 !important; }
        .code-table-editable .on { background-color: gold; 
-webkit-box-shadow:0px 0px 61px 0px rgba(255,217,0,1);
-moz-box-shadow: 0px 0px 61px 0px rgba(255,217,0,1);
box-shadow: 0px 0px 61px 0px rgba(255,217,0,1);
}
        .code-table-editable .off { background-color: gray; }
        .side-section {display: flex; flex-direction: column;}
        .send-button {margin-top:2em; background-color: #CE2029;  color: white; /* Text color */font-weight: bold;    padding: 0.6em 2em; /* Padding */    font-size: 1em; /* Font size */    border: none; /* Remove default border */    border-radius: 5px; /* Rounded corners */    box-shadow: 2px 2px 5px rgba(0, 0, 0, 0.2); /* Shadow */    cursor: pointer; /* Pointer cursor */    text-align: center; /* Center text */ transition: background-color 0.3s; /* Transition effect */}
        .encode-button {margin-top:2em; background-color: #DAA520;  color: white; /* Text color */font-weight: bold;    padding: 0.6em 2em; /* Padding */    font-size: 1em; /* Font size */    border: none; /* Remove default border */    border-radius: 5px; /* Rounded corners */    box-shadow: 2px 2px 5px rgba(0, 0, 0, 0.2); /* Shadow */    cursor: pointer; /* Pointer cursor */    text-align: center; /* Center text */ transition: background-color 0.3s; /* Transition effect */}
        .option-button {margin-top:1em; background-color: #20cec5;  color: white; /* Text color */font-weight: bold;    padding: 0.6em 0; /* Padding */    font-size: 1em; /* Font size */    border: none; /* Remove default border */    border-radius: 5px; /* Rounded corners */    box-shadow: 2px 2px 5px rgba(0, 0, 0, 0.2); /* Shadow */    cursor: pointer; /* Pointer cursor */    text-align: center; /* Center text */ transition: background-color 0.3s; /* Transition effect */}
        .send-button:hover {background-color: #45a049; /* Darker shade on hover */}
        .encode-button:hover {background-color: #45a049; /* Darker shade on hover */}
        .option-button:hover {background-color: #CE2029; /* Darker shade on hover */}
        .link-to-monitor { background-color: #DAA520; margin-top: 1em; color: white; /* Text color */ font-weight: bold;   padding: 0.6em 0; /* Padding */    font-size: 1.1em; /* Font size */    border: none; /* Remove default border */    border-radius: 5px; /* Rounded corners */    box-shadow: 2px 2px 5px rgba(0, 0, 0, 0.2); /* Shadow */    cursor: pointer; /* Pointer cursor */    text-align: center; /* Center text */ transition: background-color 0.3s; /* Transition effect */ text-decoration: none;}
        .link-to-monitor:hover {background-color: #CE2029;}
    </style>
</head>
<body>

    <h1>Contrôleur Jeu des Ampoules</h1>

    <div class="container">
        <div class="side-section">
            <h3>Code Actuel</h3>
            <div>
                <table id="currentCode" class="code-table"></table>
            </div>
            <div class="side-section"style="margin-top: 0.65em;">
                <h3>Options</h3>
                <button style="margin-top: 0;" class="option-button" onclick="randomize()">Randomize</button>
                <button class="option-button" onclick="drawNumber()">Dessiner un chiffre</button>
                <button class="option-button" onclick="clearEditCode()">Clear</button>
                <button class="option-button" onclick="fillEditCode()">Fill</button>
                <a href="/" class="link-to-monitor">Monitor</a>
            </div>
        </div>
        <div>
            <h3>Code en Édition</h3>
            <table id="editCode" class="code-table-editable"></table>
            <button class="send-button" onclick="sendToESP32()">Envoyer code</button>
            <button class="encode-button" onclick="encode()">Encoder</button>
        </div>
        <div class="side-section">
            <div>
                <h3>Encodé A</h3>
                <table id="A" class="code-table"></table>
            </div>
            <div style="margin-top: 0.65em">
                <h3>Encodé B</h3>
                <table id="B" class="code-table"></table>
            </div>
        </div>
    </div>

    <script>
        const rows = 5, cols = 3;
        let encoded = false;
        let currentCode = new Array(rows * cols).fill(false);
        let editCode = new Array(rows * cols).fill(false);
        let encodedCodeA = new Array(rows * cols).fill(false);
        let encodedCodeB = new Array(rows * cols).fill(false);

        function index(i, j) {
            return i * cols + j;
        }

        function generateTable(id, data, editable = false) {
            const table = document.getElementById(id);
            table.innerHTML = "";
            for (let i = 0; i < rows; i++) {
                const tr = document.createElement("tr");
                for (let j = 0; j < cols; j++) {
                    const td = document.createElement("td");
                    const idx = index(i, j);
                    td.className = data[idx] ? "on" : "off";
                    if (editable) {
                        td.onclick = () => {
                            encoded = false;
                            data[idx] = !data[idx];
                            td.className = data[idx] ? "on" : "off";
                        };
                    }
                    tr.appendChild(td);
                }
                table.appendChild(tr);
            }
        }

        async function fetchCodeRes() {
            try {
                const response = await fetch("/getCodeRes");
                if (!response.ok) throw new Error("Failed to fetch codeRes");
            
                const data = await response.json();
                currentCode = data.codeRes;  // Assign received array
                console.log("Received codeRes:", currentCode);
                generateTable("currentCode", currentCode);
                

                //updateUI();  // Call function to update UI if needed
            } catch (error) {
                console.error("Error fetching codeRes:", error);
            }
        }

        // Call the function on page load
        fetchCodeRes();
        setInterval(refreshCurrentCodeHandler, 1000);  // Refresh every second
        function refreshCurrentCodeHandler(){
            fetchCodeRes();
        }

        function randomize() {
            for (let i = 0; i < rows * cols; i++) {
                editCode[i] = Math.random() < 0.5;
            }
            generateTable("editCode", editCode, true);
        }

        function clearEditCode() {
            for (let i = 0; i < rows * cols; i++) {
                editCode[i] = false;
            }
            generateTable("editCode", editCode, true);
        }

        function fillEditCode() {
            for (let i = 0; i < rows * cols; i++) {
                editCode[i] = true;
            }
            generateTable("editCode", editCode, true);
        }

        function drawNumber() {
            const numbers = [
                [
                    true,  true,  true,
                    true,  false, true,
                    true,  false, true,
                    true,  false, true,
                    true,  true,  true
                ],[
                    false, true,  false,
                    true,  true,  false,
                    false, true,  false,
                    false, true,  false,
                    true,  true,  true,
                ],[
                    true,  true,  true,
                    false, false, true,
                    true,  true,  true,
                    true,  false, false,
                    true,  true,  true
                ],[
                    true,  true,  true,
                    false, false, true,
                    true,  true,  true,
                    false, false, true,
                    true,  true,  true,
                ],[
                    true,  false, true,
                    true,  false, true,
                    true,  true,  true,
                    false, false, true,
                    false, false, true,
                ],[
                    true,  true,  true,
                    true, false, false,
                    true,  true,  true,
                    false, false, true,
                    true,  true,  true,
                ],[
                    true,  true,  true,
                    true, false, false,
                    true,  true,  true,
                    true, false,  true,
                    true,  true,  true
                ],[
                    true,  true,  true,
                    false, false, true,
                    false, true,  true,
                    false, false, true,
                    false, false, true,
                ],[
                    true,  true,  true,
                    true,  false, true,
                    true,  true,  true,
                    true,  false, true,
                    true,  true,  true,
                ],[
                    true,  true,  true,
                    true,  false, true,
                    true,  true,  true,
                    false, false, true,
                    true,  true,  true,
                ]]
            let num = prompt("Entrer un chiffre ");
            if (num < '0' || num > '9' || num < 0 || num > 9)
            {
                alert("Veuillez entrer un nombre entre 0 et 9");
                return;
            }
            if (num == null)
            {
                return;
            }
            editCode = numbers[num];
            generateTable("editCode", editCode, true);
        }

        function encode() {
            /*
            - on + on = off
            - off + off = on
            - on + off = on
            */
            encoded = true;
            for (let i = 0; i < rows*cols; i++)
            {
                if (!editCode[i])
                {
                    encodedCodeA[i] = true;
                    encodedCodeB[i] = true;
                }
                else if (editCode[i])
                {
                    let rand = Math.floor(Math.random() * 3);
                    if (rand == 0)
                    {
                        encodedCodeA[i] = false;
                        encodedCodeB[i] = false;
                    }
                    else if (rand == 1)
                    {
                        encodedCodeA[i] = false;
                        encodedCodeB[i] = true;
                    }
                    else if (rand == 2)
                    {
                        encodedCodeA[i] = true;
                        encodedCodeB[i] = false;
                    }
                }
            }
            generateTable("A", encodedCodeA);
            generateTable("B", encodedCodeB);
        }

        function sendToESP32() {
            if (!encoded) {
                encode();
            }
            const data = {
                codeA: encodedCodeA,
                codeB: encodedCodeB,
                codeRes: editCode
            };
        
            fetch("/updateTables", {
                method: "POST",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify(data)
            })
            .then(response => response.text())
            .then(response => {
                console.log("ESP32 Response:", response);
                alert("Code envoyé !");
                currentCode = [...editCode]; // Met à jour le code actuel
                generateTable("currentCode", currentCode);
            })
            .catch(error => console.error("Error:", error));
        }

        generateTable("currentCode", currentCode);
        generateTable("editCode", editCode, true);
        generateTable("A", encodedCodeA);
        generateTable("B", encodedCodeB);
    </script>

</body>
</html>
    )rawliteral";

// ##### BLE Declarations #####
const char* serviceUUID = "19B10000-E8F2-537E-4F6C-D104768A1214";
const char* codeResCharacteristicUUID = "19B10001-E8F2-537E-4F6C-D104768A1214";
const char* buttonStatusCharacteristicUUID = "19B10002-E8F2-537E-4F6C-D104768A1214";
const char* bleServerName = "Mother Light Box";

BLECharacteristic *codeResCharacteristic;
BLECharacteristic *buttonStatusCharacteristic;
bool deviceConnected = false;

// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 500;


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
void readButtonStatusCharacteristic();
void handleRoot();
void handleCommandPage();
void handlePostCodes();
void handleGetCodeRes();
void handleGetEncodedCodes();
void handlePostGameMode();
void handleGetButtonStatus();


// ##### Main Setup #####
void setup() {
	Serial.begin(115200);
	setupPins();
	setupWifi();
	setupBLE();
}

void loop() {
    server.handleClient();
    transmitCode();


    delay(300);
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
    WiFi.softAP(ssid, password);
    server.on("/", handleRoot);
    server.on("/changeCode", handleCommandPage);
    server.on("/updateTables", HTTP_POST, handlePostCodes);
    server.on("/getCodeRes", HTTP_GET, handleGetCodeRes);
    server.on("/getEncodedCodes", HTTP_GET, handleGetEncodedCodes);
    server.on("/postGameMode", HTTP_POST, handlePostGameMode);
    server.on("/getButtonStatus", HTTP_GET, handleGetButtonStatus);
    server.begin();
    Serial.println("ESP32 HTTP Server started");
}

void setupBLE() {
    BLEDevice::init(bleServerName);
    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    BLEService *pService = pServer->createService(serviceUUID);
    codeResCharacteristic = pService->createCharacteristic(
        codeResCharacteristicUUID, 
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY
    );
    buttonStatusCharacteristic = pService->createCharacteristic(
        buttonStatusCharacteristicUUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY
    );

    codeResCharacteristic->setValue((uint8_t*)codeRes, sizeof(codeRes));
    buttonStatusCharacteristic->setValue((uint8_t*)buttonStatus, sizeof(buttonStatus));
    pService->start();

    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(serviceUUID);
    pAdvertising->setScanResponse(true);
    BLEDevice::startAdvertising();

    Serial.println("BLE setup complete.");
}

// ##### Handle the lamps
void displayCode(CODE code)
{
    for (int i = 0; i < 15; i++)
    {
        if (code[i])
        {
            digitalWrite(OUTPUTS[i],LOW);
        }
        else if (!code[i])
        {
            digitalWrite(OUTPUTS[i],HIGH);
        }
    }
}

// ##### Transmit BLE Data #####
void transmitCode() {
    if (deviceConnected && (millis() - lastTime > timerDelay)) {
        codeResCharacteristic->setValue((uint8_t*)codeRes, sizeof(codeRes));
        codeResCharacteristic->notify();
        lastTime = millis();
    }
    else 
    {
        BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
        pAdvertising->start(); 
    }
}

void readButtonStatusCharacteristic() {
    if (deviceConnected) {
        std::string value = buttonStatusCharacteristic->getValue();
        if (value.length() >= 15) {
            for (int i = 0; i < 15; i++) {
                buttonStatus[i] = value[i] != 0; // Convert byte to boolean
            }
            Serial.println("Received button status data:");
            for (int i = 0; i < 15; i++) {
                Serial.print(codeRes[i]);
                Serial.print(" ");
            }
            Serial.println();
        }
    }
}

// ### handle wifi server


void handleRoot() {
    server.send(200, "text/html", statusPage);
}

void handleCommandPage() {
    server.send(200, "text/html", commandPage);
}

void handlePostCodes() {
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, server.arg("plain"));

    if (error) {
        Serial.println("Failed to parse JSON");
        server.send(400, "text/plain", "Invalid JSON");
        return;
    }

    JsonArray codeResArray = doc["codeRes"].as<JsonArray>();
    JsonArray encodedAArray = doc["codeA"].as<JsonArray>();
    JsonArray encodedBArray = doc["codeB"].as<JsonArray>();

    for (int i = 0; i < 15; i++) {
        codeRes[i] = codeResArray[i];
        encodedCodeA[i] = encodedAArray[i];
        encodedCodeB[i] = encodedBArray[i];
    }

    Serial.println("Data received and updated!");
    server.send(200, "text/plain", "OK");
}

void handleGetCodeRes() {
    DynamicJsonDocument doc(512);

    JsonArray codeResArray = doc.createNestedArray("codeRes");
    for (int i = 0; i < 15; i++) {
        codeResArray.add(codeRes[i]);
    }

    String response;
    serializeJson(doc, response);
    
    server.send(200, "application/json", response);
}

void handleGetButtonStatus() {
    readButtonStatusCharacteristic();

    DynamicJsonDocument doc(512);

    JsonArray buttonStatusArray = doc.createNestedArray("buttonStatus");
    for (int i = 0; i < 15; i++) {
        buttonStatusArray.add(buttonStatus[i]);
    }

    String response;
    serializeJson(doc, response);

    server.send(200, "application/json", response);
}

void handleGetEncodedCodes() {
    DynamicJsonDocument doc(1024);

    JsonArray encodedCodeAArray = doc.createNestedArray("encodedCodeA");
    JsonArray encodedCodeBArray = doc.createNestedArray("encodedCodeB");
    for (int i = 0; i < 15; i++) {
        encodedCodeAArray.add(encodedCodeA[i]);
        encodedCodeBArray.add(encodedCodeB[i]);
    }

    String response;
    serializeJson(doc, response);

    server.send(200, "application/json", response);
}

void handlePostGameMode(){
    DynamicJsonDocument doc(128);
    DeserializationError error = deserializeJson(doc, server.arg("plain"));

    if (error) {
        Serial.println("Failed to parse JSON");
        server.send(400, "text/plain", "Invalid JSON");
        return;
    }
    JsonString gameModeString = doc["mode"].as<JsonString>();

    gameMode = gameModeString.c_str();

    Serial.println("Data received and updated!");
    server.send(200, "text/plain", "OK");
}


// ##### Update Game State #####
void updateGameState() {
	unsigned long currentTime = millis();
	switch (currentState) {
		case CODE_A:
			displayCode(encodedCodeA);
			if (currentTime - stateStartTime > displayDuration) {
				currentState = WAIT;
				stateStartTime = currentTime;
			}
			break;
		case WAIT:
			if (currentTime - stateStartTime > waitDuration) {
				currentState = CODE_B;
				stateStartTime = currentTime;
			}
			break;
		case CODE_B:
			displayCode(encodedCodeB);
			if (currentTime - stateStartTime > displayDuration) {
				currentState = CODE_RES;
				stateStartTime = currentTime;
			}
			break;
		case CODE_RES:
			displayCode(codeRes);
			// Une fois dans cet état, on reste ici et on envoie le signal d'ouverture
			break;
	}
}

