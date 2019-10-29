//************************************************************
// this is a simple example that uses the painlessMesh library to 
// setup a node that logs to a central logging node
// The logServer example shows how to configure the central logging nodes
//************************************************************
#include "painlessMesh.h"
#include "DHT.h"


#define DHTPIN 14
#define DHTTYPE DHT11


#define   MESH_PREFIX     "project"
#define   MESH_PASSWORD   "1234"
#define   MESH_PORT       5555

DHT dht(DHTPIN, DHTTYPE);

Scheduler     userScheduler; // to control your personal task
painlessMesh  mesh;

// Prototype
void receivedCallback( uint32_t from, String &msg );

size_t logServerId = 1;

// Send message to the logServer every 10 seconds 
Task myLoggingTask(5000, TASK_FOREVER, []() {

    DynamicJsonDocument jsonBuffer(1024);
    JsonObject msg = jsonBuffer.to<JsonObject>();

    float h = dht.readHumidity();
    float t = dht.readTemperature();
    
    msg["nodename"] = "mcu-t2";
    msg["sensor_ID"] = mesh.getNodeId();
    msg["Temperature:"] = float(t);
    msg["Humidity:"] = float(h);

    String str;
    serializeJson(msg, str);
    if (logServerId == 0) // If we don't know the logServer yet
        mesh.sendBroadcast(str);
    else
        mesh.sendSingle(logServerId, str);
    serializeJson(msg, Serial);
    Serial.printf("\n");
});

void setup() {
  Serial.begin(115200);
  Serial.println("Begin DHT11 Mesh Network test!");

  dht.begin();

    
  mesh.setDebugMsgTypes( ERROR | STARTUP | CONNECTION );  // set before init() so that you can see startup messages

  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT, WIFI_AP_STA, 6 );
  mesh.onReceive(&receivedCallback);

  // Add the task to the your scheduler
  userScheduler.addTask(myLoggingTask);
  myLoggingTask.enable();
}

void loop() {
  // it will run the user scheduler as well
  mesh.update();
}

void receivedCallback( uint32_t from, String &msg ) {
  Serial.printf("logClient: Received from %u msg=%s\n", from, msg.c_str());

  // Saving logServer
#if ARDUINOJSON_VERSION_MAJOR==6
  DynamicJsonDocument jsonBuffer(1024 + msg.length());
  DeserializationError error = deserializeJson(jsonBuffer, msg);
  if (error) {
    Serial.printf("DeserializationError\n");
    return;
  }
  JsonObject root = jsonBuffer.as<JsonObject>();
#else
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(msg);
#endif
  if (root.containsKey("topic")) {
      if (String("logServer").equals(root["topic"].as<String>())) {
          // check for on: true or false
          logServerId = root["sensor_Id"];
          Serial.printf("logServer detected!!!\n");
      }
      Serial.printf("Handled from %u msg=%s\n", from, msg.c_str());
  }
}
