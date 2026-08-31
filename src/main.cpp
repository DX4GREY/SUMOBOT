#include <Arduino.h>

// ================= PIN CONFIG =================
#define MOTOR_KIRI_IN1 4
#define MOTOR_KIRI_IN2 14
#define MOTOR_KANAN_IN3 18
#define MOTOR_KANAN_IN4 19
#define IBUS_RX_PIN 16
#define LED_PIN 2

// ================= MOTOR CONFIG =================
const int PWM_FREQ = 2000;
const int PWM_RESOLUTION = 8;
const int PWM_KIRI_MAJU = 0;
const int PWM_KIRI_MUNDUR = 1;
const int PWM_KANAN_MAJU = 2;
const int PWM_KANAN_MUNDUR = 3;

// ================= RECEIVER CONFIG =================
HardwareSerial IBusSerial(2);
const int IBUS_FRAME_SIZE = 32;
const int IBUS_CHANNEL_COUNT = 14;
const int STEERING_CHANNEL = 0;  // FS-i6 CH1
const int THROTTLE_CHANNEL = 1;  // FS-i6 CH2 (stick dengan pegas ke tengah)
const int TURBO_CHANNEL = 4;     // FS-i6 CH5
const int CHANNEL_MIN = 1000;
const int CHANNEL_CENTER = 1500;
const int CHANNEL_MAX = 2000;
const int CHANNEL_DEADZONE = 35;
const unsigned long RECEIVER_TIMEOUT_MS = 100;

// ================= SETTINGS =================
const int MAX_SPEED = 255;
const int NORMAL_SPEED = 180;
const float MIN_TURN_RATIO = 0.3f;

bool turboMode = false;
bool motorStopped = false;
bool receiverConnected = false;
int lastKiri = 0;
int lastKanan = 0;
uint16_t channels[IBUS_CHANNEL_COUNT] = {};
uint8_t ibusFrame[IBUS_FRAME_SIZE];
int ibusIndex = 0;
unsigned long lastReceiverFrame = 0;
unsigned long lastBlink = 0;
unsigned long lastDebug = 0;

// ================= MOTOR FUNCTIONS =================
void writeMotor(int majuChannel, int mundurChannel, int speed) {
  if (speed > 0) {
    ledcWrite(mundurChannel, 0);
    ledcWrite(majuChannel, speed);
  } else if (speed < 0) {
    ledcWrite(majuChannel, 0);
    ledcWrite(mundurChannel, -speed);
  } else {
    ledcWrite(majuChannel, 0);
    ledcWrite(mundurChannel, 0);
  }
}

void setMotor(int kiri, int kanan) {
  kiri = constrain(kiri, -255, 255);
  kanan = constrain(kanan, -255, 255);

  // Break-before-make saat arah berubah.
  if ((kiri > 0 && lastKiri < 0) || (kiri < 0 && lastKiri > 0)) {
    writeMotor(PWM_KIRI_MAJU, PWM_KIRI_MUNDUR, 0);
    delayMicroseconds(200);
  }
  if ((kanan > 0 && lastKanan < 0) || (kanan < 0 && lastKanan > 0)) {
    writeMotor(PWM_KANAN_MAJU, PWM_KANAN_MUNDUR, 0);
    delayMicroseconds(200);
  }

  writeMotor(PWM_KIRI_MAJU, PWM_KIRI_MUNDUR, kiri);
  writeMotor(PWM_KANAN_MAJU, PWM_KANAN_MUNDUR, kanan);
  lastKiri = kiri;
  lastKanan = kanan;
  motorStopped = (kiri == 0 && kanan == 0);
}

void berhenti() {
  if (motorStopped) return;

  writeMotor(PWM_KIRI_MAJU, PWM_KIRI_MUNDUR, 0);
  writeMotor(PWM_KANAN_MAJU, PWM_KANAN_MUNDUR, 0);
  lastKiri = 0;
  lastKanan = 0;
  motorStopped = true;
  Serial.println("[MOTOR] STOP");
}

// ================= I-BUS RECEIVER =================
bool decodeIBusFrame() {
  if (ibusFrame[0] != 0x20 || ibusFrame[1] != 0x40) return false;

  uint16_t checksum = 0xFFFF;
  for (int i = 0; i < IBUS_FRAME_SIZE - 2; i++) checksum -= ibusFrame[i];

  uint16_t receivedChecksum = ibusFrame[30] | (ibusFrame[31] << 8);
  if (checksum != receivedChecksum) return false;

  for (int i = 0; i < IBUS_CHANNEL_COUNT; i++) {
    channels[i] = ibusFrame[2 + i * 2] | (ibusFrame[3 + i * 2] << 8);
  }

  lastReceiverFrame = millis();
  receiverConnected = true;
  return true;
}

