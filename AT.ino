#include <ESP8266WiFi.h>

#ifdef ESP8266
extern "C" {
  #include "user_interface.h"
}
#endif

String incoming = "";
bool requestSent = false;
bool sendMode = false;
WiFiClient client;

void setup() {
  Serial.begin(115200);
}

void loop() 
{
//  while (client.available() && requestSent) //this all needs a timeout, and better handling of line endings
//  {
//    String line = client.readStringUntil('\r');
//    Serial.println(line);
//  }

  if(requestSent)
  {
    String line = "";
    uint32_t lastRead = millis();
    
    while(millis() - lastRead < 2000ul)  //2 second timeout 
    {
      if(client.available())
      {
        char ch = client.read();
        line += ch;
        if(ch == '\n')
        {
          Serial.print(line);
          line = "";
        }
        lastRead = millis();
      }
    }
  }
  
  if (Serial.available() > 0) 
  {
    if (sendMode) 
    {
      //incoming = Serial.readStringUntil('+'); //can't use this here -- we need to send lines as they arrive
      char ch = Serial.read();
      incoming += ch;
      if(ch == '\n')
      {
        client.print(incoming);
        //Serial.println(incoming); //are we just echoing back here?
        incoming = "";
      }
      if(incoming == "+++") //these should be in a packet by themselves
      {
        requestSent = true;
        sendMode = false;
        incoming = "";    
      }
    }

    else
      {
      incoming = Serial.readStringUntil('\n');
      if (incoming == "AT\r") //Attention
        Serial.print("\r\nOK\r\n");
      else if (incoming == "AT+RST\r") // Reset
      {
        Serial.print("\r\nOK\r\n");
      } 
      else if (incoming == "ATE0\r") // echo off, ignore
      {
        Serial.print("\r\nOK\r\n");
      } 
      else if (incoming == "ATE1\r") // echo on, ignore [TODO: echo back commands]
      {
        Serial.print("\r\nOK\r\n");
      } 
      else if (incoming == "AT+CWMODE?\r") // checking wifi mode
      {
        Serial.print("\r\n+CWMODE:1\r\n\r\nOK\r\n"); //always return station mode
      } 
      else if (incoming.startsWith("AT+CWMODE=")) // setting wifi mode, ignore (always station)
      {
        Serial.print("\r\nOK\r\n");
      } 
      else if (incoming.startsWith("AT+CIPMUX")) // setting the mux, ignore (we'll do whatever we want)
      {
        Serial.print("\r\nOK\r\n");
      } 
      else if (incoming.startsWith("AT+CIPMODE")) // setting transparent mode, ignore
      {
        Serial.print("\r\nOK\r\n");
      } 
      else if (incoming == "AT+CIPSTAMAC?\r") // getting MAC address, TODO: return MAC address
      {
        Serial.print("\r\nOK\r\n");
      } 
      else if (incoming == "AT+CIFSR\r") // getting IP and MAC address, TODO: return IP and MAC address
      //sample:
      //+CIFSR:STAIP,"192.168.0.11"
      //+CIFSR:STAMAC,"dc:4f:22:26:7e:ed"
      {
        Serial.print("\r\nOK\r\n");
      } 
  
      else if (incoming.substring(0, 9) == "AT+CWJAP?") //TODO: handle CWJAP? request
      //sample:  
      //+CWJAP:"MOTOROLA-4A7AC","00:26:82:ce:7a:02",1,-61\r\n
      {}
      else if (incoming.substring(0, 9) == "AT+CWJAP=") //Join Network
      {
        String ssid = split(incoming, '\"', 1);
        int ssidL = ssid.length() + 1;
        char ssidArray[ssidL];
        ssid.toCharArray(ssidArray, ssidL);
  
        String password = split(incoming, '\"', 3);
        int passwordL = password.length() + 1;
        char passwordArray[passwordL];
        password.toCharArray(passwordArray, passwordL);
  
        WiFi.begin(ssidArray, passwordArray);
  
        while (WiFi.status() != WL_CONNECTED) {
          delay(100);
        }
        Serial.print("\r\nOK\r\n");
      } else if (incoming.substring(0, 11) == "AT+CIPSTART") {
        requestSent = false;
        String addr = split(incoming, '\"', 3);
        if (!client.connect(addr, 80)) {
          Serial.println("connection failed");
        } else {
          Serial.print("\r\nCONNECT\r\n");
          Serial.print("\r\nOK\r\n");
        }
      } 
      else if (incoming.substring(0, 12) == "AT+CIPSTATUS") 
      /*
       * CIPSTATUS is a tricky one. With the AT firmware, the return value is both a function of the WiFi state
       * _and_ the client state:
       * ESP8266_STATUS_GOTIP = 2,         //wifi connected; IP assigned 
       * ESP8266_STATUS_CONNECTED = 3,     //connected to a server
       * ESP8266_STATUS_DISCONNECTED = 4,  //disconnected from a server
       * ESP8266_STATUS_NOWIFI = 5         //no wifi cxn
       * 
       * It then goes on to list info about the server cxn, but we can probably ignore that.
       * 
       * sample:
       * STATUS:3
       * +CIPSTATUS:0,"TCP","34.209.142.24",80,16119,0
       * 
       * or:
       * STATUS:4
       */
      {
        if(WiFi.status() != WL_CONNECTED) Serial.print("\r\nSTATUS:5\r\n\r\nOK\r\n");
        else if(client.status() == CLOSED) Serial.print("\r\nSTATUS:4\r\n\r\nOK\r\n");
        //else if(client.status() == ESTABLISHED) Serial.print("\r\nSTATUS:3\r\n\r\nOK\r\n");
        else Serial.print("\r\nSTATUS:3\r\n\r\nOK\r\n"); //not sure about intermediate states, but we'll default to 3
      } 
      else if (incoming.substring(0, 10) == "AT+CIPSEND") 
      {
        sendMode = true;
      } 
      else if (incoming.substring(0,11) == "AT+CIPCLOSE")
      {
        client.stop();
        Serial.print("\r\n\r\nOK\r\n");
      }
      incoming = "";
    }
  }
}

String split(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = {
    0,
    -1
  };
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}
