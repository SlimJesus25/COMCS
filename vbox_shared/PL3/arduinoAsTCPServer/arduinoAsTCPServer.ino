#include <algorithm> 
#include <cctype>
#include <locale>
#include <WiFi.h>
using namespace std;

#define LED_BUILTIN 2

WiFiServer server(8080);

const char * ssid = "Vodafone 2.4GHz"; // the SSID of the network
const char * pwd = "vK6427dauN"; // the password of the network

String rtrim(char* str, int size);

void setup() {
  Serial.begin(9600);

  pinMode(LED_BUILTIN, OUTPUT);

  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pwd);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP()); //prints the ip to which the server is listening
  server.begin();
}

void loop(){
  WiFiClient client = server.available();

  if (client) {
    while (client.connected()) {
      while (client.available() > 0) {
        String request = client.readString();
        const char* requestCStr = request.c_str();
        // request = rtrim(request.c_str(), request.length());

        // request.compareTo("on")
        if(strncmp(requestCStr, "on", 2) == 0)
          digitalWrite(LED_BUILTIN, HIGH);
        else if(strncmp(requestCStr, "off", 3) == 0) // request.compareTo("off")
          digitalWrite(LED_BUILTIN, LOW);
        sendResponse(&client, "Ok!");
      }
    }
    client.stop();
    Serial.println("Client disconnected");
  }
}

void sendResponse(WiFiClient* client, String response){
  client->print(response);
    
  // Debug print.
  Serial.println("sendResponse:");
  Serial.println(response);
}

String trim(char* str, int size){
  int i = size-1;
  while(*(str+i) == ' '){
    i--;
  }
  char* trimmed = new char[i + 2];
  strncpy(trimmed, str, i + 1);
  trimmed[i + 1] = '\0';
  return trimmed;
}