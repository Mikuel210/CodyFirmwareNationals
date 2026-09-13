#pragma once
#include "Task.h"
#include "Cody.h"
#include "GPIO.h"

#pragma region Program Parameters

// Note: 0, 0 is the center of the start area

// General
#define SLOW_SPEED 25
#define START_X 4
#define ALIGN_SET_X 48
#define ALIGN_SET_Y -48
#define ALIGN_DISTANCE 500
#define ALIGN_SPEED 37
#define ALIGN_MS 2000

// Blocks
#define FIRST_GROUP_WALL_X_MM -185
#define BLOCK_GROUPS_INCREMENT -160
#define BLOCK_DISTANCE_START 83
#define BLOCK_DISTANCE_MOSAIC 50
#define BLOCK_HEIGHT 30
#define PICK_Y 20

// Toolhead
#define TOOLHEAD_UP 40
#define TOOLHEAD_DOWN 0
#define TOOLHEAD_PICK_START_X 0
#define TOOLHEAD_LEAVE_START_X 38

// Map
#define MOSAIC_X -545
#define MOSAIC_Y 874
#define BLOCKS_LINE_DETECT_Y 300

#pragma endregion


class Program {
  public:
    static void go() {
        Fusion::restart();

        blocks();
        items();
        cement();
    }

  private:
    static Task* moveTask;
    static Task* toolheadTask;
    static Task* millTask;
    static Task* wheelsTask;

    static void blocks() {
        // Home and align
        align(-40, -ALIGN_DISTANCE, 1000, 42, 150, 250);
        Cody::setYOrientation(ALIGN_SET_Y, 0);

        // Take vPicture
        std::vector<Color> colors = { BLUE, BLUE, BLUE, BLUE, BLUE, BLUE, YELLOW, YELLOW, YELLOW, YELLOW, YELLOW, YELLOW };

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


        Cody::addPathPoint(0, PICK_Y);
        Cody::followPathAsync()->await();
        Cody::rotateToAsync(-90)->await();

        // Pick/leave cycles

            // ========== PICK ==========
            toolheadTask = Cody::zUpAsync();
            align(ALIGN_DISTANCE, PICK_Y, 1000);
            Cody::setXOrientation(ALIGN_SET_X, -90);
            toolheadTask->await();

            // Pick blocks
            for (int i = 0; i < 4; i++) {
                if (colorCounts[i] == 0 || pickedCounts[i] == colorCounts[i] || totalPicked == 6) continue;

                // Detect line
                double pickX = FIRST_GROUP_WALL_X_MM + BLOCK_GROUPS_INCREMENT * i;
                Cody::moveAsync(pickX, PICK_Y, SLOW_SPEED, false, 50, 75, 100)->await();
                Cody::rotateToAsync(-90)->await();

                // Go to first block
                toolheadTask->await();
                Cody::rotateToAsync(-90)->await();

                for (int j = pickedCounts[i]; j < colorCounts[i]; j++)
                {
                     // if (j == 1 || j == 5) {
                        // Cody::addPathPoint(pickX - 60, PICK_Y);
                        // Cody::followPathAsync(SLOW_SPEED, false, 50, 75, 100)->await();
                        // Cody::rotateToAsync(-90)->await();
                    // }

                    // if (j == 3) {
                        // Cody::addPathPoint(pickX + 10, PICK_Y);
                        // Cody::followPathAsync(SLOW_SPEED, true, 50, 75, 100)->await();
                        // Cody::rotateToAsync(-90)->await();
                    // }

                    // if (j == 4) {
                        // Cody::addPathPoint(pickX + 20, PICK_Y);
                        // Cody::followPathAsync(SLOW_SPEED, false, 50, 75, 100)->await();
                        // Cody::rotateToAsync(-90)->await();
                    // }

                    if (j == 3) {
                        Cody::addPathPoint(pickX - 60, PICK_Y);
                        Cody::followPathAsync(SLOW_SPEED, false, 50, 75, 100)->await();
                        Cody::rotateToAsync(-90)->await();
                    }

                    totalPicked++;
                    pickedCounts[i]++;

                    // int position = 0;

                    // switch (j) {
                        // case 2: position = 1; break;
                        // case 3: position = 1; break;
                        // case 4: position = 2; break;
                        // case 5: position = 2; break;
                    // }


                    int position = 0;

                    switch (j) {
                        case 1: position = 1; break;
                        case 2: position = 2; break;
                        case 3: position = 2; break;
                        case 4: position = 1; break;
                    }

                    int nextPosition = 0;

                    switch (j + 1) {
                        case 1: nextPosition = 1; break;
                        case 2: nextPosition = 2; break;
                        case 3: nextPosition = 2; break;
                        case 4: nextPosition = 1; break;
                    }

                    if (position != 0 || j > 3) Cody::moveToolheadAsync(TOOLHEAD_PICK_START_X + BLOCK_DISTANCE_START * position, 0)->await();
                    wheelsTask = Cody::moveWheelsMsAsync(j == 5 ? 1000 : 2000, true);
                    Cody::homeZAsync()->await();
                    delay(250);
                    Cody::zUpAndMoveX(TOOLHEAD_PICK_START_X + BLOCK_DISTANCE_START * nextPosition)->await();

                    if (j == 2) {
                        toolheadTask = Cody::zUpAsync();
                        delay(700);
                    }

                    if (totalPicked == 6)
                        break;
                }
            }

            // ========== LEAVE ==========
            // Go to mosaic
            double mosaicX = MOSAIC_X + 15;

            align(ALIGN_DISTANCE, PICK_Y + 10);
            Cody::setXOrientation(ALIGN_SET_X, -90);
            toolheadTask = Cody::homeAsync();
            Cody::addPathPoint(-40, PICK_Y + 10);
            Cody::followPathAsync()->await();
            Cody::rotateToAsync(0)->await();
            align(-40, -ALIGN_DISTANCE, 1000);
            Cody::setYOrientation(ALIGN_SET_Y, 0);

            Cody::addPathPoint(-40, 450);
            Cody::addPathPoint(mosaicX + 120, 450);
            Cody::followPathAsync()->await();

            Cody::addPathPoint(mosaicX - 150, 450);
            moveTask = Cody::followPathAsync(20);
            Cody::detectColorAsync(BLACK, moveTask)->await();
            moveTask->await();
            Cody::setX(mosaicX + 90);
            delay(100);

            toolheadTask->await();

            Cody::addPathPoint(mosaicX, 450);
            Cody::followPathAsync(SLOW_SPEED)->await();
            Cody::rotateToAsync(0)->await();

            Cody::addPathPoint(mosaicX, MOSAIC_Y);
            toolheadTask = Cody::moveToolheadAsync(28, 0);
            moveTask = Cody::followPathAsync(20);
            Cody::detectColorAsync(BLACK, moveTask)->await();
            Cody::setY(715);
            toolheadTask = Cody::zUpAsync();
            moveTask->await();
            delay(100);
            Cody::rotateToAsync(0)->await();

            // Color
            int position = 2;
            Cody::addPathPoint(mosaicX, MOSAIC_Y + BLOCK_DISTANCE_MOSAIC * position);
            Cody::followPathAsync(25)->await();
            toolheadTask->await();

            double orientation = 0;
            Cody::rotateToAsync(orientation)->await();

            for (int meow = 0; meow < 3; meow++) {
                totalPicked--;
                leave(meow, totalPicked, true);
            }

            SensorData sensorData = Cody::dataProvider->getData();
            FusionData fusionData = Fusion::getData(sensorData);
            Cody::addPathPoint(fusionData.position.x, MOSAIC_Y + BLOCK_DISTANCE_MOSAIC * (position - 1));
            Cody::followPathAsync(25, true, 100, 250, 40, 25)->await();
            Cody::rotateToAsync(orientation)->await();

            for (int meow = 2; meow >= 0; meow--) {
                totalPicked--;
                leave(meow, totalPicked, meow != 2);
            }

        // Carry blocks
        Cody::addPathPoint(mosaicX, 400);
        Cody::followPathAsync(45, true)->await();
        delay(100);
        toolheadTask = Cody::homeAsync();
        Cody::rotateToAsync(-90)->await();
        align(ALIGN_DISTANCE, 400, 2500);
        Cody::setXOrientation(3, -90);
        toolheadTask->await();
    }

