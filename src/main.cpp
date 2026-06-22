#include <Arduino.h>
#include <Bluepad32.h>
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
const unsigned long CONTROLLER_TIMEOUT_MS = 250;

bool turboMode = false;
bool motorStopped = false;

unsigned long lastBlink = 0;
unsigned long lastControllerData = 0;
ControllerPtr myControllers[BP32_MAX_GAMEPADS];

int accelerate(int v1, int v2, int t){
  return v1 + ((v2 - v1) * t) / 100;
}

// ================= MOTOR FUNCTIONS =================
void setMotor(int kiri, int kanan) {
  kiri = constrain(kiri, -255, 255);
  kanan = constrain(kanan, -255, 255);

  motorKiri.drive(kiri);
  motorKanan.drive(kanan);
  motorStopped = (kiri == 0 && kanan == 0);

  Serial.printf("[MOTOR] LEFT=%d RIGHT=%d\n", kiri, kanan);
}

void berhenti() {
  if (motorStopped) return;

  motorKiri.drive(0);
  motorKanan.drive(0);
  motorStopped = true;

  Serial.println("[MOTOR] STOP");
}

bool hasConnectedController() {
  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
    if (myControllers[i] && myControllers[i]->isConnected()) {
      return true;
    }
  }

  return false;
}

// ================= BLUEPAD32 CALLBACKS =================
// Dipanggil otomatis saat Controller terhubung
void onConnectedController(ControllerPtr ctl) {
  bool foundEmptySlot = false;
  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
    if (myControllers[i] == nullptr) {
      myControllers[i] = ctl;
      foundEmptySlot = true;
      Serial.printf("[BP32] Controller connected at slot [%d]\n", i);
      ControllerProperties properties = ctl->getProperties();
      Serial.printf("[BP32] Model=%s VID=0x%04x PID=0x%04x\n",
                    ctl->getModelName().c_str(),
                    properties.vendor_id,
                    properties.product_id);
      digitalWrite(LED_PIN, HIGH);
      break;
    }
  }
  if (!foundEmptySlot) {
    Serial.println("[BP32] Warning: Max controllers reached, cannot connect this one.");
  }
}

// Dipanggil otomatis saat Controller terputus
void onDisconnectedController(ControllerPtr ctl) {
  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
    if (myControllers[i] == ctl) {
      myControllers[i] = nullptr;
      Serial.printf("[BP32] Controller disconnected from slot [%d]\n", i);
      digitalWrite(LED_PIN, LOW);
      berhenti();
      break;
    }
  }
}

// ================= PROCESSING GAMEPAD DATA =================
void processGamepad(ControllerPtr ctl) {
  // Turbo Mode pakai tombol R1 (di BP32 namanya: buttons() & BUTTON_SHOULDER_R)
  turboMode = (ctl->buttons() & BUTTON_SHOULDER_R);
  int speedLimit = turboMode ? MAX_SPEED : NORMAL_SPEED;
  int deadAnalog = 40; // Nilai stick di bawah ini dianggap 0 untuk menghindari noise

  // Bluepad32 membaca stick dari -511 sampai 512.
  // axisY() bernilai negatif saat stick ke atas, axisX() positif saat ke kanan.
  int ly = ctl->axisY() == 0 ? 0 : ctl->axisY() > deadAnalog ? 512 : ctl->axisY() < -deadAnalog ? -512 : 0;
  int lx = ctl->axisRX() == 0 ? 0 : ctl->axisRX() > deadAnalog ? 512 : ctl->axisRX() < -deadAnalog ? -512 : 0; // Gunakan RX untuk steering horizontal

  // Matikan jika di dalam Deadzone (-15 sampai 15)
  if (abs(ly) < DEADZONE) ly = 0;
  if (abs(lx) < DEADZONE) lx = 0;

  // Mapping nilai stick menjadi target speed robot.
  int throttle = map(-ly, -512, 512, -speedLimit, speedLimit);
  int steering = map(-lx, -512, 512, -speedLimit, speedLimit);

  int leftMotor;
  int rightMotor;

  // Jika hampir diam, boleh pivot turn
  if (abs(throttle) < 20) {
      leftMotor = steering;
      rightMotor = -steering;
  }
  else {
      float steer = (float)steering / speedLimit;

      const float MIN_TURN_RATIO = 0.3f; // roda dalam minimal 30%

      if (steer > 0) {
          // Belok kanan
          leftMotor = throttle;

          float ratio =
              1.0f - (steer * (1.0f - MIN_TURN_RATIO));

          rightMotor = throttle * ratio;
      }
      else {
          // Belok kiri
          rightMotor = throttle;

          float ratio =
              1.0f + (steer * (1.0f - MIN_TURN_RATIO));

          leftMotor = throttle * ratio;
      }
  }

  // Safety
  leftMotor = constrain(leftMotor, -speedLimit, speedLimit);
  rightMotor = constrain(rightMotor, -speedLimit, speedLimit);

  // Nilai potong agar motor tidak berdengung
  if (abs(leftMotor) < 10) leftMotor = 0;
  if (abs(rightMotor) < 10) rightMotor = 0;

  // Jalankan Motor
  setMotor(leftMotor, rightMotor);
  // Debug Data Stick
  Serial.printf("[BP32] LX=%d LY=%d | Turbo=%s\n", lx, ly, turboMode ? "ON" : "OFF");
}

void processControllers() {
  bool anyControllerActive = false;

  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
    ControllerPtr myController = myControllers[i];

    if (myController && myController->isConnected() && myController->hasData()) {
      if (myController->isGamepad()) {
        processGamepad(myController);
        anyControllerActive = true;
        lastControllerData = millis();
        break; // Kita pakai controller pertama yang aktif saja
      }

      Serial.println("[BP32] Unsupported controller");
    }
  }

  if (!anyControllerActive && !hasConnectedController()) {
    berhenti();
  }
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);
  Serial.println("\n===== ESP32 RC ROBOT (BLUEPAD32) =====");

  // Setup I/O Pin
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  pinMode(STBY, OUTPUT);
  digitalWrite(STBY, HIGH);

  berhenti();

  // Setup Bluepad32
  String fv = BP32.firmwareVersion();
  Serial.printf("[INIT] Bluepad32 Firmware: %s\n", fv.c_str());
  const uint8_t* addr = BP32.localBdAddress();
  Serial.printf("[INIT] BD Addr: %02X:%02X:%02X:%02X:%02X:%02X\n",
                addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);

  // Daftarkan fungsi callback
  BP32.setup(&onConnectedController, &onDisconnectedController);
  BP32.enableVirtualDevice(false);
  
  // Lupakan device lama (opsional, bawaan Bluepad32 untuk reset pairing)
  BP32.forgetBluetoothKeys(); 

  Serial.println("[INIT] Waiting for controller... (Press SHARE + PS until blinking fast)");
}

// ================= LOOP =================
void loop() {
  // WAJIB dipanggil di awal loop agar Bluepad32 memproses data Bluetooth
  bool hasLatestData = BP32.update();

  if (hasLatestData) {
    processControllers();
  }

  if (hasConnectedController() && millis() - lastControllerData > CONTROLLER_TIMEOUT_MS) {
    berhenti();
  }

  // Heartbeat LED (Hanya berkedip lambat jika controller belum connect)
  if (millis() - lastBlink > 500) {
    lastBlink = millis();
    
    // Jika tidak ada controller terkoneksi, buat led berkedip
    bool connected = hasConnectedController();
    
    if(!connected) {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    } else {
      digitalWrite(LED_PIN, HIGH); // Diam/Menyala penuh saat connect
    }
  }

  delay(1);
}
