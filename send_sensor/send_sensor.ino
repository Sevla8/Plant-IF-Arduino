#include <StreamUtils.h>

#include <MemoryUsage.h>

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

/*
 * How to change the delay from the server :
 * curl http://192.168.254.118 -H 'Content-Type: application/json' -d '{"type":"delay","delay":10000}'
 * 
 */

char ssid[] = "HUAWEI P20 lite"; // "Pixel_6968"; //  your network SSID (name)

char pass[] = "motdepasse"; // "e52d936d98bf";    // your network password (use for WPA, or use as key for WEP)

int keyIndex = 0;            // your network key Index number (needed only for WEP)

int status = WL_IDLE_STATUS;

// if you don't want to use DNS (and reduce your sketch size)
// use the numeric IP instead of the name for the server:
IPAddress serverAddr(192,168,43,200);

// Initialize the Ethernet client library
// with the IP address and port of the server
// that you want to connect to (port 80 is default for HTTP):
WiFiClient client;

WiFiServer server(80);

WiFiClient clientServer;

const String LOC_SEND_DATA = "/SMARTweb/ActionServlet";

const String ID = "TEST01";

const int NB_MIN_SENSORS = 12;
const int NB_MAX_SENSORS = 15;

long captureDelay = 500; // 10 sec

void sendJson(WiFiClient& client, JsonDocument& doc) {
  String json;
  serializeJson(doc, json);
  client.println("Host: Arduino");
  client.print("Content-Length: ");
  client.println(json.length());
  client.println("Content-Type: application/json");
  client.println();
  client.println(json);
}

void getJson(WiFiClient& client, JsonDocument& doc) {
  char rc1 = '.';
  char rc2 = '.';
  while (client.available()) {
    char c = client.read();
//    Serial.write(c);
    if (c == '\r') {
      continue;
    }
    rc1 = c;
    if (rc1 == '\n' && rc2 == '\n') {
      break;
    }
    rc2 = rc1;
  }
  deserializeJson(doc, client);
}

DynamicJsonDocument sensorDataDoc(2048);
int replIndex;

void initTable() {
  sensorDataDoc.clear();
  sensorDataDoc["todo"] = "newRecord";
  sensorDataDoc["arduinoNumber"] = ID;
  sensorDataDoc.createNestedArray("data");
  replIndex = 0;
}

// RetreiveSensorData
class GetSensorData : public VariableTimedAction {
private:
  unsigned long run() {
    
      // Request the current time
      WiFiClient timeClient;
      timeClient.connect("worldtimeapi.org", 80);
      timeClient.print("GET ");
      timeClient.print("/api/timezone/Etc/UTC");
      timeClient.println(" HTTP/1.1");
      timeClient.println("Host: Arduino");
      timeClient.println();
      StaticJsonDocument<512> timeDoc;
      int wait = 0;
      while (!timeClient.available() && wait<9000) {
        delay(1);
        ++wait;
      }
      getJson(timeClient, timeDoc);
      timeClient.stop();

      JsonObject obj;
      if (sensorDataDoc["data"].size()>=NB_MAX_SENSORS) {
        obj = sensorDataDoc["data"][replIndex++];
        replIndex %= sensorDataDoc["data"].size();
      }
      else {
        obj = sensorDataDoc["data"].createNestedObject();
      }
      // Get the value of the light sensor.
      obj["light"] = analogRead(A0);

      // Get the value of the temperature sensor.
      int value = analogRead(A1);
      // Determine the current resistence of the thermistor based
      // on the sensor value.
      float resistance = (float) (1023 - value) * 10000 / value;
      // Calculare the temperature based on the resistence value.
      obj["temperature"] = 1 / (log(resistance / 10000) / 3975 + 1 / 298.15) - 273.15;

      // Get the value of the moisture sensor.
      obj["moisture"] = analogRead(A2);

      // Get the time
      obj["time"] = timeDoc["unixtime"];
    // if you get a connection, report back via serial:
    Serial.println(sensorDataDoc.memoryUsage());
    if (sensorDataDoc["data"].size()>=NB_MAX_SENSORS) {
      if (client.connect(serverAddr, 8080)) {
        Serial.print("Sending data... ");
  
        // Make a HTTP request:
        client.print("POST ");
        client.print(LOC_SEND_DATA);
        client.println(" HTTP/1.1");

        sendJson(client, sensorDataDoc);
        Serial.println("Data sent.");
        initTable();
      }
    }

//    //return code of 0 indicates no change to the interval
//    //if the interval must be changed, then return the new interval
    return captureDelay;
  }
};

GetSensorData sensor;

// Wait for client instructions
class ServerListening : public VariableTimedAction {
private:
  unsigned long run() {
    // listen for incoming clients
    clientServer = server.available();
    if (clientServer) {
      if (clientServer.connected()) {
        Serial.println("Connected to client");

        DynamicJsonDocument doc(256);
        getJson(clientServer, doc);

//        Serial.print("doc['type'] = ");
//        Serial.println((char*) doc["type"]);
        if (doc["type"] == "delay") {
          captureDelay = doc["delay"];
//          Serial.print("doc['delay'] = ");
//          Serial.println((long) doc["delay"]);
            sensor.stop();
            sensor.start(captureDelay);
            clientServer.println("HTTP/1.1 200 OK");
            clientServer.println("");
        }
        else if (doc["type"] == "get") {
            
            Serial.print("Sending data... ");
      
            // Make a HTTP request:
            clientServer.println("HTTP/1.1 200 OK");
        
            sendJson(clientServer, sensorDataDoc);
            Serial.println("Data sent.");
            initTable();
        }
        
        // close the connection:
        clientServer.stop();
      }
    }
    
    //return code of 0 indicates no change to the interval
    //if the interval must be changed, then return the new interval
    return 0;
  }
};

ServerListening serverListening;

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

  // Attempt to connect to Wifi network:
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

  server.begin();
  
  initTable();
  serverListening.start(1000);
  sensor.start(captureDelay);
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
