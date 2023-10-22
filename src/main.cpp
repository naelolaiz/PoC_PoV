#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncWebSocket.h>
#include <ArduinoJson.h>
#include <SPIFFS.h> // Include the SPIFFS library
#include "MPU6050.h"

#include "LedController.h"


#include "secrets.h"
// const char* ssid_ap = "my_SSID";
// const char* password_ap = "my_password";
// const char* ssid_sta = "my_SSID";
// const char* password_sta = "my_password";

constexpr static bool create_ap = true;

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.printf("WebSocket client #%u connected.\n", client->id());
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.printf("WebSocket client #%u disconnected.\n", client->id());
  }
}

MPU6050<4/*SDA*/, 5 /*SCL*/, 0x68 /*i2c address*/> mpu6050;

void sendSensorValuesOverWebSocket() {
  // Create a JSON object to hold the sensor values
  StaticJsonDocument<256> jsonDocument;
  jsonDocument["RateRoll"] = mpu6050.data().RateRoll;
  jsonDocument["RatePitch"] = mpu6050.data().RatePitch;
  jsonDocument["RateYaw"] = mpu6050.data().RateYaw;
  jsonDocument["AccX"] = mpu6050.data().AccX;
  jsonDocument["AccY"] = mpu6050.data().AccY;
  jsonDocument["AccZ"] = mpu6050.data().AccZ;
  jsonDocument["AngleRoll"] = mpu6050.data().AngleRoll;
  jsonDocument["AnglePitch"] = mpu6050.data().AnglePitch;
  jsonDocument["AngleYaw"] = mpu6050.data().AngleYaw;
  jsonDocument["Temperature"] = mpu6050.data().Temperature;

  // Serialize the JSON object to a string
  String data;
  serializeJson(jsonDocument, data);

  // Send the data to all connected clients over WebSocket
  ws.textAll(data);
}

LedController<144,13,GRB> ledController;
void setup() { 
  Serial.begin(115200);


// initialize MPU-6050
  mpu6050.setupSensor();
  ledController.setup();
  ledController.startTask();

  if(create_ap)
  {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid_ap, password_ap);
  }
  else // Connect to Wi-Fi
  {
    WiFi.mode(WIFI_STA);  // Set the Wi-Fi mode to Station (Client)
    WiFi.begin(ssid_sta, password_sta);  // Connect to an existing network using SSID and password
  }
  
  // Print the ESP32's IP address
  if(create_ap)
  {
      Serial.print("AP IP address: ");
      Serial.println(WiFi.softAPIP());
  }
  else
  {
    Serial.print("Connected! IP address: ");
    Serial.println(WiFi.localIP());
  }

  // Attach WebSocket server
  ws.onEvent(onWebSocketEvent);
  server.addHandler(&ws);

if (!SPIFFS.begin(true)) {
  Serial.println("An error has occurred while mounting SPIFFS");
  return;
}

server.on("/resetYaw", HTTP_GET, [](AsyncWebServerRequest *request){
  mpu6050.resetAngleYaw(); // Reset the AngleYaw
  request->send(200, "text/plain", "OK");
});

server.on("/js/three.min.js", HTTP_GET, [](AsyncWebServerRequest *request){
  request->send(SPIFFS, "/js/three.min.js", "application/javascript");
});

// Route to serve the HTML page
server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
  // Buffer to hold the HTML content
  char html[4096];
  
  // Open file and read the template
  File file = SPIFFS.open("/template.html", "r");
  if (!file) {
    Serial.println("File not found");
    return;
  }

  size_t bytesRead = file.readBytes(html, sizeof(html) - 1);
  file.close();
  html[bytesRead] = '\0'; // Null-terminate the read content

  // Replace placeholders with actual values
  char buffer[4096];
  snprintf(buffer, sizeof(buffer),
           html,
           mpu6050.data().RateRoll, mpu6050.data().RatePitch, mpu6050.data().RateYaw, 
           mpu6050.data().AccX, mpu6050.data().AccY, mpu6050.data().AccZ,
           mpu6050.data().AngleRoll, mpu6050.data().AnglePitch, mpu6050.data().AngleYaw, 
           mpu6050.data().Temperature);
  
  request->send(200, "text/html", buffer);
});
  server.begin();

}

void loop() 
{
  mpu6050.readAndUpdateValues();

  ledController.setInstantVelocity(mpu6050.data().RatePitch);
  
  // Send the updated sensor values over WebSocket
  sendSensorValuesOverWebSocket();
  delay(50); // Update every n milliseconds
}
