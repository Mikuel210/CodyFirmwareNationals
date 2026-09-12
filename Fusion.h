#pragma once
#include "Vector3.h"
#include "SensorData.h"
#include "FusionData.h"
#include "Color.h"
#include <SensorFusion.h>
#include <vector>

// Fusion parameters
#define TRAVEL_PER_REVOLUTION_MM 272.376083066
#define X_AXIS_MM_PER_REVOLUTION 69.115038379
#define Z_AXIS_MM_PER_REVOLUTION 56.5486677646
#define WHEELS_MM_PER_REVOLUTION 38.2790981791
#define TICKS_PER_REVOLUTION 600.0
#define N20_TICKS_PER_REVOLUTION 5000.0
#define DISTANCE_BETWEEN_WHEELS_MM 228.0

// Colors
#define SATURATION_THRESHOLD 0.1
#define HUE_YELLOW 60.0
#define HUE_GREEN 125.0f
#define HUE_BLUE 220.0

class Fusion {
  public:
    static FusionData getData(SensorData sensorData) {
      // Get orientation
      FusionData fusionData;

      // Get position
      double leftDistanceMm = (sensorData.leftPulses - previousSensorData.leftPulses) * TRAVEL_PER_REVOLUTION_MM / TICKS_PER_REVOLUTION;
      double rightDistanceMm = (sensorData.rightPulses - previousSensorData.rightPulses) * TRAVEL_PER_REVOLUTION_MM / TICKS_PER_REVOLUTION;
      double deltaDistanceMm = (leftDistanceMm + rightDistanceMm) / 2.0;

      double deltaOrientation = (leftDistanceMm - rightDistanceMm) / DISTANCE_BETWEEN_WHEELS_MM * (180.0 / M_PI);
      double orientation = previousFusionData.orientation + deltaOrientation;
      double averageOrientation = previousFusionData.orientation + deltaOrientation / 2.0;

      double deltaX = deltaDistanceMm * sin(averageOrientation * DEG_TO_RAD);
      double deltaY = deltaDistanceMm * cos(averageOrientation * DEG_TO_RAD);

      // Get toolhead position
      double toolheadDeltaX = (sensorData.xAxisPulses - previousSensorData.xAxisPulses) * X_AXIS_MM_PER_REVOLUTION / N20_TICKS_PER_REVOLUTION / 2;
      double toolheadDeltaZ = (sensorData.zAxisPulses - previousSensorData.zAxisPulses) * Z_AXIS_MM_PER_REVOLUTION / N20_TICKS_PER_REVOLUTION;

      // Get wheels position
      double wheelsDelta = (sensorData.wheelsPulses - previousSensorData.wheelsPulses) * WHEELS_MM_PER_REVOLUTION / N20_TICKS_PER_REVOLUTION;

      // Get mill position
      double millDelta = (sensorData.millPulses - previousSensorData.millPulses) / N20_TICKS_PER_REVOLUTION * 360.0;

      // Get voltage
      double averageVoltage = sensorData.bms3 * 2.0;

      // Get color
      fusionData.color = rgbToColor(sensorData.colorData);

      // Construct fusion data
      fusionData.orientation = orientation;
      fusionData.position = Vector3(previousFusionData.position.x + deltaX, previousFusionData.position.y + deltaY, 0);
      fusionData.toolheadPosition = Vector3(previousFusionData.toolheadPosition.x + toolheadDeltaX, previousFusionData.toolheadPosition.y + toolheadDeltaZ);
      fusionData.wheelsPosition = Vector3(previousFusionData.wheelsPosition.x - wheelsDelta);
      fusionData.millPosition = Vector3(previousFusionData.millPosition.x + millDelta);
      fusionData.voltage = averageVoltage;

      // Set previous data
      previousSensorData = sensorData;
      previousFusionData = fusionData;

      return fusionData;
    }

    static void restart() {
      previousFusionData.orientation = 0;
      previousFusionData.position = Vector3();
      previousFusionData.wheelsPosition = Vector3();
      previousFusionData.millPosition = Vector3();

      /*


          float orientation;
          Vector3 position;

          Vector3 toolheadPosition;
          Vector3 wheelsPosition;
          Vector3 millPosition;


          */
    }

    static void setX(double x) {
      previousFusionData.position = Vector3(x, previousFusionData.position.y);
    }

    static void setY(double y) {
      previousFusionData.position = Vector3(previousFusionData.position.x, y);
    }

    static void setOrientation(double orientation) {
      previousFusionData.orientation = orientation;
    }

    static void homingComplete() {
      previousFusionData.toolheadPosition = Vector3();
    }

    static void xHomingComplete() {
            previousFusionData.toolheadPosition.y = 0;
        }

    static void zHomingComplete() {
        previousFusionData.toolheadPosition.y = 0;
    }

  private:
    static float deltat;
    static SF fusion;

    static FusionData previousFusionData;
    static SensorData previousSensorData;

    static Color rgbToColor(ColorData colorData) {
      // Convert RGB to HSV
      double r = colorData.r;
      double g = colorData.g;
      double b = colorData.b;

      double cmax  = r > g ? (r > b ? r : b) : (g > b ? g : b);
      double cmin  = r < g ? (r < b ? r : b) : (g < b ? g : b);
      double delta = cmax - cmin;

      double s = (cmax > 0.0) ? (delta / cmax) : 0.0;

      // Low saturation
      if (s < SATURATION_THRESHOLD)
      {
        if (cmax > 0.5) return WHITE;
        return BLACK;
      }

      // Get hue
      double h;

      if (cmax == r)
        h = 60.0f * std::fmod((g - b) / delta, 6.0f);
      else if (cmax == g)
        h = 60.0f * ((b - r) / delta + 2.0f);
      else
        h = 60.0f * ((r - g) / delta + 4.0f);

      if (h < 0.0f) h += 360.0f;

      // Find color with smallest hue distance
      const double hues[]  = { HUE_YELLOW, HUE_GREEN, HUE_BLUE };
      const Color colors[] = { YELLOW,     GREEN,     BLUE     };

      double minDistance = 361.0f;
      Color result = WHITE;

      for (int i = 0; i < 3; i++) {
        float distance = std::abs(h - hues[i]);
        if (distance > 180.0f) distance = 360.0f - distance;

        if (distance < minDistance) {
          minDistance = distance;
          result = colors[i];
        }
      }

      return result;
    }
};
