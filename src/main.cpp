#include "sumobot_app.h"

SumobotApp robot;

void setup() {
  robot.begin();
}

void loop() {
  robot.update();
}
