#include <WiFi.h>
#include <WebServer.h>
#include <SparkFun_TB6612.h>
#include <Ps3Controller.h>

// ----- Pin -----
#define AIN1 4
#define AIN2 14
#define PWMA 5
#define BIN1 18
#define BIN2 19
#define PWMB 21
#define STBY 33

// LED indikator (optional, gunakan built-in LED GPIO2)
#define LED_PIN 2

// Motor objects (offset 1 atau -1 untuk arah)
Motor motorKiri(AIN1, AIN2, PWMA, 1, STBY);
Motor motorKanan(BIN1, BIN2, PWMB, 1, STBY);

int speedValue = 200;

WebServer server(80);

// ================= FUNGSI GERAK =================
void maju(int spd) {
  Serial.printf("[MOTOR] MAJU | Speed: %d\n", spd);
  motorKiri.drive(spd);
  motorKanan.drive(spd);
}
void mundur(int spd) {
  Serial.printf("[MOTOR] MUNDUR | Speed: %d\n", spd);
  motorKiri.drive(-spd);
  motorKanan.drive(-spd);
}
void belokKiri(int spd) {
  Serial.printf("[MOTOR] BELOK KIRI | Speed: %d\n", spd);
  motorKiri.drive(-spd);
  motorKanan.drive(spd);
}
void belokKanan(int spd) {
  Serial.printf("[MOTOR] BELOK KANAN | Speed: %d\n", spd);
  motorKiri.drive(spd);
  motorKanan.drive(-spd);
}
void berhenti() {
  Serial.println("[MOTOR] STOP (BRAKE)");
  motorKiri.brake();
  motorKanan.brake();
}

// ================= DEBUGGING FUNCTIONS =================
void checkHardware() {
  Serial.println("\n===== CHECK HARDWARE =====");
  Serial.printf("STBY Pin (%d) State: %s\n", STBY, digitalRead(STBY) ? "HIGH" : "LOW");
  Serial.printf("AIN1: %d, AIN2: %d, PWMA: %d\n", AIN1, AIN2, PWMA);
  Serial.printf("BIN1: %d, BIN2: %d, PWMB: %d\n", BIN1, BIN2, PWMB);
  // Baca tegangan VCC (pin 5V dari ESP32, tapi kita bisa baca hanya indikasi)
  // Baca pin PWM (setelah di-set ke 0 atau berhenti)
  Serial.println("Motor driver power should be connected to VM (motor voltage) and VCC (logic voltage 5V/3.3V)");
  Serial.println("-> Pastikan GND motor dan GND ESP32 tersambung (common ground)!");
  Serial.println("==========================\n");
}

void handleMotor() {
  if (server.hasArg("cmd")) {
    String cmd = server.arg("cmd");
    Serial.printf("[CMD] Received: %s\n", cmd.c_str());

    if (cmd == "maju") {
      maju(speedValue);
    } else if (cmd == "mundur") {
      mundur(speedValue);
    } else if (cmd == "kiri") {
      belokKiri(speedValue);
    } else if (cmd == "kanan") {
      belokKanan(speedValue);
    } else if (cmd == "stop") {
      berhenti();
    } else if (cmd == "speed") {
      if (server.hasArg("val")) {
        speedValue = server.arg("val").toInt();
        Serial.printf("[CMD] Set speed: %d\n", speedValue);
      }
    } else if (cmd == "debug") {
      Serial.println("\n==== DEBUG MANUAL ====");
      checkHardware();
      // checkWiFi();
      Serial.println("======================\n");
    }
    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "Bad Request");
  }
}

// Callback Function
void notify() {
  if (Ps3.event.button_down.up){
    maju(Ps3.event.button_down.r1 ? 200 : 255);
  } else if (Ps3.event.button_down.down){
    mundur(Ps3.event.button_down.r1 ? 200 : 255);
  } else if (Ps3.event.button_down.left){
    belokKiri(Ps3.event.button_down.r1 ? 200 : 255);
  } else if (Ps3.event.button_down.right){
    belokKanan(Ps3.event.button_down.r1 ? 200 : 255);
  }
  delay(10);
}

// On Connection function
void onConnect() {
  // Print to Serial Monitor
  Serial.println("[INIT] Ps3 Connected.");
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  while (!Serial);  // tunggu serial siap (khusus board native USB, opsional)
  delay(500);
  
  Serial.println("\n\n======== BOOTING ========");
  Serial.println("ESP32 TB6612FNG Debug Mode");

  // LED built-in untuk indikasi heartbeat
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  // Setup PS3 Controller
  Serial.println("[INIT] Setting up PS3 Controller...");
  Ps3.attach(notify);
  Serial.println("[INIT] Connecting PS3 Controller...");
  Ps3.attachOnConnect(onConnect);
  Ps3.begin("12:34:56:78:9A:BC");  // Ganti dengan MAC address PS3 controller Anda (unicast MAC)

  // Setup pin STBY
  pinMode(STBY, OUTPUT);
  digitalWrite(STBY, HIGH);
  Serial.println("[INIT] STBY set HIGH -> Driver enabled");

  // Matikan motor awal (brake)
  berhenti();
}

// ================= LOOP =================
unsigned long lastDebugTime = 0;

void loop() {
  server.handleClient();

  // LED berkedip indikasi server berjalan
  static unsigned long lastBlink = 0;
  if (millis() - lastBlink > 1000) {
    lastBlink = millis();
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));  // toggle
  }

  // Debug periodik setiap 10 detik
  if (millis() - lastDebugTime > 10000) {
    lastDebugTime = millis();
    Serial.println("\n[PERIODIC DEBUG]");
    Serial.printf("  STBY=%d, Speed=%d\n", digitalRead(STBY), speedValue);
    Serial.printf("  AIN1=%d AIN2=%d PWMA=analogWrite(%d)\n", digitalRead(AIN1), digitalRead(AIN2), PWMA);
    Serial.printf("  BIN1=%d BIN2=%d PWMB=analogWrite(%d)\n", digitalRead(BIN1), digitalRead(BIN2), PWMB);
  }
}