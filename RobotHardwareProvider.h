#pragma once
#include "IHardwareProvider.h"
#include "GPIO.h"
#include "Cody.h"
#include <Arduino.h>
#include <Wire.h>

// PWM
#define L_PWM 2
#define R_PWM 4
#define A1_PWM 16 // X
#define A2_PWM 17 // Z
#define A3_PWM 18 // Wheels
#define A4_PWM 19 // Mill

// Direction
#define L_IN_1 0
#define L_IN_2 1
#define R_IN_1 2
#define R_IN_2 3
#define A1_IN_1 4
#define A1_IN_2 5
#define A2_IN_1 6
#define A2_IN_2 7
#define A3_IN_1 8
#define A3_IN_2 9
#define A4_IN_1 10
#define A4_IN_2 11

// Driver protection
#define MAX_PWM_PER_TICK 5

// State indication
#define LED 12
#define BUZZER 5

class RobotHardwareProvider : public IHardwareProvider {
  public:
    void initialize() override {
      pinMode(L_PWM, OUTPUT);
      pinMode(R_PWM, OUTPUT);
      pinMode(A1_PWM, OUTPUT);
      pinMode(A2_PWM, OUTPUT);
      pinMode(A3_PWM, OUTPUT);
      pinMode(A4_PWM, OUTPUT);
      pinMode(BUZZER, OUTPUT);
    }

    void move(NavigationData navigationData) override {
      moveMotor(navigationData.leftMotor, L_IN_2, L_IN_1, L_PWM);
      moveMotor(navigationData.rightMotor, R_IN_1, R_IN_2, R_PWM);
    }

    void moveToolhead(ToolheadData toolheadData) override {
      // Driver protection
      xAxisPwm = getPwm(toolheadData.xAxisMotor, xAxisPwm);
      zAxisPwm = getPwm(toolheadData.zAxisMotor, zAxisPwm);

      // Move motors
      //moveMotor({ xAxisPwm > 0 ? true : false, std::abs(xAxisPwm) }, A2_IN_1, A2_IN_2, A2_PWM);
      //moveMotor({ zAxisPwm > 0 ? true : false, std::abs(zAxisPwm) }, A1_IN_1, A1_IN_2, A1_PWM);
      moveMotor(toolheadData.xAxisMotor, A1_IN_2, A1_IN_1, A1_PWM);
      moveMotor(toolheadData.zAxisMotor, A2_IN_1, A2_IN_2, A2_PWM);
    }

    void moveWheels(WheelsData wheelsData) override {
      wheelsPwm = getPwm(wheelsData.wheelsMotor, wheelsPwm);
      moveMotor({ wheelsPwm > 0 ? true : false, std::abs(wheelsPwm) }, A3_IN_1, A3_IN_2, A3_PWM);
    }

    void moveMill(MillData millData) override {
      millPwm = getPwm(millData.millMotor, millPwm);
      moveMotor({ millPwm > 0 ? true : false, std::abs(millPwm) }, A4_IN_1, A4_IN_2, A4_PWM);
    }

    void writeLed(uint8_t value) override {
      GPIO::digitalWrite(LED, value);
    }

    void toneBuzzer(unsigned int frequency, unsigned long duration = 0) override {
      if (duration == 0) tone(BUZZER, frequency);
      else tone(BUZZER, frequency, duration);
    }

    void noToneBuzzer() override {
      noTone(BUZZER);
    }

  private:
    int xAxisPwm = 0;
    int zAxisPwm = 0;
    int wheelsPwm = 0;
    int millPwm = 0;

    void moveMotor(MotorData motorData, unsigned int in1, unsigned int in2, unsigned int pwm) {
      if (motorData.forwards) {
        GPIO::digitalWrite(in1, HIGH);
        GPIO::digitalWrite(in2, LOW);
      } else {
        GPIO::digitalWrite(in1, LOW);
        GPIO::digitalWrite(in2, HIGH);
      }

      analogWrite(pwm, motorData.pwm);
    }

    int getPwm(MotorData motorData, int pwm) {
      int requestedPwm = motorData.pwm * (motorData.forwards ? 1 : -1);
      if (requestedPwm > pwm + MAX_PWM_PER_TICK) requestedPwm = pwm + MAX_PWM_PER_TICK;
      if (requestedPwm < pwm - MAX_PWM_PER_TICK) requestedPwm = pwm - MAX_PWM_PER_TICK;

      return requestedPwm;
    }
};
