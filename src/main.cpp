#include <Arduino.h>
#include <WiFi.h>

// ##### pins ####

const uint8_t IN_CLOSED = 4;

// ------output-----
const  uint8_t OUTPUTS[15] = {12,13,16,17,18,19,19,21,22,23,25,26,27,32,33};

// #### declarations ####

// ------functions------
void getDataFromWifi(WiFiClient _client);
void displayWebPage(WiFiClient client);

// --------values----------
// _____wifi & query____
const char* ssid = "La_Boite_à_Ampoules";
const char* password = "lejeulalejeu";
WiFiServer server(80);
String header;
String outputStates[15];

// #### main code ####

// main 
void setup() {
  Serial.begin(115200);
  pinMode(IN_CLOSED,INPUT);
  for (int i = 0; i < 15; i++) {
    pinMode(OUTPUTS[i], OUTPUT);
    outputStates[i] = "off";
  }

  Serial.print("Setting AP");
  WiFi.softAP(ssid, password);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  server.begin();
}

void loop() {
  WiFiClient client = server.available();
  
  if (client)
  {
    Serial.println("New Client");
    String currentLine = "";
    
    while (client.connected())
    {
      if (client.available())
      {
        char c = client.read();
        Serial.write(c);
        header += c;
        if (c == '\n')
        {
          if  (currentLine.length() == 0)
          {
            // http headers start
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            // display figure
            for (int i = 0; i < 15; i++)
            {
              if (header.indexOf("GET /" + String(i) + "/on") >= 0)
              {
                Serial.println("lampe " + String(i) + " on");
                outputStates[i] = "on";
                digitalWrite(OUTPUTS[i],HIGH);
              }
              else if (header.indexOf("GET /" + String(i) + "/off") >= 0)
              {
                Serial.println("lampe " + String(i) + " off");
                outputStates[i] = "off";
                digitalWrite(OUTPUTS[i],LOW);
              }
            }
              // display the web page
              displayWebPage(client);

              // end of the http reponse
              client.println();
              break;
          }
          else 
          {
            currentLine = "";
          }
        }
        else if (c != '\r')
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
  client.println("button { width: 100px; height: 100px; color: white; border: none; border-radius: 40px; cursor: pointer; font-size: 16px; transition: 0.3s; }");
  client.println("button:hover { background: #DAA520 !important; }</style></head>");
  // __body page__
  client.println("<body><h1>Le jeu des ampoules</h1>");
  client.println("<p>Cliquez sur les boutons pour allumer les lampes</p>");
  // button grid
  client.println("<div class=\"grid\">");
  for (int i = 0; i < 15; i++)
  {
    if(outputStates[i] == "on")
    {
      client.println("<a href=\"/" + String(i) + "/off\"><button style=\"background: #FFD700;\">L" + String(i) + "</button></a>");
    }
    else if (outputStates[i] == "off")
    {
      client.println("<a href=\"/" + String(i) + "/on\"><button style=\"background: #555555;\">L" + String(i) + "</button></a>");
    }
  }
  client.println("</div>");
  client.println("</body></html>");
}