#pragma once
#include "IDataProvider.h"
#include "IHardwareProvider.h"
#include "SensorData.h"
#include "FusionData.h"
#include "Fusion.h"
#include "Navigation.h"
#include "NavigationTarget.h"
#include "NavigationData.h"
#include "ToolheadData.h"
#include "WheelsData.h"
#include "MillData.h"
#include "Pursuit.h"
#include "PursuitData.h"
#include "Task.h"
#include "TaskArgs.h"
#include <vector>

#ifdef DEBUG
  #include "Plotter.h"
  #include "Globals.h"
#endif

// Task parameters
#define HZ 200.0

// Movement
#define MOVEMENT_ACCELERATION_MS 500.0
#define MOVEMENT_DECELERATION_MM 250.0
#define MOVEMENT_MIN_SPEED 17.0
#define MOVEMENT_LOOKAHEAD 100.0
#define TRANSITION_LOOKAHEAD 250.0

// Toolhead
#define TOOLHEAD_ACCELERATION_MS 250.0
#define TOOLHEAD_DECELERATION_MM 10.0
#define TOOLHEAD_MIN_SPEED 80.0
#define TOOLHEAD_LOOKAHEAD 10.0

// Wheels
#define WHEELS_ACCELERATION_MS 100.0
#define WHEELS_DECELERATION_MM 10.0
#define WHEELS_MIN_SPEED 30.0
#define WHEELS_LOOKAHEAD 10.0

// Mill
#define MILL_ACCELERATION_MS 100.0
#define MILL_DECELERATION_MM 10.0
#define MILL_MIN_SPEED 30.0
#define MILL_LOOKAHEAD 25.0

class Cody {
  public:
    static IDataProvider* dataProvider;
    static IHardwareProvider* hardwareProvider;

    static void initialize(IDataProvider& dataProvider_, IHardwareProvider& hardwareProvider_) {
      dataProvider = &dataProvider_;
      hardwareProvider = &hardwareProvider_;
    }

    // Drive
    static Task* moveAsync(double x, double y, double speed = 45, bool backwards = false, double lookaheadDistance = MOVEMENT_LOOKAHEAD,
      double transitionDistance = TRANSITION_LOOKAHEAD, double decelerationMm = MOVEMENT_DECELERATION_MM) {

      addPathPoint(x, y);
      return followPathAsync(speed, backwards, lookaheadDistance, transitionDistance, decelerationMm);
    }

    static void addPathPoint(double x, double y) {
      Vector3 point = Vector3(x, y);
      Vector3 firstPoint;

      // Move pointer
      if (pathData.points.size() == 0)
        firstPoint = Fusion::getData(dataProvider->getPulses()).position;
      else
        firstPoint = pathData.points[0];

      pointer = point;
      orientation = atan2(point.x - firstPoint.x, point.y - firstPoint.y) * (180.0 / M_PI);

      // Add point
      pathData.points.push_back(point);
    }

    static Task* followPathAsync(double speed = 45, bool backwards = false, double lookaheadDistance = MOVEMENT_LOOKAHEAD,
      double transitionDistance = TRANSITION_LOOKAHEAD, double decelerationMm = MOVEMENT_DECELERATION_MM, double accelerationMs = MOVEMENT_ACCELERATION_MS) {

      Task* task = new Task("followPath", followPathTask);
      FollowPathArgs* args = new FollowPathArgs();
      pathData.lookaheadDistance = lookaheadDistance;

      args->task = task;
      args->data = &pathData;
      args->speed = speed / 100.0;
      args->minSpeed = MOVEMENT_MIN_SPEED / 100.0;
      args->accelerationMs = accelerationMs;
      args->decelerationMm = decelerationMm;
      args->transitionLookahead = transitionDistance;
      args->positionMember = &FusionData::position;
      args->navigationTarget = &Navigation::drive;
      args->stopFunction = &stopRobot;

      if (backwards) args->moveFunction = &moveRobotBackwards;
      else args->moveFunction = &moveRobot;

      task->start(args);
      return task;
    }

