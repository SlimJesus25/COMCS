#include "BluetoothSerial.h"
#include <WiFi.h>

#define LED_BUILTIN 2 // Set the GPIO pin where you connected your test LED or comment this line out if your dev board has a built-in LED

BluetoothSerial SerialBT;
WiFiClient client;

const char* ip = "192.168.1.154";
const uint port = 9998;


void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);

  if(!SerialBT.begin())
    Serial.println("BT error");
  
  Serial.println("Bluetooth Started! Ready to pair..."); 

  WiFi.begin("Vodafone 2.4GHz", "vK6427dauN");

  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(1000);
  }
  Serial.print("\n\nConnected with IP ");
  Serial.print(WiFi.localIP());

  if(!client.connect(ip, port)){
    Serial.println("\nConnection not established!");
  }else{
    Serial.print("\n\nConnection established with server ");
    Serial.print(ip);
    Serial.print(":");
    Serial.println(port);
  }
}

void loop() {

  if (SerialBT.available()) {

    String incoming_value = SerialBT.readStringUntil('\n');

    incoming_value.trim();
    // const char* incoming_value = str.c_str();

    Serial.println(incoming_value);
    Serial.println(incoming_value.length());

      if (incoming_value.equalsIgnoreCase("off")){
        digitalWrite(LED_BUILTIN, LOW); //If value is 0 then LED turns OFF
        if(client.connected())
          client.println("The led has been turned off");
        else
          return;
      }else if(incoming_value.equalsIgnoreCase("on")){
        digitalWrite(LED_BUILTIN, HIGH); //If value is 1 then LED turns ON
        if(client.connected())
          client.println("The led has been turned on");
        else
          return;
      }else
        client.println(incoming_value);

    if(client.connected()){
      while(!client.available());
      String res = client.readStringUntil('\n');
      String res2 = client.readString();
      res.trim();
      Serial.print("Server > ");
      Serial.println(res);
      Serial.print("Server 2 > ");
      Serial.println(res2);
      SerialBT.println(res);
    }else
      SerialBT.println("No response from server");
  }
  delay(200);
}