#include <Arduino.h>
#include <WiFi.h>

// ##### pins ####
/*
// ------- input------
const uint8_t IN_A = 2;
const uint8_t IN_B = 3;
const uint8_t IN_C = 4;
const uint8_t IN_D = 5;
*/
const uint8_t IN_CLOSED = 4;

// ------output-----
const  uint8_t OUTPUTS[15] = {12,13,16,17,18,19,19,21,22,23,25,26,27,32,33};

// #### declarations ####

// ------functions------
void display_figure();
void clear_display();
void getDataFromWifi(WiFiClient _client);
void displayWebPage(WiFiClient client);

// --------values----------
// _____wifi & query____
const char* ssid = "la_boit_à_ampoules";
const char* password = "le_mot_de_passe";
WiFiServer server(80);
String header;
bool outputStates[15];

// _______time______
// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

// #### main code ####

// main 
void setup() {
  Serial.begin(115200);
  pinMode(IN_CLOSED,INPUT);

  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
}

void loop() {
  WiFiClient client = server.available();
  
  if (client) {                             // If a new client connects,
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client.");
    getDataFromWifi(client);
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }

  if (digitalRead(IN_CLOSED))
    display_figure();
  else
    clear_display();

}

// functions
void display_figure()
{
  for (int i; i < 15; i++)
  {
    if (outputStates[i])
    {
      digitalWrite(OUTPUTS[i],HIGH);
    }
    else if (!outputStates[i])
    {
      digitalWrite(OUTPUTS[i],LOW);
    }
  }
}

void clear_display()
{
  for (int i; i < 15; i++)
  {
    digitalWrite(OUTPUTS[i], LOW);
  }
}

void getDataFromWifi(WiFiClient client)
{
  String currentLine = "";                // make a String to hold incoming data from the client
  while (client.connected() && currentTime - previousTime <= timeoutTime) {  // loop while the client's connected
      currentTime = millis();
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:

            
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            /*
            // turns the GPIOs on and off
            if (header.indexOf("GET /26/on") >= 0) {
              Serial.println("GPIO 26 on");
              output26State = "on";
              digitalWrite(output26, HIGH);
            } else if (header.indexOf("GET /26/off") >= 0) {
              Serial.println("GPIO 26 off");
              output26State = "off";
              digitalWrite(output26, LOW);
            } else if (header.indexOf("GET /27/on") >= 0) {
              Serial.println("GPIO 27 on");
              output27State = "on";
              digitalWrite(output27, HIGH);
            } else if (header.indexOf("GET /27/off") >= 0) {
              Serial.println("GPIO 27 off");
              output27State = "off";
              digitalWrite(output27, LOW);
            }
            
            
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons 
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #555555;}</style></head>");
            
            // Web Page Heading
            client.println("<body><h1>ESP32 Web Server</h1>");
            
            // Display current state, and ON/OFF buttons for GPIO 26  
            client.println("<p>GPIO 26 - State " + output26State + "</p>");
            // If the output26State is off, it displays the ON button       
            if (output26State=="off") {
              client.println("<p><a href=\"/26/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/26/off\"><button class=\"button button2\">OFF</button></a></p>");
            } 
            
            // Display current state, and ON/OFF buttons for GPIO 27  
            client.println("<p>GPIO 27 - State " + output27State + "</p>");
            // If the output27State is off, it displays the ON button       
            if (output27State=="off") {
              client.println("<p><a href=\"/27/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/27/off\"><button class=\"button button2\">OFF</button></a></p>");
            }
            client.println("</body></html>");

            */
            
            // get and set the figure variable
            for (int i = 0; i < 15; i++)
            {
              if (header.indexOf("GET /" + String(i) + "/on"))
              {
                Serial.println("lampe " + String(i) + " must be on");
                outputStates[i] = true;
              }
              else if (header.indexOf("GET /" + String(i) + "/off"))
              {
                Serial.println("lampe " + String(i) + " must be off");
                outputStates[i] = false;
              }
            } 

            //display the web page
            displayWebPage(client);

            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
  header = "";
}
void displayWebPage(WiFiClient client)
{
  // ## head ##
  client.println("<!DOCTYPE html><html lang=\"fr\">");
  client.println("<head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">");
  client.println("<title>Le jeu des ampoules commande</title>");
  // __style css__
  client.println("<style> body { display: flex; flex-direction: column; justify-content: center; align-items: center; height: 100vh; margin: 0; font-family: Arial, sans-serif; }");
  client.println("h1 { font-size: 2em; color: #333; margin: 10px 0;}");
  client.println("p { font-size: 1.2em; color: #555; text-align: center; margin: 0 0 20px 0; }");
  client.println(".grid { display: grid; grid-template-columns: repeat(3, 100px); grid-template-rows: repeat(5, 100px); gap: 10px; }");
  client.println("a { text-decoration: none; }");
  client.println("button { width: 100px; height: 100px; color: white; border: none; cursor: pointer; font-size: 16px; transition: 0.3s; }");
  client.println("button:hover { background: #DAA520 !important; }</style></head>");
  // __body page__
  client.println("<body><h1>Le jeu des ampoules commande</h1>");
  client.println("<p>Cliquez sur les boutons pour allumer les lampes</p>");
  // button grid
  client.println("<div class=\"grid\">");
  for (int i = 0; i < 15; i++)
  {
    if(outputStates[i] == true)
    {
      client.println("<a href=\"/" + String(i) + "/off\"><button style=\"background: #FFD700;\">L" + String(i) + "</button></a>");
    }
    else if (outputStates[i] == false)
    {
      client.println("<a href=\"/" + String(i) + "/on\"><button style=\"background: #555555;\">L" + String(i) + "</button></a>");
    }
  }
  client.println("</div>");
  client.println("</body></html>");
}
/*
uint8_t get_nb_choice()
{

  bool a = digitalRead(IN_A);
  bool b = digitalRead(IN_B);
  bool c = digitalRead(IN_C);
  bool d = digitalRead(IN_D);

  if (!a && !b && !c && !d)
    return 0;

  else if (!a && !b && !c && d)
    return 1;

  else if (!a && !b && c && !d)
    return 2;

  else if (!a && !b && c && d)
    return 3;

  else if (!a && b && !c && !d)
    return 4;

  else if (!a && b && !c && d)
    return 5;

  else if (!a &&  b && c && !d)
    return 6;

  else  if (!a && b && c && d)
    return 7;

  else  if (a && !b && !c && !d)
    return 8;

  else  if (a && !b && !c && d)
    return 9;

  return 0;
}
*/