    static Task* detectColorAsync(int transitionMs, double startSpeed = 40, double endSpeed = 15, bool backwards = false, Color color = BLACK,
      double lookaheadDistance = MOVEMENT_LOOKAHEAD, double transitionDistance = TRANSITION_LOOKAHEAD, double decelerationMm = MOVEMENT_DECELERATION_MM) {

      Task* task = new Task("detectColor", detectColorTask);
      DetectColorArgs* args = new DetectColorArgs();
      pathData.lookaheadDistance = lookaheadDistance;

      args->task = task;
      args->transitionMs = transitionMs;
      args->startSpeed = startSpeed / 100.0;
      args->endSpeed = endSpeed / 100.0;
      args->minSpeed = MOVEMENT_MIN_SPEED / 100.0;
      args->accelerationMs = MOVEMENT_ACCELERATION_MS;
      args->decelerationMm = decelerationMm;
      args->transitionLookahead = transitionDistance;

      if (backwards) args->moveFunction = &moveRobotBackwards;
      else args->moveFunction = &moveRobot;

      task->start(args);
      return task;
    }

    static Task* rotateToAsync(double heading, double speed = 25, double decelerationDegrees = 45) {
      Task* task = new Task("rotate", rotateTask);
      RotateArgs* args = new RotateArgs();

      args->task = task;
      args->heading = heading;
      args->speed = speed / 100.0;
      args->accelerationMs = MOVEMENT_ACCELERATION_MS;
      args->decelerationDegrees = decelerationDegrees;
      args->minSpeed = MOVEMENT_MIN_SPEED / 200.0;

      orientation = heading;
      task->start(args);
      return task;
    }

    // Movement
    static Vector3 pointer;
    static double orientation;

    static void forwards(double distance) {
      double x = pointer.x + distance * sin(orientation);
      double y = pointer.y + distance * cos(orientation);
      addPathPoint(x, y);
    }

    static void backwards(double distance) {
      forwards(-distance);
    }

    static void turn(double degrees) {
      orientation += degrees;
    }

    static void setPosition(double x, double y, double theta) {
      pointer = Vector3(x, y);
      orientation = theta;

      Fusion::setX(x);
      Fusion::setY(y);
      Fusion::setOrientation(theta);
    }

    static void setXOrientation(double x, double theta) {
      pointer.x = x;
      orientation = theta;

      Fusion::setX(x);
      Fusion::setOrientation(theta);
    }

    static void setYOrientation(double y, double theta) {
      pointer.y = y;
      orientation = theta;

      Fusion::setY(y);
      Fusion::setOrientation(theta);
    }

    static void setX(double x) {
      pointer.x = x;
      Fusion::setX(x);
    }

    static void setY(double y) {
      pointer.y = y;
      Fusion::setY(y);
    }

    // Toolhead
    static Task* moveToolheadAsync(double x, double z, double speed = 100, double lookaheadDistance = TOOLHEAD_LOOKAHEAD) {
      addToolheadPathPoint(x, z);
      return followToolheadPathAsync(speed, lookaheadDistance);
    }

    static void addToolheadPathPoint(double x, double z) {
      Vector3 point = Vector3(x, z);
      toolheadPathData.points.push_back(point);
    }

    static Task* followToolheadPathAsync(double speed = 100, double lookaheadDistance = TOOLHEAD_LOOKAHEAD) {
      FollowPathArgs* args = new FollowPathArgs();
      Task* task = new Task("followToolheadPath", followPathTask);
      toolheadPathData.lookaheadDistance = lookaheadDistance;

      args->task = task;
      args->data = &toolheadPathData;
      args->speed = speed / 100.0;
      args->minSpeed = TOOLHEAD_MIN_SPEED / 100.0;
      args->accelerationMs = TOOLHEAD_ACCELERATION_MS;
      args->decelerationMm = TOOLHEAD_DECELERATION_MM;
      args->transitionLookahead = lookaheadDistance;
      args->positionMember = &FusionData::toolheadPosition;
      args->navigationTarget = &Navigation::toolhead;
      args->moveFunction = &moveToolhead;
      args->stopFunction = &stopToolhead;

      task->start(args);
      return task;
    }

