#pragma once

#include <Arduino.h>
#include <SparkFun_TB6612.h>

class SumobotMotor {
 public:
  SumobotMotor();

  void begin();
  void drive(int left, int right);
  void stop();
  bool isStopped() const;

 private:
  Motor leftMotor_;
  Motor rightMotor_;
  bool stopped_ = false;
};
