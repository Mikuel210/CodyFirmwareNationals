#pragma once
#include "IDataProvider.h"
#include "GPIO.h"
#include <TCS_Clone.h>
#include <Arduino.h>
#include <Wire.h>
#include <ESP32Encoder.h>
#include <FastIMU.h>
#include <ADS1115_WE.h>
#include <Wire.h>

#define IMU_ADDRESS 0x68
#define ADS_ADDRESS 0x48
#define G 9.807
#define DEG2RAD 0.01745329251

// Encoders
#define L_A 26
#define L_B 27
#define R_A 32
#define R_B 33
#define A1_A 34
#define A1_B 35
#define A2_A 36
#define A2_B 39
#define A3_A 12
#define A3_B 13
#define A4_A 14
#define A4_B 15

// Switches
#define LIMIT_1 13
#define LIMIT_2 14
#define BUTTON 15

// Camera
#define CAM_RX 23
#define CAM_TX 25

// Sensors
ADS1115_WE ADS = ADS1115_WE(ADS_ADDRESS);
TCS_Clone TCS;

class SensorDataProvider : public IDataProvider {
  public:
    void initialize() override {
      // Initialize IMU
      Wire.begin();
      Wire.setClock(400000);

      IMU.setIMUGeometry(7);
      int error = IMU.init({ 0 }, IMU_ADDRESS);

      if (error != 0) {
        Serial.print("Error initializing IMU: ");
        Serial.println(error);
      }

      // Initialize ADS1115
      Wire.begin();

      while (!ADS.init()) {
        Serial.println("Initializing ADS");
        delay(10);
      }

      ADS.setVoltageRange_mV(ADS1115_RANGE_6144);

      // Initialize TCS34725
      if (!TCS.begin()) Serial.println("Error initializing TCS34725");
      TCS.setGain(TCS_GAIN_16X);

      TCS.calibrateBlack({ 2700,  3850,  3800 });
      TCS.calibrateWhite({ 11450, 12450, 8700 });

      // Initialize PCF8575
      GPIO::initialize();
      
      // Initialize encoders
      leftEncoder.attachHalfQuad(L_A, L_B);
      leftEncoder.setCount(0);
      rightEncoder.attachHalfQuad(R_A, R_B);
      rightEncoder.setCount(0);
      xEncoder.attachHalfQuad(A1_A, A1_B);
      xEncoder.setCount(0);
      zEncoder.attachHalfQuad(A2_A, A2_B);
      zEncoder.setCount(0);
      wheelsEncoder.attachHalfQuad(A3_A, A3_B);
      wheelsEncoder.setCount(0);
      millEncoder.attachHalfQuad(A4_A, A4_B);
      millEncoder.setCount(0);

      // Initialize camera communication
      Serial2.begin(9600, SERIAL_8N1, CAM_RX, CAM_TX);
    }

    SensorData getData() override {
      SensorData data;

      // Get IMU data
      /*
      IMU.update();
      data.acceleration = readAccelerometer();
      data.gyroscope = readGyroscope();
      data.magnetometer = readMagnetometer();
      */

      // Get wheel pulses
      data.leftPulses = leftEncoder.getCount();
      data.rightPulses = -rightEncoder.getCount();
      data.xAxisPulses = xEncoder.getCount();
      data.zAxisPulses = zEncoder.getCount();
      data.wheelsPulses = wheelsEncoder.getCount();
      data.millPulses = millEncoder.getCount();

      // Get color sensor
      RGBCNorm color = TCS.readNormalised();
      data.colorData = ColorData(color.r, color.g, color.b);

      // Get BMS
      data.bms3 = readVoltage(ADS1115_COMP_2_GND);

      // Get switches
      data.xLimit = GPIO::digitalRead(LIMIT_2) == 1;
      data.zLimit = GPIO::digitalRead(LIMIT_1) == 1;
      data.button = GPIO::digitalRead(BUTTON) == 1;

      return data;
    }

    SensorData getPulses() override {
      SensorData data;

      data.leftPulses = leftEncoder.getCount();
      data.rightPulses = -rightEncoder.getCount();
      data.xAxisPulses = xEncoder.getCount();
      data.zAxisPulses = zEncoder.getCount();
      data.wheelsPulses = wheelsEncoder.getCount();
      data.millPulses = millEncoder.getCount();

      return data;
    }

    SensorData getButtons() override {
      SensorData data;

      data.xLimit = GPIO::digitalRead(LIMIT_2) == 1;
      data.zLimit = GPIO::digitalRead(LIMIT_1) == 1;
      data.button = GPIO::digitalRead(BUTTON) == 1;

      return data;
    }

  private:
    // Encoders
    ESP32Encoder leftEncoder;
    ESP32Encoder rightEncoder;
    ESP32Encoder xEncoder;
    ESP32Encoder zEncoder;
    ESP32Encoder wheelsEncoder;
    ESP32Encoder millEncoder;

    // Orientation
    MPU9250 IMU;
    AccelData accelData;
    GyroData gyroData;
    MagData magData;

    Vector3 readAccelerometer() {
      Vector3 acceleration;
      IMU.getAccel(&accelData);
      acceleration.x = accelData.accelX * G;
      acceleration.y = accelData.accelY * G;
      acceleration.z = accelData.accelZ * G;

      return acceleration;
    }

    Vector3 readGyroscope() {
      Vector3 gyroscope;
      IMU.getGyro(&gyroData);
      gyroscope.x = gyroData.gyroX * DEG2RAD;
      gyroscope.y = gyroData.gyroY * DEG2RAD;
      gyroscope.z = gyroData.gyroZ * DEG2RAD;

      return gyroscope;
    }

    Vector3 readMagnetometer() {
      Vector3 magnetometer;
      IMU.getMag(&magData);
      magnetometer.x = magData.magX;
      magnetometer.y = magData.magY;
      magnetometer.z = magData.magZ;

      return magnetometer;
    }

    double readVoltage(ADS1115_MUX channel) {
      ADS.setCompareChannels(channel);
      ADS.startSingleMeasurement();

      while (ADS.isBusy()) { delay(0); }
      double voltage = ADS.getResult_V();

      return voltage;
    }
};