    static void items() {
        // Align

        // Carry first item
        // Cody::addPathPoint(0, 400);
        // Cody::followPathAsync()->await();

        // millTask = Cody::moveMillAsync(90);
        // Cody::rotateToAsync(-180)->await();
        // Cody::addPathPoint(0, 1000);
        // Cody::followPathAsync(45, true)->await();
        // millTask->await();

        // Cody::moveMillAsync(0)->await();
        // Cody::addPathPoint(25, 400);
        // Cody::followPathAsync()->await();

        // Carry 1st item

        toolheadTask = Cody::zUpAndMoveX(160);

        Cody::addPathPoint(-60, 400);
        Cody::addPathPoint(-60, 1050);
        Cody::followPathAsync(40)->await();

        Cody::addPathPoint(0, 850);
        Cody::addPathPoint(20, 675);
        Cody::followPathAsync(40, true, 50, 100)->await();

        // Carry second item
        Cody::addPathPoint(60, 1030);
        Cody::followPathAsync()->await();
        Cody::moveZMsAsync(100, 100, false)->await();
        toolheadTask = Cody::moveToolheadAsync(50, 0);

        Cody::addPathPoint(25, 1200);
        Cody::addPathPoint(25, 1650);
        Cody::followPathAsync(30, false, 100, 150)->await();

        Cody::rotateToAsync(-90)->await();
        Cody::addPathPoint(-150, 1650);
        Cody::followPathAsync()->await();
        toolheadTask = Cody::moveZMsAsync(300);

        // Carry 3rd item

        /*
        Cody::moveMillMsAsync(300, false);
        Cody::addPathPoint(0, 1600);
        Cody::addPathPoint(30, 1500);
        Cody::followPathAsync(30, true)->await();

        Cody::rotateToAsync(-30, 15)->await();

        Cody::addPathPoint(45, 1300);
        Cody::addPathPoint(45, 1200);
        Cody::addPathPoint(-65, 700);
        Cody::addPathPoint(-100, 500);
        Cody::addPathPoint(-150, 350);
        Cody::addPathPoint(-150, 150);
        Cody::followPathAsync(30, true)->await();
*/
        // Go to white
        Cody::addPathPoint(-100, 550);
        Cody::addPathPoint(-15, 725);
        Cody::addPathPoint(-15, 1250);
        Cody::addPathPoint(-30, 1400);
        Cody::addPathPoint(-30, 1525);
        moveTask = Cody::followPathAsync();

        delay(1000);
        Cody::moveMillMsAsync(250)->await();
        moveTask->await();

    }

