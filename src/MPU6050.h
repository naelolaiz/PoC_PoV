#include <Arduino.h>
#include <Wire.h>

template<int PIN_SDA, int PIN_SCL, int MPU_ADDR=0x68>
class MPU6050
{
public:
    struct MPU6050_DATA {
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
    };

private:
    unsigned long mLastTimeMeasureYaw = 0;

    MPU6050_DATA mData;

private:

    void configSensor()
    {
      Wire.beginTransmission(MPU_ADDR);
      Wire.write(0x1A); // config
      //Wire.write(0x05); // ~14ms latency - https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Register-Map1.pdf
      Wire.write(0x01); // ~ 2ms latency
      Wire.endTransmission();
      Wire.beginTransmission(MPU_ADDR);
      Wire.write(0x1C); // config accel
      Wire.write(0x10); // +/- 8g. no self tests
      Wire.endTransmission();
      Wire.beginTransmission(MPU_ADDR);
      Wire.write(0x1B);  // gyro config
      Wire.write(0x8);   // +/- 500 degrees/s. no self tests
      Wire.endTransmission();                                                   
    }
public:
 
    void setupSensor()
    {
      Wire.setClock(400000);
      Wire.begin(PIN_SDA, PIN_SCL);
      delay(250);
      Wire.beginTransmission(MPU_ADDR); // Begins a transmission to the I2C slave (GY-521 board)
      Wire.write(0x6B); // PWR_MGMT_1 register
      Wire.write(0); // set to zero (wakes up the MPU-6050)
      Wire.endTransmission(true);
      configSensor();
    }

    const MPU6050_DATA & data() const 
    {
      return mData;
    }
    void resetAngleYaw()
    {
      mData.AngleYaw = 0.f;
    }
private:
    void readMPU()
    {
      //configSensor();

      Wire.beginTransmission(MPU_ADDR);
      Wire.write(0x3B); // start reading accel values
      Wire.endTransmission(); 
      Wire.requestFrom(MPU_ADDR,6+2+6); // 6 reads for acc, 2 for temp, 6 for gyros
      int16_t AccXLSB = Wire.read() << 8 | Wire.read();
      int16_t AccYLSB = Wire.read() << 8 | Wire.read();
      int16_t AccZLSB = Wire.read() << 8 | Wire.read();

      mData.Temperature = (float) (int16_t(Wire.read() << 8 | Wire.read()))/ 340.0 + 36.53;

      int16_t GyroX=Wire.read()<<8 | Wire.read();
      int16_t GyroY=Wire.read()<<8 | Wire.read();
      int16_t GyroZ=Wire.read()<<8 | Wire.read();
      mData.RateRoll=(float)GyroX/65.5;
      mData.RatePitch=(float)GyroY/65.5;
      mData.RateYaw=(float)GyroZ/65.5;
      mData.AccX=(float)AccXLSB/4096;
      mData.AccY=(float)AccYLSB/4096;
      mData.AccZ=(float)AccZLSB/4096;
    }
    
    void updateAbsAngles()
    {
      mData.AngleRoll=atan(mData.AccY/sqrt(mData.AccX*mData.AccX+mData.AccZ*mData.AccZ))*1/(3.142/180);
      mData.AnglePitch=-atan(mData.AccX/sqrt(mData.AccY*mData.AccY+mData.AccZ*mData.AccZ))*1/(3.142/180);
    
      // Get the current time
      unsigned long currentTime = micros();
       // Calculate time difference (dt) between current and last loop iteration
      double dt = (double)(currentTime - mLastTimeMeasureYaw) / 1000000.0; // Convert to seconds
      mLastTimeMeasureYaw = currentTime;
      // Integrate the gyroscope data to get yaw
      mData.AngleYaw += mData.RateYaw * dt;
    }
public:
    void readAndUpdateValues() {
      readMPU();
      updateAbsAngles();
    }
};

