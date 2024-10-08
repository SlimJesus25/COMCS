#include <WiFi.h>
#include <WiFiUdp.h>

#define LED_BUILTIN 2

// IoTDEI2
// #8tud3nt2024
const char * ssid = "Vodafone 2.4GHz"; // the SSID of the network
const char * pwd = "vK6427dauN"; // the password of the network

// 192.168.8.208
const char * udpAddress = "192.168.1.154"; //the address of the SRV01
const int udpPort = 9999;

WiFiUDP udp;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  pinMode(LED_BUILTIN, OUTPUT);

  WiFi.begin(ssid, pwd);

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  
  //data will be sent to server
  uint8_t buffer[50] = "hello world";
  Serial.println("initializing udp and transfer buffer...");
  Serial.print("IP address: ");
  Serial.println(udpAddress);
  Serial.print("Port: ");
  Serial.println(udpPort);

  //This initializes udp and transfer buffer
  udp.beginPacket(udpAddress, udpPort);
  udp.write(buffer, 11);
  udp.endPacket();
  memset(buffer, 0, 50);

  //processing incoming packet, must be called before reading the buffer
  udp.parsePacket();

  //receive response from server, it will be ....
  if(udp.read(buffer, 50) > 0){
    Serial.print("Server to client: ");
    Serial.println((char *)buffer);
    if(strcmp((char*)buffer, "LightOn") == 0)
      digitalWrite(LED_BUILTIN, HIGH);
    else
      digitalWrite(LED_BUILTIN, LOW);
  }


  //Wait for 5 second
  delay(1000);
}