bool readIBus() {
  bool receivedFrame = false;

  while (IBusSerial.available()) {
    uint8_t value = IBusSerial.read();

    // Setiap frame valid dimulai dengan panjang 0x20 dan perintah kanal 0x40.
    if (ibusIndex == 0 && value != 0x20) continue;
    if (ibusIndex == 1 && value != 0x40) {
      ibusIndex = (value == 0x20) ? 1 : 0;
      if (ibusIndex == 1) ibusFrame[0] = 0x20;
      continue;
    }

    ibusFrame[ibusIndex++] = value;
    if (ibusIndex == IBUS_FRAME_SIZE) {
      receivedFrame |= decodeIBusFrame();
      ibusIndex = 0;
    }
  }

  return receivedFrame;
}

int channelToSpeed(uint16_t value, int speedLimit) {
  int centered = constrain((int)value, CHANNEL_MIN, CHANNEL_MAX) - CHANNEL_CENTER;
  if (abs(centered) <= CHANNEL_DEADZONE) return 0;

  if (centered > 0) {
    return map(centered, CHANNEL_DEADZONE, CHANNEL_MAX - CHANNEL_CENTER, 0, speedLimit);
  }
  return map(centered, CHANNEL_MIN - CHANNEL_CENTER, -CHANNEL_DEADZONE, -speedLimit, 0);
}

void processReceiver() {
  turboMode = channels[TURBO_CHANNEL] > 1500;
  int speedLimit = turboMode ? MAX_SPEED : NORMAL_SPEED;
  int throttle = channelToSpeed(channels[THROTTLE_CHANNEL], speedLimit);
  int steering = channelToSpeed(channels[STEERING_CHANNEL], speedLimit);

  int leftMotor;
  int rightMotor;

  if (abs(throttle) < 20) {
    leftMotor = steering;
    rightMotor = -steering;
  } else {
    float steer = (float)steering / speedLimit;
    if (steer > 0) {
      leftMotor = throttle;
      rightMotor = throttle * (1.0f - steer * (1.0f - MIN_TURN_RATIO));
    } else {
      rightMotor = throttle;
      leftMotor = throttle * (1.0f + steer * (1.0f - MIN_TURN_RATIO));
    }
  }

  setMotor(constrain(leftMotor, -speedLimit, speedLimit),
           constrain(rightMotor, -speedLimit, speedLimit));

  if (millis() - lastDebug >= 100) {
    lastDebug = millis();
    Serial.printf("[IBUS] CH1=%u CH2=%u CH5=%u | L=%d R=%d Turbo=%s\n",
                  channels[0], channels[1], channels[4],
                  leftMotor, rightMotor, turboMode ? "ON" : "OFF");
  }
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);
  Serial.println("\n===== ESP32 SUMOBOT (FS-iA6B i-BUS) =====");

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  ledcSetup(PWM_KIRI_MAJU, PWM_FREQ, PWM_RESOLUTION);
  ledcSetup(PWM_KIRI_MUNDUR, PWM_FREQ, PWM_RESOLUTION);
  ledcSetup(PWM_KANAN_MAJU, PWM_FREQ, PWM_RESOLUTION);
  ledcSetup(PWM_KANAN_MUNDUR, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(MOTOR_KIRI_IN1, PWM_KIRI_MAJU);
  ledcAttachPin(MOTOR_KIRI_IN2, PWM_KIRI_MUNDUR);
  ledcAttachPin(MOTOR_KANAN_IN3, PWM_KANAN_MAJU);
  ledcAttachPin(MOTOR_KANAN_IN4, PWM_KANAN_MUNDUR);

  IBusSerial.begin(115200, SERIAL_8N1, IBUS_RX_PIN, -1);
  berhenti();
  Serial.println("[INIT] Menunggu data i-BUS pada GPIO16...");
}

// ================= LOOP =================
void loop() {
  if (readIBus()) processReceiver();

  if (receiverConnected && millis() - lastReceiverFrame > RECEIVER_TIMEOUT_MS) {
    receiverConnected = false;
    berhenti();
    Serial.println("[FAILSAFE] Sinyal receiver hilang");
  }

  if (millis() - lastBlink >= 500) {
    lastBlink = millis();
    digitalWrite(LED_PIN, receiverConnected ? HIGH : !digitalRead(LED_PIN));
  }

  delay(1);
}
