#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncWebSocket.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include <SPIFFS.h> // Include the SPIFFS library


const char* ssid = "0000000";
const char* password = "MyPassword1234";

constexpr int PIN_SDA = 1;
constexpr int PIN_SCL = 2;
const int MPU_ADDR = 0x68; // I2C address of the MPU-6050. If AD0 pin is set to HIGH, the I2C address will be 0x69.

float RateRoll{};    // gyro: rate of change in roll  (degrees/s)
float RatePitch{};   // gyro: rate of change in pitch (degrees/s)
float RateYaw{};     // gyro: rate of change in yaw   (degrees/s)
float AccX{};        // accelerometer in X (g's)
float AccY{};        // accelerometer in Y (g's)
float AccZ{};        // accelerometer in Z (g's)
float AngleRoll{};   // calculated roll based on accelerometers (degrees, centered at 0)
float AnglePitch{};  // calculated pitch based on accelerometers (degrees, centered at 0)
float AngleYaw{};    // integrated yaw, based on gyros
float Temperature{};
unsigned long lastTimeMeasureYaw = 0;



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
  jsonDocument["AngleYaw"] = AngleYaw;
  jsonDocument["Temperature"] = Temperature;

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

if (!SPIFFS.begin(true)) {
  Serial.println("An error has occurred while mounting SPIFFS");
  return;
}

server.on("/resetYaw", HTTP_GET, [](AsyncWebServerRequest *request){
  AngleYaw = 0.0; // Reset the AngleYaw
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
           RateRoll, RatePitch, RateYaw, 
           AccX, AccY, AccZ,
           AngleRoll, AnglePitch, AngleYaw, 
           Temperature);
  
  request->send(200, "text/html", buffer);
});
  server.begin();
}

void readTemperature()
{
  // Request temperature data
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x41);  // Address of TEMP_OUT_H
  Wire.endTransmission();
  Wire.requestFrom(MPU_ADDR, 2);  // Reading 2 bytes

  Temperature = (float) (int16_t(Wire.read() << 8 | Wire.read()))/ 340.0 + 36.53;
}


void readMPU()
{
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
}


void updateAbsAngles()
{
  AngleRoll=atan(AccY/sqrt(AccX*AccX+AccZ*AccZ))*1/(3.142/180);
  AnglePitch=-atan(AccX/sqrt(AccY*AccY+AccZ*AccZ))*1/(3.142/180);

  // Get the current time
  unsigned long currentTime = millis();
   // Calculate time difference (dt) between current and last loop iteration
  float dt = (float)(currentTime - lastTimeMeasureYaw) / 1000.0; // Convert to seconds
  lastTimeMeasureYaw = currentTime;
  // Integrate the gyroscope data to get yaw
  AngleYaw += RateYaw * dt;
}

void loop() {
  readTemperature();
  readMPU();
  updateAbsAngles();
  // Send the updated sensor values over WebSocket
  sendSensorValuesOverWebSocket();
  delay(50); // Update every n milliseconds
}