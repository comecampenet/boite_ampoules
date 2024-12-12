#include <Arduino.h>
#include <WiFi.h>

// ##### pins ####

// pin to check if the box is closed
const uint8_t IN_CLOSED = 34;

// ------output-----
// pins for each lamp
const  uint8_t OUTPUTS[15] = {4, 13, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33, 5};

// #### declarations ####

// ------functions------
void displayWebPage(WiFiClient _client);

// --------values----------
// _____wifi & query___

// ssid and pwd
const char* ssid = "La_Boite_Ampoules";
const char* password = "lejeulalejeu";

// create the wifi server on port 80
WiFiServer server(80);
String header; // store incoming the http header
String outputStates[15]; //keep each lamp state 

// #### main code ####

// main 
void setup() {
  Serial.begin(115200);
  //set modes for each pins and initialize output states
  pinMode(IN_CLOSED,INPUT);

  for (int i = 0; i < 15; i++) {
    pinMode(OUTPUTS[i], OUTPUT);
    outputStates[i] = "off";
  }

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
                outputStates[i] = "on"; // save the status of the lamp
              }
              else if (header.indexOf("GET /" + String(i) + "/off") >= 0) // if the i button is asked to be off
              {
                Serial.println("lampe " + String(i) + "commanded to be off"); // log that it's off
                outputStates[i] = "off"; // save the status  of the lamp
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
      if (outputStates[i] == "on")
      {
        digitalWrite(OUTPUTS[i],LOW); // set on the pin for the lamp (inversion sue to the circuit)
        Serial.println("L" +  String(i) + " is on");
      }
      else if (outputStates[i] == "off")
      {
        digitalWrite(OUTPUTS[i],HIGH); // set off the pin for the lamp (inversion sue to the circuit)
      }
    }
  }
  else if (digitalRead(IN_CLOSED) == LOW)
  {
    for  (int i = 0; i < 15; i++)
    {
      digitalWrite(OUTPUTS[i],HIGH); // set off the pin for the lamp
    }
  }
  delay(500);
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
    if(outputStates[i] == "on")
    {
      _client.println("<a href=\"/" + String(i) + "/off\"><button style=\"background: #FFD700;\">L" + String(i) + "</button></a>");
    }
    else if (outputStates[i] == "off")
    {
      _client.println("<a href=\"/" + String(i) + "/on\"><button style=\"background: #555555;\">L" + String(i) + "</button></a>");
    }
  }
  _client.println("</div>");
  _client.println("</body></html>");
}