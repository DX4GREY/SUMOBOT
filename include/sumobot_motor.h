#pragma once

#include <Arduino.h>
#include <SparkFun_TB6612.h>

struct SumobotMotorTelemetry {
  int leftCommand = 0;
  int rightCommand = 0;
  float leftRpm = 0.0f;
  float rightRpm = 0.0f;
  float leftTorqueNm = 0.0f;
  float rightTorqueNm = 0.0f;
};

class SumobotMotor {
 public:
  SumobotMotor();

  void begin();
  void drive(int left, int right);
  void update();
  void stop();
  bool isStopped() const;
  const SumobotMotorTelemetry& telemetry() const;

 private:
  static bool shouldAccelerate(int left, int right);
  void applyOutputs(int left, int right);
  void refreshRamp();

  static float estimateRpm(int command);
  static float estimateTorqueNm(int command);

  Motor leftMotor_;
  Motor rightMotor_;
  bool stopped_ = false;
  bool rampActive_ = false;
  int appliedLeft_ = 0;
  int appliedRight_ = 0;
  int rampStartLeft_ = 0;
  int rampStartRight_ = 0;
  int rampTargetLeft_ = 0;
  int rampTargetRight_ = 0;
  unsigned long rampStartedAt_ = 0;
  SumobotMotorTelemetry telemetry_;
};