    static Task* homeAsync(double speed = 100) {
      HomeArgs* args = new HomeArgs();
      Task* task = new Task("home", homeTask);

      args->task = task;
      args->speed = speed / 100.0;

      task->start(args);
      return task;
    }

    static Task* homeXAsync(double speed = 100) {
          HomeArgs* args = new HomeArgs();
          Task* task = new Task("homeX", homeXTask);

          args->task = task;
          args->speed = speed / 100.0;

          task->start(args);
          return task;
        }

    static Task* homeZAsync(double speed = 100) {
      HomeArgs* args = new HomeArgs();
      Task* task = new Task("homeZ", homeZTask);

      args->task = task;
      args->speed = speed / 100.0;

      task->start(args);
      return task;
    }

    static Task* moveZMsAsync(double ms, double speed = 100, bool direction = true) {
      MoveZMsArgs* args = new MoveZMsArgs();
      Task* task = new Task("moveZMs", moveZMsTask);

      args->task = task;
      args->ms = ms;
      args->speed = speed / 100.0;
      args->forwards = direction;

      task->start(args);
      return task;
    }

    static Task* zUpAsync() {
      return moveZMsAsync(900);
    }

    // Wheels
    static Task* moveWheelsAsync(double z, double speed = 100, double lookaheadDistance = WHEELS_LOOKAHEAD) {
      addWheelsPathPoint(z);
      return followWheelsPathAsync(speed, lookaheadDistance);
    }

    static void addWheelsPathPoint(double z) {
      Vector3 point = Vector3(z);
      wheelsPathData.points.push_back(point);
    }

    static Task* followWheelsPathAsync(double speed = 100, double lookaheadDistance = WHEELS_LOOKAHEAD) {
      FollowPathArgs* args = new FollowPathArgs();
      Task* task = new Task("followWheelsPath", followPathTask);
      wheelsPathData.lookaheadDistance = lookaheadDistance;

      args->task = task;
      args->data = &wheelsPathData;
      args->speed = speed / 100.0;
      args->minSpeed = WHEELS_MIN_SPEED / 100.0;
      args->accelerationMs = WHEELS_ACCELERATION_MS;
      args->decelerationMm = WHEELS_DECELERATION_MM;
      args->transitionLookahead = lookaheadDistance;
      args->positionMember = &FusionData::wheelsPosition;
      args->navigationTarget = &Navigation::wheels;
      args->moveFunction = &moveWheels;
      args->stopFunction = &stopWheels;

      task->start(args);
      return task;
    }

    static Task* moveWheelsMsAsync(double ms, bool up = true) {
        Task* task = new Task("moveWheelsMs", moveWheelsMsTask);
        MoveWheelsMsArgs* args = new MoveWheelsMsArgs();

        args->task = task;
        args->ms = ms;
        args->up = up;

        task->start(args);
        return task;
    }

    // Mill
    static Task* moveMillAsync(double z, double speed = 100, double lookaheadDistance = MILL_LOOKAHEAD) {
      addMillPathPoint(z);
      return followMillPathAsync(speed, lookaheadDistance);
    }

    static void addMillPathPoint(double z) {
      Vector3 point = Vector3(z);
      millPathData.points.push_back(point);
    }

    static Task* followMillPathAsync(double speed = 100, double lookaheadDistance = MILL_LOOKAHEAD) {
      FollowPathArgs* args = new FollowPathArgs();
      Task* task = new Task("followMillPath", followPathTask);
      millPathData.lookaheadDistance = lookaheadDistance;

      args->task = task;
      args->data = &millPathData;
      args->speed = speed / 100.0;
      args->minSpeed = MILL_MIN_SPEED / 100.0;
      args->accelerationMs = MILL_ACCELERATION_MS;
      args->decelerationMm = MILL_DECELERATION_MM;
      args->transitionLookahead = lookaheadDistance;
      args->positionMember = &FusionData::millPosition;
      args->navigationTarget = &Navigation::mill;
      args->moveFunction = &moveMill;
      args->stopFunction = &stopMill;

      task->start(args);
      return task;
    }

