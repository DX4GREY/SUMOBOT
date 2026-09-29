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
  void drive(int left, int right, bool bypassRamp = false);
  void update();
  void stop();
  void brake();
  bool isStopped() const;
  const SumobotMotorTelemetry& telemetry() const;

  void setAccelerationEnabled(bool enabled);
  bool isAccelerationEnabled() const;

 private:
  void applyOutputs(int left, int right);
  void refreshRamp();

  static int calculateStep(int current, int target, unsigned long elapsed);
  static float estimateRpm(int command);
  static float estimateTorqueNm(int command);

  Motor leftMotor_;
  Motor rightMotor_;
  bool stopped_ = true;
  bool rampActive_ = false;
  bool accelerationEnabled_ = true;
  int appliedLeft_ = 0;
  int appliedRight_ = 0;
  int rampTargetLeft_ = 0;
  int rampTargetRight_ = 0;
  unsigned long rampLastUpdatedAt_ = 0;
  SumobotMotorTelemetry telemetry_;
};
