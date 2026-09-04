#pragma once
#include "Task.h"
#include "Cody.h"

#pragma region Program Parameters

// Note: 0, 0 is the center of the start area

// General
#define START_X 4
#define ALIGN_SET_X 48
#define ALIGN_SET_Y -48
#define ALIGN_DISTANCE 500
#define ALIGN_SPEED 40
#define ALIGN_MS 2000

// Blocks
#define FIRST_GROUP_WALL_X_MM -170
#define BLOCK_GROUPS_INCREMENT -160
#define BLOCK_DISTANCE_START 64
#define BLOCK_DISTANCE_MOSAIC 50
#define BLOCK_HEIGHT 30

// Toolhead
#define TOOLHEAD_UP 40
#define TOOLHEAD_DOWN 0
#define TOOLHEAD_PICK_START_X 10
#define TOOLHEAD_LEAVE_START_X 20
#define COLOR_Y_OFFSET 87

// Map
#define MOSAIC_X -510
#define MOSAIC_Y 850
#define MOSAIC_X_OFFSET 30
#define BLOCKS_LINE_DETECT_Y 300
#define FIRST_LINE_X -275

#pragma endregion


class Program {
  public:
    static void go() {
      Fusion::restart();
      // Cody::homeAsync()->await();
      // Cody::moveToolheadAsync(20, 0)->await();

      blocks();
    }

  private:
    static Task* moveTask;
    static Task* toolheadTask;
    static Task* millTask;

    static void blocks() {
      // Home and align
      toolheadTask = Cody::homeAsync();

      align(0, -ALIGN_DISTANCE);
      Cody::setPosition(START_X, ALIGN_SET_Y, 0);

      // Go to mosaic
      Cody::addPathPoint(START_X, 400);
      Cody::addPathPoint(-200, 400);
      Cody::followPathAsync()->await();

      align(ALIGN_DISTANCE, 400);
      Cody::setXOrientation(ALIGN_SET_X, -90);

      toolheadTask->await();
      toolheadTask = Cody::zUpAsync();

      Cody::addPathPoint(MOSAIC_X, 400);
      Cody::addPathPoint(MOSAIC_X, MOSAIC_Y);
      Cody::followPathAsync()->await();

      // Take picture
      toolheadTask->await();

      // Serial2.println("GO");
      std::vector<Color> colors = {};
      // unsigned int startMillis = millis();

      // while (colors.size() < 12) {
        // if ((millis() - startMillis) > 2000) break;
        // if (!Serial2.available()) continue;
//
        // String message = Serial2.readStringUntil('\n');
        // colors.push_back(static_cast<Color>(message.toInt()));
      // }

      if (colors.size() < 12)
        colors = { BLUE, BLUE, BLUE, BLUE, BLUE, BLUE, YELLOW, YELLOW, YELLOW, YELLOW, YELLOW, YELLOW };

      // Initialize variables
      int yellow = std::count(colors.begin(), colors.end(), YELLOW);
      int blue   = std::count(colors.begin(), colors.end(), BLUE);
      int green  = std::count(colors.begin(), colors.end(), GREEN);
      int white  = std::count(colors.begin(), colors.end(), WHITE);

      int colorCounts[4] = { yellow, blue, green, white };
      int pickedCounts[4] = { 0, 0, 0, 0 };
      int totalPicked = 0;

      int mosaic[4][3] = {
        { colors[0], colors[1],  colors[2]  },
        { colors[3], colors[4],  colors[5]  },
        { colors[6], colors[7],  colors[8]  },
        { colors[9], colors[10], colors[11] },
      };

      bool positionsLeft[4][3] = {
        {false, false, false},
        {false, false, false},
        {false, false, false},
        {false, false, false}
    };

      // Carry blocks
      pickBatch(colorCounts, pickedCounts, &totalPicked);
      leaveBatch(pickedCounts, mosaic, positionsLeft, &totalPicked);
      pickBatch(colorCounts, pickedCounts, &totalPicked);
      leaveBatch(pickedCounts, mosaic, positionsLeft, &totalPicked);
    }