    static Task* moveMillMsAsync(double ms, bool forwards = true, double speed = 100.0) {
        /*
          Task* task = new Task("moveMillMs", moveMillMsTask);
          MoveMillMsArgs* args = new MoveMillMsArgs();

          args->task = task;
          args->ms = ms;
          args->forwards = !forwards;
          args->speed = speed / 100.0;

          task->start(args);
          return task;
          */
    }

    // LED
    static void writeLed(uint8_t value) {
      hardwareProvider->writeLed(value);
    }

  private:
    static PursuitData pathData;
    static PursuitData toolheadPathData;
    static PursuitData wheelsPathData;
    static PursuitData millPathData;

    using PositionMember = Vector3 (FusionData::*);
    using MoveFunction = void (*)(FusionData, double);
    using StopFunction = void (*)();

    // Follow path
    struct FollowPathArgs : TaskArgs {
      PursuitData* data;

      double speed;
      double minSpeed;
      double accelerationMs;
      double decelerationMm;
      double transitionLookahead;

      PositionMember positionMember;
      NavigationTarget* navigationTarget;
      MoveFunction moveFunction;
      StopFunction stopFunction;
    };

    static void followPathTask(void* task) {
      FollowPathArgs* args = (FollowPathArgs*)task;
      PursuitData* data = args->data;

      // Set first point
      SensorData sensorData = dataProvider->getPulses();
      FusionData fusionData = Fusion::getData(sensorData);
      args->navigationTarget->decelerationDistance = data->lookaheadDistance;

      data->lineIndex = 0;
      data->points.insert(data->points.begin(), fusionData.*(args->positionMember));

      // Get last segment
      int pointCount = data->points.size();
      Line lastSegment(data->points[pointCount - 2], data->points[pointCount - 1]);

      // Transition data
      PursuitData* transitionData = new PursuitData(data->points, args->transitionLookahead, data->lineIndex);
      Vector3 currentTransitionPoint;
      double currentTransitionTime = 0.0;
      int currentTransitionLineIndex = 0;

      unsigned long msStart = millis();

      while (true) {
        if (*(args->task->requestStop)) break;

        unsigned long msLoop = millis();

        SensorData sensorData = dataProvider->getPulses();
        FusionData fusionData = Fusion::getData(sensorData);
        Vector3 position = fusionData.*(args->positionMember);

        // Find lookahead and transition points
        Vector3 lookaheadPoint = Pursuit::findLookahead(position, data, true);
        Line lookaheadLine = { data->points[data->lineIndex], data->points[data->lineIndex + 1] };
        double lookaheadTime = Pursuit::findLookaheadTime(position, lookaheadLine, data->lookaheadDistance);

        Vector3 transitionPoint = Pursuit::findLookahead(position, transitionData, true);
        Line transitionLine = { data->points[transitionData->lineIndex], data->points[transitionData->lineIndex + 1] };
        double transitionTime = Pursuit::findLookaheadTime(position, transitionLine, transitionData->lookaheadDistance);

        if (transitionData->lineIndex > currentTransitionLineIndex) {
          currentTransitionPoint = transitionPoint;
          currentTransitionTime = transitionTime;
          currentTransitionLineIndex = transitionData->lineIndex;
        }

        if (data->lineIndex == currentTransitionLineIndex && lookaheadTime > currentTransitionTime)
          args->navigationTarget->target = lookaheadPoint;
        else
          args->navigationTarget->target = currentTransitionPoint;

        // Debug
        #ifdef DEBUG
          if (position.x == fusionData.position.x) {
            plotX = fusionData.position.y;
            plotY = -fusionData.position.x;
            plot.Plot();
          }
        #endif

        // End condition
        bool inLastSegment = data->lineIndex == data->points.size() - 2;
        double time = Pursuit::getClosestTime(lastSegment, position);
        if (inLastSegment && time >= 1) break;

        // Acceleration and deceleration
        double distance = Navigation::getDistance(position, lastSegment.end);
        double acceleration = Navigation::dmap((msLoop - msStart) / args->accelerationMs, 0.0, 1.0, args->minSpeed, args->speed);
        double deceleration = Navigation::dmap(distance, args->decelerationMm, 0.0, args->speed, args->minSpeed);
        double speed = std::min((inLastSegment && distance <= args->decelerationMm) ? deceleration : acceleration, args->speed);
        args->moveFunction(fusionData, std::clamp(speed, 0.0, args->speed));

        vTaskDelay(max(1000.0 / HZ - (millis() - msLoop), 0.0));
      }

      args->stopFunction();
      data->points.clear();
      args->task->stop();

      delete transitionData;
      delete args;
    }

