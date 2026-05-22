#include <Ps3Controller.h>
#include <SparkFun_TB6612.h>

// ================= PIN CONFIG =================
#define AIN1 4
#define AIN2 14
#define PWMA 5

#define BIN1 18
#define BIN2 19
#define PWMB 21

#define STBY 33
#define LED_PIN 2

// ================= MOTOR CONFIG =================
Motor motorKiri(AIN1, AIN2, PWMA, 1, STBY);
Motor motorKanan(BIN1, BIN2, PWMB, 1, STBY);

// ================= SETTINGS =================
const int DEADZONE = 15;
const int MAX_SPEED = 255;
const int NORMAL_SPEED = 180;

bool turboMode = false;

unsigned long lastBlink = 0;
unsigned long lastPacket = 0;

// ================= MOTOR FUNCTIONS =================
void setMotor(int kiri, int kanan) {
  kiri = constrain(kiri, -255, 255);
  kanan = constrain(kanan, -255, 255);

  motorKiri.drive(kiri);
  motorKanan.drive(kanan);

  Serial.printf(
    "[MOTOR] LEFT=%d RIGHT=%d\n",
    kiri,
    kanan
  );
}

void berhenti() {
  motorKiri.drive(0);
  motorKanan.drive(0);

  Serial.println("[MOTOR] STOP");
}

// ================= PS3 CONNECT =================
void onConnect() {
  Serial.println("\n[PS3] Controller Connected!");
  digitalWrite(LED_PIN, HIGH);
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  Serial.println("\n===== ESP32 RC ROBOT =====");

  // LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Driver Enable
  pinMode(STBY, OUTPUT);
  digitalWrite(STBY, HIGH);

  // Stop motor awal
  berhenti();

  // PS3 Init
  Ps3.attachOnConnect(onConnect);

  /*
    Ganti MAC ini sesuai MAC ESP32 yang ingin dipair ke PS3
  */
  Ps3.begin("12:34:56:78:9A:BC");

  Serial.println("[INIT] Waiting PS3 connection...");
}

// ================= MAIN CONTROL =================
void handlePs3() {

  // Fail-safe
  if (!Ps3.isConnected()) {
    berhenti();
    return;
  }

  // Update packet timer
  lastPacket = millis();

  // Turbo Mode
  turboMode = Ps3.data.button.r1;

  int speedLimit = turboMode ? MAX_SPEED : NORMAL_SPEED;

  // Analog Stick
  int ly = Ps3.data.analog.stick.ly;
  int lx = Ps3.data.analog.stick.lx;

  // Deadzone
  if (abs(ly) < DEADZONE)
    ly = 0;

  if (abs(lx) < DEADZONE)
    lx = 0;

  // Mapping
  int throttle = map(ly, -128, 127, speedLimit, -speedLimit);
  int steering = map(lx, -128, 127, -speedLimit, speedLimit);

  // Differential Steering
  int leftMotor = throttle + steering;
  int rightMotor = throttle - steering;

  // Constraint
  leftMotor = constrain(leftMotor, -255, 255);
  rightMotor = constrain(rightMotor, -255, 255);

  // Smooth curve
  if (abs(leftMotor) < 10)
    leftMotor = 0;

  if (abs(rightMotor) < 10)
    rightMotor = 0;

  // Apply Motor
  setMotor(leftMotor, rightMotor);

  // Debug
  Serial.printf(
    "[PS3] LX=%d LY=%d | Turbo=%s\n",
    lx,
    ly,
    turboMode ? "ON" : "OFF"
  );
}

// ================= LOOP =================
void loop() {

  // Handle controller
  handlePs3();

  // Heartbeat LED
  if (millis() - lastBlink > 500) {
    lastBlink = millis();

    digitalWrite(
      LED_PIN,
      !digitalRead(LED_PIN)
    );
  }

  // Emergency timeout
  if (millis() - lastPacket > 1000) {
    berhenti();
  }

  delay(10);
}