    static void pickBatch(int* colorCounts, int* pickedCounts, int* totalPicked) {
      // Go to start
      millTask = Cody::moveMillAsync(0);

      Cody::addPathPoint(MOSAIC_X, 400);
      Cody::addPathPoint(-50, 400);
      align(-50, -ALIGN_DISTANCE, 7000, 35, 100, 200);
      Cody::setYOrientation(ALIGN_SET_Y, 0);

      toolheadTask->await();
      toolheadTask = Cody::homeZAsync();

      Cody::addPathPoint(-50, 0);
      Cody::followPathAsync()->await();
      Cody::rotateToAsync(-90)->await();

      align(ALIGN_DISTANCE, 0);
      Cody::setXOrientation(ALIGN_SET_X, -90);

      pause();

      // Pick blocks
      for (int i = 0; i < 4; i++) {
        if (colorCounts[i] == 0 || pickedCounts[i] == colorCounts[i]) continue;
        toolheadTask = Cody::zUpAsync();

        // Detect line
        double pickX = FIRST_LINE_X + BLOCK_GROUPS_INCREMENT * i;
        Cody::moveAsync(pickX, 0)->await();

        // Go to first block
        toolheadTask = Cody::zUpAsync();
        toolheadTask->await();
        Cody::rotateToAsync(-90)->await();

        for (int j = pickedCounts[i]; j < colorCounts[i]; j++)
        {
          if (j == 3)
          {
            Cody::addPathPoint(pickX - BLOCK_DISTANCE_START, 0);
            Cody::followPathAsync()->await();
          }

          totalPicked++;
          pickedCounts[i]++;
          pick(j % 3, *totalPicked);

          if (*totalPicked == 6)
            return;
        }
      }
    }

    static void leaveBatch(int* pickedCounts, int (*mosaic)[3], bool (*positionsLeft)[3], int* totalPicked) {
      // Go to mosaic
      double leaveX = MOSAIC_X + MOSAIC_X_OFFSET;

      align(ALIGN_DISTANCE, 0);
      Cody::setXOrientation(ALIGN_SET_X, -90);

      Cody::addPathPoint(-25, 0);
      Cody::addPathPoint(-25, 400);
      Cody::addPathPoint(leaveX, 400);
      Cody::addPathPoint(leaveX, MOSAIC_Y);
      Cody::followPathAsync(45, false)->await();

      // Color
      for (int i = 3; i >= 0; i--) {
        if (pickedCounts[i] == 0) continue;

        Color color =
          (i == 0) ? YELLOW :
          (i == 1) ? BLUE :
          (i == 2) ? GREEN : WHITE;

        // Row
        for (int j = 0; j < 4; j++) {
          std::vector<int> positions;

          // Column
          for (int k = 0; k < 3; k++) {
            if (mosaic[j][k] != color || positionsLeft[j][k]) continue;
            positions.push_back(k);
          }

          if (positions.size() == 0) continue;

          double targetY = MOSAIC_Y + BLOCK_DISTANCE_MOSAIC * j;
          bool backwards = Fusion::getData(Cody::dataProvider->getPulses()).position.y > targetY;
          moveTask = Cody::moveAsync(leaveX, targetY, 40, backwards);
          toolheadTask = Cody::zUpAsync();//Cody::moveToolheadAsync(TOOLHEAD_LEAVE_START_X + BLOCK_DISTANCE_MOSAIC * positions[0], TOOLHEAD_UP);

          moveTask->await();
          toolheadTask->await();

          for (int k : positions)
          {
            totalPicked--;
            positionsLeft[j][k] = true;
            leave(k, *totalPicked);

            if (*totalPicked == 0) return;
          }
        }
      }
    }

    static void align(double x, double y, int ms = ALIGN_MS, double speed = ALIGN_SPEED, double lookaheadDistance = MOVEMENT_LOOKAHEAD,
      double transitionLookahead = TRANSITION_LOOKAHEAD, double decelerationMm = MOVEMENT_DECELERATION_MM) {

      Cody::addPathPoint(x, y);
      moveTask = Cody::followPathAsync(speed, true, lookaheadDistance, transitionLookahead, decelerationMm);
      delay(ms);

      delete moveTask->requestStop;
      moveTask->requestStop = new bool(true);
      moveTask->await();
    }

    // x = 0, z = 1 for first block
    static void pick(int x, int z) {
      double xPosition = TOOLHEAD_PICK_START_X + BLOCK_DISTANCE_START * x;
      pickLeave(xPosition, BLOCK_HEIGHT * z);
    }

    static void leave(int x, int z) {
      double xPosition = TOOLHEAD_LEAVE_START_X + BLOCK_DISTANCE_MOSAIC * x;
      pickLeave(xPosition, BLOCK_HEIGHT * z);
    }

    static void pickLeave(double xPosition, double zPosition) {
      Cody::homeZAsync()->await();
      Cody::zUpAsync()->await();
      Cody::moveToolheadAsync(xPosition, 0)->await();
      Cody::homeZAsync()->await();
      Cody::moveToolheadAsync(xPosition, 0)->await();
      Cody::moveWheelsAsync(zPosition)->await();
      Cody::homeZAsync()->await();
      Cody::moveToolheadAsync(xPosition, 0)->await();
    }

    static void pause()
    {
      delay(999999999);
    }
};