    // Home
    struct HomeArgs : TaskArgs {
      double speed;
    };

    static void homeTask(void* task) {
      HomeArgs* args = (HomeArgs*)task;
      int pwm = (int)(args->speed * 255.0);

      bool xLimit = false;
      bool zLimit = false;

      while (true) {
        unsigned long msStart = millis();

        SensorData sensorData = dataProvider->getData();
        FusionData fusionData = Fusion::getData(sensorData);

        xLimit = xLimit || sensorData.xLimit;
        zLimit = zLimit || sensorData.zLimit;

        MotorData xAxis(false, xLimit ? 0 : pwm);
        MotorData zAxis(false, zLimit ? 0 : pwm);
        hardwareProvider->moveToolhead({ xAxis, zAxis });

        if (sensorData.xLimit && sensorData.zLimit) break;
        vTaskDelay(max(1000.0 / HZ - (millis() - msStart), 0.0));
      }

      Fusion::homingComplete();
      args->task->stop();
      delete args;
    }

    static void homeXTask(void* task) {
          HomeArgs* args = (HomeArgs*)task;
          int pwm = (int)(args->speed * 255.0);

          bool xLimit = false;
          unsigned long msStart = millis();

          while (true) {
            unsigned long msLoop = millis();

            SensorData sensorData = dataProvider->getButtons();
            FusionData fusionData = Fusion::getData(sensorData);

            xLimit = xLimit || sensorData.xLimit;

            MotorData zAxis(false, 0);
            MotorData xAxis(false, xLimit ? 0 : pwm);
            hardwareProvider->moveToolhead({ xAxis, zAxis });

            if (sensorData.xLimit) break;
            if ((msLoop - msStart) >= 1000) break;
            vTaskDelay(max(1000.0 / HZ - (millis() - msLoop), 0.0));
          }

          Fusion::xHomingComplete();
          args->task->stop();
          delete args;
        }


    static void homeZTask(void* task) {
      HomeArgs* args = (HomeArgs*)task;
      int pwm = (int)(args->speed * 255.0);

      bool zLimit = false;
      unsigned long msStart = millis();

      while (true) {
        unsigned long msLoop = millis();

        SensorData sensorData = dataProvider->getButtons();
        FusionData fusionData = Fusion::getData(sensorData);

        zLimit = zLimit || sensorData.zLimit;

        MotorData xAxis(false, 0);
        MotorData zAxis(false, zLimit ? 0 : pwm);
        hardwareProvider->moveToolhead({ xAxis, zAxis });

        if (sensorData.zLimit) break;
        if ((msLoop - msStart) >= 1000) break;
        vTaskDelay(max(1000.0 / HZ - (millis() - msLoop), 0.0));
      }

      Fusion::zHomingComplete();
      args->task->stop();
      delete args;
    }

    struct MoveZMsArgs : TaskArgs {
      double ms;
      double speed;
      bool forwards;
    };

    static void moveZMsTask(void* task) {
      MoveZMsArgs* args = (MoveZMsArgs*)task;
      int pwm = (int)(args->speed * 255.0);

      unsigned long msStart = millis();

      while (true) {
        unsigned long msLoop = millis();

        hardwareProvider->moveToolhead({{false, 0}, {args->forwards, pwm}});

        if ((msLoop - msStart) >= args->ms) break;
        vTaskDelay(max(1000.0 / HZ - (millis() - msLoop), 0.0));
      }

      hardwareProvider->moveToolhead({{false, 0}, {false, 0}});
      args->task->stop();
      delete args;
    }

