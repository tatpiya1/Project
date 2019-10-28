#include <ESP8266WiFi.h>
#include <MySQL_Connection.h>
#include <MySQL_Cursor.h>
#include "painlessMesh.h"

#define   MESH_PREFIX     "project"
#define   MESH_PASSWORD   "1234"
#define   MESH_PORT       5555


// Wireless Network & Authentication
WiFiClient client; 

// Change SSID and Wifi password
char* ssid     = "tatpiyaWIFI";
char* password = "12345678";

// MySQL Server

// Change IP, user, and password
IPAddress server_addr(192,168,43,98);
char user[] = "USER";
char dbPassword[] = "PASSWORD";

char query[128];
MySQL_Connection dbConnector((Client *)&client);

// Change MySimpleIoT to your database name and 'measured_data" to your table
const char INSERT_DATA[] = "INSERT INTO demo.measured_data VALUEs (NULL, %d, %f, %f, NULL)";
int sensor_id ;

painlessMesh  mesh;

void receivedCallback( uint32_t from, String &msg );

Task logServerTask(5000, TASK_FOREVER, []() {
    DynamicJsonDocument jsonBuffer(1024);
    JsonObject msg = jsonBuffer.to<JsonObject>();
    msg["topic"] = "logServer";
    msg["sensor_Id"] = mesh.getNodeId();

    String str;
    serializeJson(msg, str);
    mesh.sendBroadcast(str);

    serializeJson(msg, Serial);
    Serial.printf("\n");
});
  
void setup() {

  Serial.begin(115200);
  
  
    // Connect to a WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.print(ssid);
  Serial.println();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

    
  
  // Database Connection
  while (!dbConnector.connect(server_addr, 3306, user, dbPassword)) {
    Serial.print(".");
    delay(1000);
  }
   
  Serial.println("Database successfully connected.");  


  mesh.setDebugMsgTypes( ERROR | CONNECTION | S_TIME );  // set before init() so that you can see startup messages
  mesh.init( MESH_PREFIX, MESH_PASSWORD, MESH_PORT, WIFI_AP_STA, 6 );
  mesh.onReceive(&receivedCallback);
  

  mesh.onNewConnection([](size_t nodeId) {
    Serial.printf("New Connection %u\n", nodeId);
  });

  mesh.onDroppedConnection([](size_t nodeId) {
    Serial.printf("Dropped Connection %u\n", nodeId);
  });


}


void loop() {

  mesh.update();

  // Read humidity
  float h;

  // Read temperature as Celsius (the default)
  float t; 

  // Insert new data into DB
  sprintf(query, INSERT_DATA, sensor_id, t, h);
  //Serial.println("Inserting reading data into database.");
  Serial.println(query);

  MySQL_Cursor *cursor = new MySQL_Cursor(&dbConnector);
  cursor->execute(query);
  delete cursor;
  delay(3000000);

  
}

void receivedCallback( uint32_t from, String &msg ){
  Serial.printf("logServer: Received from %u msg=%s\n", from, msg.c_str());
}
