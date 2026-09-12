// Global parameters
#undef SIMULATION
#undef DEBUG
#define BAUD_RATE 115200

// Debug
#ifdef DEBUG
  #include "Plotter.h"
  #include "Globals.h"

  Plotter plot;
  double plotX = 0;
  double plotY = 0;
#endif

// Include dependencies
#include "Cody.h"
#include "BMS.h"
#include "Program.h"

#ifdef SIMULATION
  #include "SimulationDataProvider.h"
  #include "SimulationHardwareProvider.h"
  SimulationDataProvider dataProvider;
  SimulationHardwareProvider hardwareProvider;
#else
  #include "SensorDataProvider.h"
  #include "RobotHardwareProvider.h"
  SensorDataProvider dataProvider;
  RobotHardwareProvider hardwareProvider;
#endif

// Setup
void setup() {
    Serial.begin(BAUD_RATE);
    delay(500);

    dataProvider.initialize();
    hardwareProvider.initialize();
    Cody::initialize(dataProvider, hardwareProvider);
    BMS::initialize();

#ifdef DEBUG
    plot.Begin();
    plot.AddXYGraph("Position", 1000, "x", plotX, "y", plotY);
#endif

    waitForButton();
    Cody::homeAsync()->await();
    Fusion::homingComplete();
    delay(500);
    // double xPosition = TOOLHEAD_PICK_START_X + BLOCK_DISTANCE_START * 2;
    // Cody::moveToolheadAsync(xPosition, 0)->await();
    waitForButton();
    Program::go();
}

void waitForButton() {
  SensorData sensorData;

  while (!sensorData.button) {
    sensorData = dataProvider.getData();
    delay(10);
  }
}

void loop() {}