    // Mill
    struct MoveMillMsArgs : TaskArgs {
          double ms;
          bool forwards;
          double speed;
    };

    static void moveMillMsTask(void* task) {
        MoveMillMsArgs* args = (MoveMillMsArgs*)task;
        unsigned long msStart = millis();

        while (true) {
        unsigned long msLoop = millis();

        if ((msLoop - msStart) >= args->ms) break;
        hardwareProvider->moveToolhead({{args->forwards, args->speed * 255.0}, {false, 0}});

        vTaskDelay(max(1000.0 / HZ - (millis() - msStart), 0.0));
        }

        hardwareProvider->moveToolhead({{false, 0}, {false, 0}});
        args->task->stop();
        delete args;
    }

    // Detect color
    struct DetectColorArgs : TaskArgs {
      Color color;
      int transitionMs;

      double startSpeed;
      double endSpeed;
      double minSpeed;
      double accelerationMs;
      double decelerationMm;
      double transitionLookahead;

      MoveFunction moveFunction;
    };

    static void detectColorTask(void* task) {
      DetectColorArgs* args = (DetectColorArgs*)task;
      PursuitData* data = &pathData;

      // Set first point
      SensorData sensorData = dataProvider->getPulses();
      FusionData fusionData = Fusion::getData(sensorData);
      Navigation::drive.decelerationDistance = data->lookaheadDistance;

      data->lineIndex = 0;
      data->points.insert(data->points.begin(), fusionData.position);

      // Get last segment
      int pointCount = data->points.size();
      Line lastSegment(data->points[pointCount - 2], data->points[pointCount - 1]);

      // Transition data
      PursuitData* transitionData = new PursuitData(data->points, args->transitionLookahead, data->lineIndex);
      Vector3 currentTransitionPoint;
      double currentTransitionTime = 0.0;
      int currentTransitionLineIndex = 0;

      unsigned long msStart = millis();

      while (true) {
        unsigned long msLoop = millis();
        bool detectingColor = (msLoop - msStart) >= args->transitionMs;

        SensorData sensorData = dataProvider->getData();
        FusionData fusionData = Fusion::getData(sensorData);
        Vector3 position = fusionData.position;

        // Find lookahead and transition points
        Vector3 lookaheadPoint = Pursuit::findLookahead(position, data, true);
        Line lookaheadLine = { data->points[data->lineIndex], data->points[data->lineIndex + 1] };
        double lookaheadTime = Pursuit::findLookaheadTime(position, lookaheadLine, data->lookaheadDistance);

        Vector3 transitionPoint = Pursuit::findLookahead(position, transitionData, true);
        Line transitionLine = { data->points[transitionData->lineIndex], data->points[transitionData->lineIndex + 1] };
        double transitionTime = Pursuit::findLookaheadTime(position, transitionLine, transitionData->lookaheadDistance);

        if (transitionData->lineIndex > currentTransitionLineIndex) {
          currentTransitionPoint = transitionPoint;
          currentTransitionTime = transitionTime;
          currentTransitionLineIndex = transitionData->lineIndex;
        }

        if (data->lineIndex == currentTransitionLineIndex && lookaheadTime > currentTransitionTime)
          Navigation::drive.target = lookaheadPoint;
        else
          Navigation::drive.target = currentTransitionPoint;

        // End condition: finished path
        bool inLastSegment = data->lineIndex == data->points.size() - 2;
        double time = Pursuit::getClosestTime(lastSegment, position);
        if (inLastSegment && time >= 1) break;

        // End condition: color
        if (detectingColor && fusionData.color == args->color) break;

        // Acceleration and speed transition
        double acceleration = Navigation::dmap((msLoop - msStart) / args->accelerationMs, 0.0, 1.0, args->minSpeed, args->startSpeed);
        double speed = detectingColor ? args->endSpeed : std::min(acceleration, args->startSpeed);
        args->moveFunction(fusionData, std::clamp(speed, 0.0, args->startSpeed));

        vTaskDelay(max(1000.0 / HZ - (millis() - msLoop), 0.0));
      }

      stopRobot();
      data->points.clear();
      args->task->stop();

      delete transitionData;
      delete args;
    }