    static void cement() {
        // Align
        Cody::addPathPoint(-150, 1650);
        Cody::addPathPoint(-150, 1700);
        Cody::followPathAsync()->await();
        Cody::rotateToAsync(-90)->await();

        align(ALIGN_DISTANCE, 1700, 1500);
        Cody::setXOrientation(ALIGN_SET_X, -90);

        toolheadTask = Cody::homeAsync();

        // Pick green
        double center = 1015;
        Cody::addPathPoint(-525, 1700);
        Cody::followPathAsync()->await();
        millTask = Cody::moveMillAsync(90);
        Cody::rotateToAsync(180)->await();
        Cody::addPathPoint(-525, 1975);
        Cody::followPathAsync(45, true)->await();
        millTask->await();
        Cody::moveMillAsync(0)->await();
        Cody::addPathPoint(-525, 1130);
        Cody::followPathAsync()->await();
        Cody::moveMillAsync(90)->await();
        Cody::addPathPoint(-525, 1700);
        Cody::followPathAsync(45, true)->await();

        toolheadTask->await();
        toolheadTask = Cody::moveToolheadAsync(160, 0);

        // Pick yellow
        Cody::rotateToAsync(-90)->await();
        Cody::addPathPoint(-780, 1700);
        Cody::followPathAsync()->await();
        Cody::rotateToAsync(180)->await();
        Cody::addPathPoint(-780, 1975);
        Cody::followPathAsync(45, true)->await();
        Cody::moveMillAsync(0)->await();
        Cody::addPathPoint(-780, 1800);
        Cody::addPathPoint(0, 1800);
        Cody::addPathPoint(0, 1030);
        Cody::addPathPoint(-400, 1030);
        Cody::followPathAsync()->await();
        Cody::moveMillAsync(90)->await();
        Cody::addPathPoint(0, 1030);
        Cody::followPathAsync(45, true)->await();

        toolheadTask->await();

        //// REGLA SORPRESA
        Cody::rotateToAsync(0)->await();
        Cody::addPathPoint(30, 1100);
        Cody::addPathPoint(30, 1160);
        Cody::followPathAsync()->await();

        Color color = Fusion::getData(Cody::dataProvider->getData()).color;

        Cody::addPathPoint(10, 1650);
        Cody::addPathPoint(10, 1700);
        Cody::followPathAsync()->await();
        Cody::rotateToAsync(-90)->await();

        toolheadTask = Cody::homeAsync();

        // Pick green
        int colorIndex = 0;

        switch (color) {
            case BLUE: colorIndex = 1; break;
            case GREEN: colorIndex = 2; break;
            case YELLOW: colorIndex = 3; break;
        }

        double position = -245 * colorIndex;

        Cody::addPathPoint(position, 1700);
        Cody::followPathAsync()->await();
        millTask = Cody::moveMillAsync(90);
        Cody::rotateToAsync(0)->await();
        Cody::addPathPoint(position, 1975);
        Cody::followPathAsync(45)->await();
        millTask->await();
    }

    // Helpers
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
    static void pick(int x, int z, bool force = false, bool isLast = false) {
        double xPosition = TOOLHEAD_PICK_START_X + BLOCK_DISTANCE_START * x;
        if (!isLast) pickLeave(xPosition, 2000, true, force, true);
        else pickLeave(xPosition, 1000, true, force, true);
    }

    static void leave(int x, int z, bool moveX) {
        double xPosition = TOOLHEAD_LEAVE_START_X + 66 * x;
        // pickLeave(xPosition, 1000, false, false);

        if (moveX) Cody::moveToolheadAsync(xPosition, 0)->await();
        Cody::moveZMsAsync(200, 100, false)->await();
        Cody::moveWheelsMsAsync(675, false)->await();
        Cody::moveZMsAsync(300)->await();
    }

    static void pickLeave(double xPosition, double wheelsMs, bool wheelsUp, bool force = false, bool asyncWheels = false) {
        if (xPosition != 0 || force) Cody::moveToolheadAsync(xPosition, 0)->await();
        if (asyncWheels) millTask = Cody::moveWheelsMsAsync(wheelsMs, wheelsUp);
        Cody::homeZAsync()->await();
        delay(250);
        if (!asyncWheels) Cody::moveWheelsMsAsync(wheelsMs, wheelsUp)->await();
        Cody::zUpAsync()->await();
        if (asyncWheels) millTask->await();
    }

    static void pause() {
      delay(999999999);
    }
};
