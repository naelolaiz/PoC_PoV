#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncWebSocket.h>
#include <Wire.h>


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
float LoopTimer{};


AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

//int16_t sensorValue = 0; // Initialize your sensor value here

void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    // New client connected
    Serial.printf("WebSocket client #%u connected.\n", client->id());
  } else if (type == WS_EVT_DISCONNECT) {
    // Client disconnected
    Serial.printf("WebSocket client #%u disconnected.\n", client->id());
  }
}

void sendSensorValueOverWebSocket() {
  String data = String(sensorValue); // Update this with your sensor reading
  ws.textAll(data); // Send the data to all connected clients
}


void setupSensor()
{
//  pinMode(13, OUTPUT);
//  digitalWrite(13, HIGH);

  Wire.setClock(400000); //// ???
  Wire.begin(PIN_SDA, PIN_SCL);
  delay(250);  ////// ??
  Wire.beginTransmission(MPU_ADDR); // Begins a transmission to the I2C slave (GY-521 board)
  Wire.write(0x6B); // PWR_MGMT_1 register
  Wire.write(0); // set to zero (wakes up the MPU-6050)
  Wire.endTransmission(true);
}

//Define a mutex to protect access to the sensor data
SemaphoreHandle_t sensorMutex;



// Define the task function for reading sensor data
void sensorTask(void *parameter) {

  // Acquire the mutex before updating sensor data
    if (xSemaphoreTake(sensorMutex, portMAX_DELAY) == pdTRUE) {
  while (true) {
    /*
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x3B);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU_ADDR, 7 * 2, true);

  // "Wire.read()<<8 | Wire.read();" means two registers are read and stored in the same variable
  accelerometer_x = Wire.read()<<8 | Wire.read(); // reading registers: 0x3B (ACCEL_XOUT_H) and 0x3C (ACCEL_XOUT_L)
  accelerometer_y = Wire.read()<<8 | Wire.read(); // reading registers: 0x3D (ACCEL_YOUT_H) and 0x3E (ACCEL_YOUT_L)
  accelerometer_z = Wire.read()<<8 | Wire.read(); // reading registers: 0x3F (ACCEL_ZOUT_H) and 0x40 (ACCEL_ZOUT_L)
  temperature = Wire.read()<<8 | Wire.read(); // reading registers: 0x41 (TEMP_OUT_H) and 0x42 (TEMP_OUT_L)
  gyro_x = Wire.read()<<8 | Wire.read(); // reading registers: 0x43 (GYRO_XOUT_H) and 0x44 (GYRO_XOUT_L)
  gyro_y = Wire.read()<<8 | Wire.read(); // reading registers: 0x45 (GYRO_YOUT_H) and 0x46 (GYRO_YOUT_L)
  gyro_z = Wire.read()<<8 | Wire.read(); // reading registers: 0x47 (GYRO_ZOUT_H) and 0x48 (GYRO_ZOUT_L)
  */


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
      // Release the mutex after updating sensor data
      xSemaphoreGive(sensorMutex);
      delay(100); // Update every 100 milliseconds
    }   
    
  }
}

void setup() {
  Serial.begin(115200);

  Serial.println("Hello world!");

  // Connect to Wi-Fi
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);

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
    html += "<p>Sensor Value: <span id='sensorValue'>" + String(sensorValue) + "</span></p>";
    html += "<script>";
    html += "const socket = new WebSocket('ws://' + window.location.hostname + '/ws');";
    html += "socket.onopen = function(event) {";
    html += "  console.log('WebSocket connection opened.');";
    html += "};";
    html += "socket.onmessage = function(event) {";
    html += "  const sensorValue = parseInt(event.data);";
    html += "  document.getElementById('sensorValue').textContent = sensorValue;";
    html += "};";
    html += "socket.onclose = function(event) {";
    html += "  console.log('WebSocket connection closed.');";
    html += "};";
    html += "</script>";
    html += "</body></html>";
    request->send(200, "text/html", html);
  });

  
  // Create a mutex for sensor data
  sensorMutex = xSemaphoreCreateMutex();

  server.begin();

  // Create a FreeRTOS task for sensor reading
  xTaskCreatePinnedToCore(sensorTask, "Sensor Task", 4096, NULL, 1, NULL, 0);

}

void loop() {
  // Simulate incrementing sensor values
  sensorValue++;
  sendSensorValueOverWebSocket(); // Send updated value over WebSocket
  delay(1000); // Adjust the delay as needed
}