    // Rotate
    struct RotateArgs : TaskArgs {
      double heading;
      double speed;

      double accelerationMs;
      double decelerationDegrees;
      double minSpeed;
    };

    static void rotateTask(void* task) {
      RotateArgs* args = (RotateArgs*)task;

      unsigned long msStart = millis();

      while (true) {
        unsigned long msLoop = millis();

        SensorData sensorData = dataProvider->getPulses();
        FusionData fusionData = Fusion::getData(sensorData);

        // Get orientation correction
        float targetOrientation = args->heading;
        float error = targetOrientation - fusionData.orientation;
        while (error > 180.0) { error -= 360.0; targetOrientation -= 360.0; }
        while (error < -180.0) { error += 360.0; targetOrientation += 360.0; }

        // Acceleration and deceleration
        double acceleration = Navigation::dmap((msLoop - msStart) / args->accelerationMs, 0.0, 1.0, args->minSpeed, args->speed);
        double deceleration = Navigation::dmap(std::abs(error), args->decelerationDegrees, 0.0, args->speed, args->minSpeed);
        double speed = std::min(std::abs(error) <= args->decelerationDegrees ? deceleration : acceleration, args->speed);

        // End condition
        if (error > -1.0 && error < 1.0) break;

        // Move
        hardwareProvider->move({{ error > 0, speed * 255.0 }, { error < 0, speed * 255.0 }});

        vTaskDelay(max(1000.0 / HZ - (millis() - msStart), 0.0));
      }

      stopRobot();
      args->task->stop();
      delete args;
    }

    // Wheels
    struct MoveWheelsMsArgs : TaskArgs {
              double ms;
              bool up;
        };

    static void moveWheelsMsTask(void* task) {
        MoveWheelsMsArgs* args = (MoveWheelsMsArgs*)task;
        unsigned long msStart = millis();

        while (true) {
            unsigned long msLoop = millis();

            if ((msLoop - msStart) >= args->ms) break;
            hardwareProvider->moveWheels({{!(args->up), 255}});

            vTaskDelay(max(1000.0 / HZ - (millis() - msStart), 0.0));
        }

        hardwareProvider->moveWheels({{false, 0}});
        args->task->stop();
        delete args;
    }

    // Move functions
    static void moveRobot(FusionData fusionData, double speed) {
      NavigationData navigationData = Navigation::getData(fusionData, speed);
      hardwareProvider->move(navigationData);
    }

    static void moveRobotBackwards(FusionData fusionData, double speed) {
      NavigationData navigationData = Navigation::getData(fusionData, speed, true);
      hardwareProvider->move(navigationData);
    }

    static void moveToolhead(FusionData fusionData, double speed) {
      ToolheadData toolheadData = Navigation::getToolheadData(fusionData, speed);
      hardwareProvider->moveToolhead({ toolheadData.xAxisMotor, { false, 0 } });
    }

    static void moveWheels(FusionData fusionData, double speed) {
      WheelsData wheelsData = Navigation::getWheelsData(fusionData, speed);
      hardwareProvider->moveWheels(wheelsData);
    }

    static void moveMill(FusionData fusionData, double speed) {
      MillData millData = Navigation::getMillData(fusionData, speed);
      hardwareProvider->moveMill(millData);
    }

    // Stop functions
    static void stopRobot() {
      hardwareProvider->move({{true, 0}, {true, 0}});
    }

    static void stopToolhead() {
      hardwareProvider->moveToolhead({{true, 0}, {true, 0}});
    }

    static void stopWheels() {
      hardwareProvider->moveWheels({{true, 0}});
    }

    static void stopMill() {
      hardwareProvider->moveMill({{true, 0}});
    }
};
