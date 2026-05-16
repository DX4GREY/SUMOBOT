#include <WiFi.h>
#include <WebServer.h>
#include <SparkFun_TB6612.h>
#include <Ps3Controller.h>

// ----- Konfigurasi SoftAP -----
const char* ssid = "RobotController";
const char* password = "12345678";

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

void checkWiFi() {
  Serial.printf("\n[WIFI] Access Point SSID: %s\n", ssid);
  Serial.printf("[WIFI] IP Address: %s\n", WiFi.softAPIP().toString().c_str());
  Serial.printf("[WIFI] Client count: %d\n", WiFi.softAPgetStationNum());
}

// ================= WEBSERVER HANDLERS =================
String getHTML() {
  String html = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
    <title>Robot Control</title>
    <style>
      :root {
        --bg: #0f172a;
        --surface: #1e293b;
        --primary: #3b82f6;
        --danger: #ef4444;
        --success: #22c55e;
        --warning: #eab308;
        --gray: #64748b;
        --text: #f1f5f9;
        --text-secondary: #94a3b8;
      }
      * { margin: 0; padding: 0; box-sizing: border-box; }
      body {
        font-family: 'Segoe UI', system-ui, -apple-system, sans-serif;
        background: var(--bg);
        color: var(--text);
        display: flex;
        flex-direction: column;
        align-items: center;
        justify-content: center;
        min-height: 100vh;
        padding: 20px;
        user-select: none;
        -webkit-tap-highlight-color: transparent;
      }
      .header {
        text-align: center;
        margin-bottom: 20px;
      }
      .header h1 {
        font-size: 2rem;
        font-weight: 700;
        letter-spacing: 1px;
        background: linear-gradient(135deg, var(--primary), #8b5cf6);
        -webkit-background-clip: text;
        -webkit-text-fill-color: transparent;
        margin-bottom: 8px;
      }
      .header .wifi-badge {
        display: inline-flex;
        align-items: center;
        gap: 6px;
        background: var(--surface);
        padding: 6px 16px;
        border-radius: 20px;
        font-size: 0.85rem;
        color: var(--text-secondary);
      }
      .wifi-badge .dot {
        width: 10px;
        height: 10px;
        border-radius: 50%;
        background: #22c55e;
        box-shadow: 0 0 6px #22c55e;
        display: inline-block;
      }
      .controller {
        display: grid;
        grid-template-columns: 100px 100px 100px;
        grid-template-rows: auto;
        gap: 12px;
        justify-content: center;
        align-items: center;
        margin: 20px 0;
      }
      .btn {
        position: relative;
        width: 100%;
        aspect-ratio: 1/1;
        border: none;
        border-radius: 30px;
        font-size: 2.4rem;
        font-weight: bold;
        color: white;
        background: var(--surface);
        box-shadow: 0 6px 0 #0f172a, 0 8px 20px rgba(0,0,0,0.4);
        cursor: pointer;
        transition: transform 0.05s ease, box-shadow 0.05s ease;
        display: flex;
        align-items: center;
        justify-content: center;
        line-height: 1;
      }
      .btn:active {
        transform: translateY(4px);
        box-shadow: 0 2px 0 #0f172a, 0 4px 12px rgba(0,0,0,0.4);
      }
      .btn-maju {
        grid-column: 2;
        grid-row: 1;
        background: var(--success);
        box-shadow: 0 6px 0 #15803d, 0 8px 20px rgba(34,197,94,0.4);
      }
      .btn-kiri {
        grid-column: 1;
        grid-row: 2;
        background: var(--warning);
        color: #1e293b;
        box-shadow: 0 6px 0 #a16207, 0 8px 20px rgba(234,179,8,0.4);
      }
      .btn-stop {
        grid-column: 2;
        grid-row: 2;
        aspect-ratio: 1/1;
        width: 100%;
        background: var(--danger);
        box-shadow: 0 6px 0 #b91c1c, 0 8px 20px rgba(239,68,68,0.5);
        font-size: 1.8rem;
      }
      .btn-kanan {
        grid-column: 3;
        grid-row: 2;
        background: var(--primary);
        box-shadow: 0 6px 0 #1d4ed8, 0 8px 20px rgba(59,130,246,0.5);
      }
      .btn-mundur {
        grid-column: 2;
        grid-row: 3;
        background: #8b5cf6;
        box-shadow: 0 6px 0 #6d28d9, 0 8px 20px rgba(139,92,246,0.5);
      }
      .speed-panel {
        margin: 30px 0 20px;
        width: 100%;
        max-width: 320px;
        background: var(--surface);
        border-radius: 24px;
        padding: 20px;
        text-align: center;
      }
      .speed-panel label {
        font-size: 0.9rem;
        color: var(--text-secondary);
        display: block;
        margin-bottom: 8px;
      }
      .speed-value {
        font-size: 3.5rem;
        font-weight: 800;
        color: white;
        margin-bottom: 10px;
      }
      input[type=range] {
        width: 100%;
        appearance: none;
        height: 8px;
        border-radius: 4px;
        background: linear-gradient(90deg, var(--success), var(--warning), var(--danger));
        outline: none;
        cursor: pointer;
      }
      input[type=range]::-webkit-slider-thumb {
        appearance: none;
        width: 28px;
        height: 28px;
        border-radius: 50%;
        background: white;
        box-shadow: 0 2px 8px rgba(0,0,0,0.4);
        border: 2px solid var(--primary);
      }
      .debug-btn {
        margin-top: 20px;
        background: transparent;
        border: 2px solid var(--gray);
        color: var(--text-secondary);
        padding: 12px 28px;
        border-radius: 30px;
        font-size: 0.95rem;
        cursor: pointer;
        transition: all 0.2s;
        letter-spacing: 0.5px;
      }
      .debug-btn:active {
        background: var(--gray);
        color: white;
      }
      .status-bar {
        margin-top: 15px;
        font-size: 0.85rem;
        color: var(--text-secondary);
        display: flex;
        gap: 20px;
        align-items: center;
      }
      .status-dot {
        width: 8px;
        height: 8px;
        border-radius: 50%;
        display: inline-block;
        margin-right: 5px;
      }
    </style>
  </head>
  <body>
    <div class="header">
      <h1>⚡ TB6612FNG</h1>
      <div class="wifi-badge">
        <span class="dot"></span> RobotController
      </div>
    </div>

    <div class="controller">
      <button class="btn btn-maju" 
              ontouchstart="sendCmd('maju')" ontouchend="sendCmd('stop')"
              onmousedown="sendCmd('maju')" onmouseup="sendCmd('stop')">▲</button>
      <button class="btn btn-kiri" 
              ontouchstart="sendCmd('kiri')" ontouchend="sendCmd('stop')"
              onmousedown="sendCmd('kiri')" onmouseup="sendCmd('stop')">◀</button>
      <button class="btn btn-stop" onclick="sendCmd('stop')">■</button>
      <button class="btn btn-kanan" 
              ontouchstart="sendCmd('kanan')" ontouchend="sendCmd('stop')"
              onmousedown="sendCmd('kanan')" onmouseup="sendCmd('stop')">▶</button>
      <button class="btn btn-mundur" 
              ontouchstart="sendCmd('mundur')" ontouchend="sendCmd('stop')"
              onmousedown="sendCmd('mundur')" onmouseup="sendCmd('stop')">▼</button>
    </div>

    <div class="speed-panel">
      <label>KECEPATAN</label>
      <div class="speed-value" id="speedDisplay">200</div>
      <input type="range" min="50" max="255" value="200" id="speedSlider" 
             oninput="updateSpeed(this.value)">
    </div>

    <button class="debug-btn" onclick="sendCmd('debug')">🔍 Debug Serial</button>
    <div class="status-bar">
      <span id="lastCmd">Stop</span>
    </div>

    <script>
      const speedDisplay = document.getElementById('speedDisplay');
      const lastCmdSpan = document.getElementById('lastCmd');

      function sendCmd(cmd) {
        fetch('/motor?cmd=' + cmd)
          .then(() => {
            if(cmd !== 'speed' && cmd !== 'debug') {
              lastCmdSpan.textContent = cmd.toUpperCase();
            }
          })
          .catch(() => lastCmdSpan.textContent = 'ERROR');
      }

      function updateSpeed(val) {
        speedDisplay.textContent = val;
        fetch('/motor?cmd=speed&val=' + val);
      }

      // Set tampilan awal
      updateSpeed(200);
      lastCmdSpan.textContent = 'SIAP';
    </script>
  </body>
  </html>
  )rawliteral";
  return html;
}

