#include <VariableTimedAction.h>


#include <SPI.h>
#include <WiFiNINA.h>
#include <ArduinoJson.h>

/*
  Web client
  This sketch connects to a website (http://www.google.com)
  using a WiFi shield.
  
  This example is written for a network using WPA encry
  by dlf (Metodo2 srl)ption. For
  WEP or WPA, change the Wifi.begin() call accordingly.

  Circuit:
  * WiFi shield attached
  created 13 July 2010
  modified 31 May 2012
  by Tom Igoe
*/

char ssid[] = "HUAWEI P20 lite"; //  your network SSID (name)

char pass[] = "motdepasse";    // your network password (use for WPA, or use as key for WEP)

int keyIndex = 0;            // your network key Index number (needed only for WEP)

int status = WL_IDLE_STATUS;

// if you don't want to use DNS (and reduce your sketch size)
// use the numeric IP instead of the name for the server:
IPAddress serverAddr(192,168,43,216);

// Initialize the Ethernet client library
// with the IP address and port of the server
// that you want to connect to (port 80 is default for HTTP):
WiFiClient client;

void sendJson(WiFiClient& client, JsonDocument& doc) {
  String json;
  serializeJson(doc, json);
  // Make a HTTP request:
  client.println("PUT /root.json HTTP/1.1");
  client.println("Host: Arduino");
  client.print("Content-Length: ");
  client.println(json.length());
  client.println();
  client.println(json);
}

//Wait for client instructions
class ServerListening : public VariableTimedAction {
private:
  unsigned long run() {
    
    //return code of 0 indicates no change to the interval
    //if the interval must be changed, then return the new interval
    return 0;
  }
};
//RetreiveSensorData
class GetSensorData : public VariableTimedAction {
private:
  unsigned long run() {
    // if you get a connection, report back via serial:
    if (client.connect(serverAddr, 3000)) {
      Serial.println("connected to server");
      StaticJsonDocument<200> doc;
      doc["light"] = analogRead(A0);
      doc["temperature"] = analogRead(A1);
      doc["moisture"] = analogRead(A2);
      sendJson(client, doc);
    }
    
    // if there are incoming bytes available
    // from the server, read them and print them:
    while (client.available()) {
      char c = client.read();
      Serial.write(c);
    }
  
    // if the server's disconnected, stop the client:
    if (!client.connected()) {
      Serial.println();
      Serial.println("disconnecting from server.");
      client.stop();
  
      // do nothing forevermore:
      while (true);
    }
    //return code of 0 indicates no change to the interval
    //if the interval must be changed, then return the new interval
    return 0;
  }
};

ServerListening server;
GetSensorData sensor;

void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.println("Starting Program");
  
  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("WiFi shield not present");
    // don't continue:
    while (true);
  }
  Serial.println("WiFi check OK");

  String fv = WiFi.firmwareVersion();
  if (fv != "1.1.0") {
    Serial.println("Please upgrade the firmware");
  }

  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);
    // wait 10 seconds for connection:
    delay(10000);
  }
  Serial.println("Connected to wifi");

  printWifiStatus();

  Serial.println("\nStarting connection to server...");

  server.start(1000);
  sensor.start(5*60*1000);
}

void loop() {
  VariableTimedAction::updateActions();
}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}
