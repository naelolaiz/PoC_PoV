#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncWebSocket.h>
#include <Wire.h>
#include <ArduinoJson.h>

const char* ssid = "0000000";
const char* password = "MyPassword1234";

constexpr int PIN_SDA = 1;
constexpr int PIN_SCL = 2;
const int MPU_ADDR = 0x68; // I2C address of the MPU-6050. If AD0 pin is set to HIGH, the I2C address will be 0x69.

float RateRoll{};
float RatePitch{};
float RateYaw{};
float AccX{};
float AccY{};
float AccZ{};
float AngleRoll{};
float AnglePitch{};

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.printf("WebSocket client #%u connected.\n", client->id());
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.printf("WebSocket client #%u disconnected.\n", client->id());
  }
}

void sendSensorValuesOverWebSocket() {
  // Create a JSON object to hold the sensor values
  StaticJsonDocument<256> jsonDocument;
  jsonDocument["RateRoll"] = RateRoll;
  jsonDocument["RatePitch"] = RatePitch;
  jsonDocument["RateYaw"] = RateYaw;
  jsonDocument["AccX"] = AccX;
  jsonDocument["AccY"] = AccY;
  jsonDocument["AccZ"] = AccZ;
  jsonDocument["AngleRoll"] = AngleRoll;
  jsonDocument["AnglePitch"] = AnglePitch;

  // Serialize the JSON object to a string
  String data;
  serializeJson(jsonDocument, data);

  // Send the data to all connected clients over WebSocket
  ws.textAll(data);
}

void setupSensor()
{
  Wire.setClock(400000);
  Wire.begin(PIN_SDA, PIN_SCL);
  delay(250);
  Wire.beginTransmission(MPU_ADDR); // Begins a transmission to the I2C slave (GY-521 board)
  Wire.write(0x6B); // PWR_MGMT_1 register
  Wire.write(0); // set to zero (wakes up the MPU-6050)
  Wire.endTransmission(true);
}

void setup() {
  // Connect to Wi-Fi
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);

  Serial.begin(115200);

  // initialize MPU-6050
  setupSensor();

  // Print the ESP32's IP address
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  // Attach WebSocket server
  ws.onEvent(onWebSocketEvent);
  server.addHandler(&ws);

 // Route to serve the HTML page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    String html = "<html><body>";
    html += "<h1>ESP32 Web Server</h1>";
    html += "<p>Rate Roll: <span id='RateRoll'>" + String(RateRoll) + "</span></p>";
    html += "<p>Rate Pitch: <span id='RatePitch'>" + String(RatePitch) + "</span></p>";
    html += "<p>Rate Yaw: <span id='RateYaw'>" + String(RateYaw) + "</span></p>";
    html += "<p>Acc X: <span id='AccX'>" + String(AccX) + "</span></p>";
    html += "<p>Acc Y: <span id='AccY'>" + String(AccY) + "</span></p>";
    html += "<p>Acc Z: <span id='AccZ'>" + String(AccZ) + "</span></p>";
    html += "<p>Angle Roll: <span id='AngleRoll'>" + String(AngleRoll) + "</span></p>";
    html += "<p>Angle Pitch: <span id='AnglePitch'>" + String(AnglePitch) + "</span></p>";
    html += "<script>";
    html += "const socket = new WebSocket('ws://' + window.location.hostname + '/ws');";
    html += "socket.onopen = function(event) {";
    html += "  console.log('WebSocket connection opened.');";
    html += "};";
    html += "socket.onmessage = function(event) {";
    html += "  const data = JSON.parse(event.data);";
    html += "  document.getElementById('RateRoll').textContent = data.RateRoll;";
    html += "  document.getElementById('RatePitch').textContent = data.RatePitch;";
    html += "  document.getElementById('RateYaw').textContent = data.RateYaw;";
    html += "  document.getElementById('AccX').textContent = data.AccX;";
    html += "  document.getElementById('AccY').textContent = data.AccY;";
    html += "  document.getElementById('AccZ').textContent = data.AccZ;";
    html += "  document.getElementById('AngleRoll').textContent = data.AngleRoll;";
    html += "  document.getElementById('AnglePitch').textContent = data.AnglePitch;";
    html += "};";
    html += "socket.onclose = function(event) {";
    html += "  console.log('WebSocket connection closed.');";
    html += "};";
    html += "</script>";
    html += "</body></html>";
    request->send(200, "text/html", html);
  });

  server.begin();
}

void loop() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x1A);
  Wire.write(0x05);
  Wire.endTransmission();
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x1C);
  Wire.write(0x10);
  Wire.endTransmission();
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  Wire.endTransmission(); 
  Wire.requestFrom(MPU_ADDR,6);
  int16_t AccXLSB = Wire.read() << 8 | Wire.read();
  int16_t AccYLSB = Wire.read() << 8 | Wire.read();
  int16_t AccZLSB = Wire.read() << 8 | Wire.read();
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x1B); 
  Wire.write(0x8);
  Wire.endTransmission();                                                   
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x43);
  Wire.endTransmission();
  Wire.requestFrom(MPU_ADDR,6);
  int16_t GyroX=Wire.read()<<8 | Wire.read();
  int16_t GyroY=Wire.read()<<8 | Wire.read();
  int16_t GyroZ=Wire.read()<<8 | Wire.read();
  RateRoll=(float)GyroX/65.5;
  RatePitch=(float)GyroY/65.5;
  RateYaw=(float)GyroZ/65.5;
  AccX=(float)AccXLSB/4096;
  AccY=(float)AccYLSB/4096;
  AccZ=(float)AccZLSB/4096;
  AngleRoll=atan(AccY/sqrt(AccX*AccX+AccZ*AccZ))*1/(3.142/180);
  AnglePitch=-atan(AccX/sqrt(AccY*AccY+AccZ*AccZ))*1/(3.142/180);
  // Send the updated sensor values over WebSocket
  sendSensorValuesOverWebSocket();
  delay(50); // Update every n milliseconds
}