void handleRoot() {
  server.send(200, "text/html", getHTML());
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
      checkWiFi();
      Serial.println("======================\n");
    }
    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "Bad Request");
  }
}

// Callback Function
void notify() {

  // // Shoulder & Trigger button changes for RGB LED
  // // Set RGB values based upon analog button values
  // if (abs(Ps3.event.analog_changed.button.l1)) {
  //   // Left Shoulder - Red
  //   redPWM = int(Ps3.data.analog.button.l1);
  // }

  // if (abs(Ps3.event.analog_changed.button.l2)) {
  //   // Left Trigger - Green
  //   greenPWM = int(Ps3.data.analog.button.l2);
  // }

  // if (abs(Ps3.event.analog_changed.button.r2)) {
  //   // Right Trigger - Blue
  //   bluePWM = int(Ps3.data.analog.button.r2);
  // }

  // // Right Shoulder button turns on all LED segments full, for white
  // if (abs(Ps3.event.analog_changed.button.r1)) {
  //   // Right Shoulder - White
  //   redPWM = 255;
  //   greenPWM = 255;
  //   bluePWM = 255;
  // }

  // // Write LED values to RGB LED
  // ledcWrite(redChannel, redPWM);
  // ledcWrite(greenChannel, greenPWM);
  // ledcWrite(blueChannel, bluePWM);

  // // Print to Serial Monitor
  // Serial.print("R = ");
  // Serial.print(redPWM);
  // Serial.print(" - G = ");
  // Serial.print(greenPWM);
  // Serial.print(" - B = ");
  // Serial.println(bluePWM);
  if (Ps3.event.button_down.up) {
    maju(Ps3.event.button_down.r1 ? 200 : 255);
  } else if (Ps3.event.button_down.down)
  {
    mundur(Ps3.event.button_down.r1 ? 200 : 255);
  } else if (Ps3.event.button_down.left)
  {
    belokKiri(Ps3.event.button_down.r1 ? 200 : 255);
  } else if (Ps3.event.button_down.right)
  {
    belokKanan(Ps3.event.button_down.r1 ? 200 : 255);
  }
  
  
  

  delay(10);
}

