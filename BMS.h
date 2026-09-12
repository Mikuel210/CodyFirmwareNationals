#pragma once
#include "IDataProvider.h"
#include "IHardwareProvider.h"
#include "Fusion.h"
#include "Task.h"
#include "TaskArgs.h"
#include "Cody.h"

// BMS parameters
#define BMS_HZ 20
#define VOLTAGE_THRESHOLD 3.5
#define BMS_FREQUENCY 440

class BMS {
  public:
    static void initialize() {
      Task* task = new Task("bms", bmsTask);
      TaskArgs* args = new TaskArgs();
      args->task = task;
      task->start(args);
    }

  private:
    static void bmsTask(void* task) {
      TaskArgs* args = (TaskArgs*)task;

      while (true) {
        unsigned long msStart = millis();

        SensorData sensorData = Cody::dataProvider->getData();
        FusionData fusionData = Fusion::getData(sensorData);

        if (fusionData.voltage <= VOLTAGE_THRESHOLD)
          Cody::hardwareProvider->toneBuzzer(BMS_FREQUENCY, 1000.0 / BMS_HZ / 2);


        // Serial.print(sensorData.colorData.r); Serial.print(", ");
        // Serial.print(sensorData.colorData.g); Serial.print(", ");
        // Serial.print(sensorData.colorData.b); Serial.print(", || ");
        // Serial.println(fusionData.color);
        Serial.println(fusionData.toolheadPosition.x);

        vTaskDelay(max(1000.0 / BMS_HZ - (millis() - msStart), 0.0));
      }


      args->task->stop();
      delete args;
    }
};
