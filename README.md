# SUMOBOT - ESP32 + ZK-BM1 10A

Robot sumo dua motor yang dikendalikan radio FlySky melalui receiver FS-iA6B dan protokol i-BUS. Driver motor yang dipakai adalah ZK-BM1 dual-channel 10A.

## Hardware

- ESP32 development board
- ZK-BM1 dual-channel DC motor driver, 10A per kanal
- 2 motor DC
- Transmitter FlySky FS-i6/kompatibel dan receiver FS-iA6B
- Catu motor 3-18V yang sesuai dengan tegangan dan arus motor

## Wiring

| ZK-BM1 | ESP32 / perangkat | Fungsi |
| --- | --- | --- |
| IN1 | GPIO 4 | Motor kiri maju (PWM) |
| IN2 | GPIO 14 | Motor kiri mundur (PWM) |
| IN3 | GPIO 18 | Motor kanan maju (PWM) |
| IN4 | GPIO 19 | Motor kanan mundur (PWM) |
| GND (signal) | GND ESP32 | Ground bersama |
| OUT1, OUT2 | Motor kiri | Keluaran kanal A |
| OUT3, OUT4 | Motor kanan | Keluaran kanal B |
| VIN +, VIN - | Catu motor | Masukan daya motor |
| FS-iA6B i-BUS `SERVO` signal | Level shifter/pembagi tegangan → GPIO 16 | Data receiver |
| FS-iA6B VCC | 5V regulated | Daya receiver |
| FS-iA6B GND | GND ESP32 | Ground bersama |

ZK-BM1 tidak memakai pin `PWMA`, `PWMB`, atau `STBY` terpisah. Kecepatan dan arah dikendalikan langsung dengan PWM pada pasangan IN1/IN2 dan IN3/IN4.

> Jangan menyuplai ESP32 dari terminal VIN driver secara langsung. Satukan GND ESP32 dengan GND sinyal ZK-BM1. Modul tidak memiliki proteksi polaritas terbalik, jadi periksa VIN sebelum menyalakan daya.

> Jangan sambungkan signal i-BUS 5V langsung ke GPIO16 ESP32. Gunakan level shifter 5V-ke-3,3V atau pembagi tegangan, misalnya 1 kΩ dari signal receiver ke GPIO16 dan 2 kΩ dari GPIO16 ke GND. Pastikan orientasi pin signal, VCC, dan GND pada port `SERVO` benar.

## Kontrol motor

| Gerakan kanal | IN maju | IN mundur |
| --- | --- | --- |
| Maju | PWM | LOW |
| Mundur | LOW | PWM |
| Berhenti bebas (coast) | LOW | LOW |
| Rem elektrik | HIGH | HIGH |

Firmware menggunakan mode coast saat berhenti, PWM 2 kHz dengan resolusi 8-bit, dan mematikan kanal sesaat sebelum membalik arah. Nilai kecepatan adalah 0-255.

## Remote FS-iA6B

- CH1: kemudi kiri/kanan
- CH2: maju/mundur menggunakan stick yang kembali ke tengah
- CH5: turbo jika nilainya di atas 1500 µs (batas 255; normal 180)
- LED GPIO 2 berkedip saat menunggu dan menyala ketika data receiver diterima
- Motor otomatis berhenti jika frame i-BUS tidak diterima selama 100 ms

Hubungkan port i-BUS bertanda `SERVO` pada FS-iA6B, bukan port `SENS`, ke GPIO16 melalui penurun level. Firmware membaca frame pada 115200 baud dan memvalidasi checksum sebelum menggerakkan motor.

Bind receiver menggunakan bind plug sesuai petunjuk FS-iA6B. Pada transmitter, set endpoint CH1/CH2 sekitar 1000–2000 µs dan posisi netral sekitar 1500 µs. Jika stick yang dipilih tidak kembali ke tengah, aktifkan self-centering atau ubah `THROTTLE_CHANNEL` di firmware.

## Build dan upload

```bash
pio run --environment esp32dev
pio run --target upload --environment esp32dev
pio device monitor --baud 115200
```

Jika arah salah, tukar kedua kabel motor pada kanal terkait. Untuk beban mendekati 10A, gunakan kabel yang sesuai dan pendinginan yang memadai.

Referensi driver: [ZK-BM1 10A Motor Driver](https://docs.cirkitdesigner.com/component/86071865-61ac-4a34-bb10-e60bb542f0c1/zk-bm1-10a-motor-driver).