// On Connection function
void onConnect() {
  // Print to Serial Monitor
  Serial.println("Connected.");
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  // Define Callback Function
  Ps3.attach(notify);
  // Define On Connection Function
  Ps3.attachOnConnect(onConnect);
  // Emulate console as specific MAC address (change as required)
  Ps3.begin("38:4F:F0:93:61:38");

  while (!Serial);  // tunggu serial siap (khusus board native USB, opsional)
  delay(500);
  
  Serial.println("\n\n======== BOOTING ========");
  Serial.println("ESP32 TB6612FNG Debug Mode");

  // LED built-in untuk indikasi heartbeat
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  // Setup pin STBY
  pinMode(STBY, OUTPUT);
  digitalWrite(STBY, HIGH);
  Serial.println("[INIT] STBY set HIGH -> Driver enabled");

  // Matikan motor awal (brake)
  berhenti();

  // Cek hardware awal
  checkHardware();

  // Buat Access Point
  Serial.printf("[WIFI] Membuat Access Point: %s ...\n", ssid);
  if (WiFi.softAP(ssid, password)) {
    checkWiFi();
  } else {
    Serial.println("[ERROR] Gagal membuat Access Point! Restart ESP32.");
  }

  // Rute server
  server.on("/", handleRoot);
  server.on("/motor", handleMotor);
  server.begin();
  Serial.println("[SERVER] Web server dimulai pada port 80");

  // Jantung
  Serial.println("======== BOOT SELESAI ========");
  Serial.println("Sambungkan HP/laptop ke WiFi 'RobotController' lalu buka http://192.168.4.1");
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
    Serial.printf("  WiFi clients: %d\n", WiFi.softAPgetStationNum());
  }
}