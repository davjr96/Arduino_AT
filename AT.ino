#
include < ESP8266WiFi.h >

  #ifdef ESP8266
extern "C" {#
  include "user_interface.h"
}#
endif

String incoming = "";
bool requestSent = false;
bool sendMode = false;
WiFiClient client;
//test comment by GCL -- just making sure I have github configured correctly

void setup() {
  Serial.begin(115200);

}

void loop() {
  while (client.available() && requestSent) {
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }
  if (Serial.available() > 0) {
    if (sendMode) {
      incoming = Serial.readStringUntil('+');
      Serial.println(incoming);
      client.print(incoming);
      requestSent = true;
      sendMode = false;

    }
    incoming = Serial.readStringUntil('\n');
    if (incoming == "AT\r") //Attention
      Serial.println("\r\nOK\r\n");
    else if (incoming == "AT+RST\r") // Reset
    {
      Serial.println("\r\nOK\r\n");
    } else if (incoming.substring(0, 8) == "AT+CWJAP") //Join Network
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
      Serial.println("\r\nOK\r\n");
    } else if (incoming.substring(0, 11) == "AT+CIPSTART") {
      requestSent = false;
      String addr = split(incoming, '\"', 3);
      if (!client.connect(addr, 80)) {
        Serial.println("connection failed");
      } else {
        Serial.print("\r\nCONNECT\r\n");
        Serial.print("\r\nOK\r\n");
      }
    } else if (incoming.substring(0, 12) == "AT+CIPSTATUS") {

      Serial.print("Status:3\r\n");
      Serial.print("+CIPSTATUS:0,\"TCP\",");
      Serial.print("google.com\",80,0\r\nOK\r\n");
    } else if (incoming.substring(0, 10) == "AT+CIPSEND") {
      sendMode = true;

    } else if (incoming.substring(0,11) == "AT+CIPCLOSE"){
      client.stop();
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
