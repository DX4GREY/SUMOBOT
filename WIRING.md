# Full Wiring SUMOBOT

Konfigurasi ini menggunakan:

- ESP32 DevKit
- Driver motor dual-channel ZK-BM1 10A
- Receiver FlySky FS-iA6B melalui i-BUS
- Dua motor DC
- Baterai motor yang sesuai dengan spesifikasi motor dan ZK-BM1
- Regulator/BEC 5V untuk ESP32 dan receiver

## Diagram blok

```text
                         +--------------------+
                         |   FlySky FS-iA6B   |
                  5V --->| VCC                |
Common GND --------------| GND                |
                         | SERVO/i-BUS SIGNAL |---[1k]---+---> ESP32 GPIO16
                         +--------------------+          |
                                                        [2k]
                                                         |
Common GND ----------------------------------------------+

  Baterai + ---- Sekering ---- Sakelar ----+----> ZK-BM1 VIN+
                                           |
                                           +----> Regulator/BEC input+

  Baterai - -------------------------------+----> ZK-BM1 VIN-
                                           |
                                           +----> Regulator/BEC input-

  Regulator/BEC 5V+ ----------------------------> ESP32 5V/VIN
                       +------------------------> FS-iA6B VCC

  Regulator/BEC GND ----------------------------> ESP32 GND
                       +------------------------> FS-iA6B GND
                       +------------------------> ZK-BM1 signal GND

  ESP32 GPIO4  ---------------------------------> ZK-BM1 IN1
  ESP32 GPIO14 ---------------------------------> ZK-BM1 IN2
  ESP32 GPIO18 ---------------------------------> ZK-BM1 IN3
  ESP32 GPIO19 ---------------------------------> ZK-BM1 IN4

  ZK-BM1 OUT1/OUT2 -----------------------------> Motor kiri
  ZK-BM1 OUT3/OUT4 -----------------------------> Motor kanan
```

## 1. Jalur daya

| Dari | Ke | Keterangan |
| --- | --- | --- |
| Baterai `+` | Sekering → sakelar utama | Pasang sekering sedekat mungkin dengan baterai |
| Keluaran sakelar `+` | ZK-BM1 `VIN+` | Daya utama motor |
| Keluaran sakelar `+` | Input `+` regulator/BEC | Sumber 5V elektronik kontrol |
| Baterai `-` | ZK-BM1 `VIN-` | Negatif daya motor |
| Baterai `-` | Input `-` regulator/BEC | Ground regulator |
| Output regulator 5V | ESP32 `5V`/`VIN` | Jangan masuk ke pin 3V3 |
| Output regulator 5V | FS-iA6B `VCC` | Receiver membutuhkan sekitar 4,0–6,5V |
| Output regulator GND | ESP32 GND dan FS-iA6B GND | Ground bersama |

Gunakan regulator/BEC 5V yang stabil. Jangan menghubungkan tegangan baterai motor langsung ke pin 5V, VIN, atau 3V3 ESP32. Jangan mengandalkan regulator kecil pada board ESP32 untuk menyuplai receiver dari baterai motor bertegangan tinggi.

## 2. ESP32 ke ZK-BM1

| ESP32 | ZK-BM1 | Fungsi firmware |
| --- | --- | --- |
| GPIO4 | IN1 | Motor kiri maju, PWM |
| GPIO14 | IN2 | Motor kiri mundur, PWM |
| GPIO18 | IN3 | Motor kanan maju, PWM |
| GPIO19 | IN4 | Motor kanan mundur, PWM |
| GND | GND/signal GND | Referensi sinyal bersama |

GPIO5, GPIO21, dan GPIO33 tidak digunakan pada konfigurasi ZK-BM1 ini. ZK-BM1 tidak membutuhkan pin `PWMA`, `PWMB`, atau `STBY` terpisah.

## 3. Motor ke ZK-BM1

| Motor | Terminal driver |
| --- | --- |
| Motor kiri | OUT1 dan OUT2 |
| Motor kanan | OUT3 dan OUT4 |

Jika satu motor berputar terbalik, matikan daya lalu tukar posisi kedua kabel motor tersebut pada terminal output. Jangan membalik VIN baterai untuk mengubah arah motor.

## 4. FS-iA6B ke ESP32

Gunakan port keluaran i-BUS bertanda `SERVO`, bukan port `SENS`. Ikuti tulisan `S`, `+`, dan `-` pada casing/PCB receiver; jangan menentukan urutan pin hanya dari posisi gambar karena orientasi konektor dapat membingungkan.

| FS-iA6B | Tujuan | Keterangan |
| --- | --- | --- |
| `SERVO/i-BUS` signal (`S`) | Pembagi tegangan → GPIO16 | UART2 RX, 115200 baud |
| VCC (`+`) | 5V regulator/BEC | Jangan diberi tegangan baterai motor langsung |
| GND (`-`) | Common GND | Harus tersambung ke GND ESP32 |

### Pembagi tegangan i-BUS

GPIO ESP32 bekerja pada 3,3V dan tidak 5V-tolerant. Gunakan level shifter atau pembagi berikut:

```text
FS-iA6B signal ---- resistor 1kΩ ----+---- ESP32 GPIO16
                                     |
                                  resistor 2kΩ
                                     |
                                    GND
```

Dengan sinyal masukan 5V, pembagi 1kΩ/2kΩ menghasilkan sekitar 3,33V. Ground receiver dan ESP32 wajib sama. GPIO17/TX2 tidak perlu disambungkan karena firmware hanya menerima data dari receiver.

## 5. Pemetaan remote

| Kanal | Kontrol | Fungsi robot |
| --- | --- | --- |
| CH1 | Stick horizontal | Belok kiri/kanan |
| CH2 | Stick vertikal dengan pegas tengah | Maju/mundur |
| CH5 | Sakelar | Turbo ketika nilai di atas 1500 µs |

Firmware mengharapkan posisi kanal sekitar:

- Minimum: 1000 µs
- Tengah: 1500 µs
- Maksimum: 2000 µs
- Deadzone tengah: ±35 µs

Pastikan CH2 kembali ke posisi tengah. Jika transmitter menempatkan throttle pada CH3 yang tidak memakai pegas tengah, jangan gunakan kanal tersebut untuk maju/mundur sebelum mekanisme stick dibuat self-centering. Nomor kanal dapat diubah melalui `STEERING_CHANNEL`, `THROTTLE_CHANNEL`, dan `TURBO_CHANNEL` di `src/main.cpp`.

## 6. Ground bersama

Semua titik berikut harus terhubung secara elektrik:

- Negatif baterai
- GND keluaran regulator/BEC
- GND ESP32
- GND FS-iA6B
- GND sinyal ZK-BM1

Gunakan kabel daya tebal untuk baterai dan motor. Jalur arus motor jangan dilewatkan melalui breadboard atau pin GND kecil ESP32. Buat cabang ground daya langsung di dekat baterai/driver dan cabang ground sinyal menuju ESP32.

## 7. Proteksi dan tata kabel

- Pasang sekering sesuai arus aman baterai, kabel, driver, dan motor.
- ZK-BM1 tidak memiliki proteksi polaritas terbalik; periksa `VIN+` dan `VIN-` dengan multimeter.
- Gunakan pendinginan tambahan jika arus kanal mendekati atau melebihi 8A.
- Pasang kapasitor keramik 100nF langsung pada terminal setiap motor untuk membantu mengurangi noise.
- Jauhkan kabel antena receiver dari motor, kabel baterai, driver, logam, dan sumber noise.
- Letakkan kedua ujung antena receiver dengan orientasi berbeda dan jangan melipat bagian aktif antena.
- Firmware mengoperasikan PWM ZK-BM1 pada 2kHz dan menghentikan kanal sesaat sebelum membalik arah.

## 8. Urutan pemeriksaan pertama

1. Lepaskan motor dari driver.
2. Pastikan sakelar utama mati.
3. Periksa tidak ada hubungan pendek antara baterai `+` dan `-`.
4. Periksa polaritas VIN ZK-BM1 dan input regulator.
5. Nyalakan sistem tanpa motor, lalu ukur output regulator: harus sekitar 5V.
6. Pastikan tegangan pada GPIO16 tidak melebihi 3,3V saat receiver mengirim data.
7. Bind transmitter dan FS-iA6B. LED receiver harus menunjukkan koneksi stabil.
8. Buka serial monitor 115200 baud. Pastikan muncul data `[IBUS] CH1=... CH2=... CH5=...`.
9. Matikan transmitter dan pastikan muncul `[FAILSAFE] Sinyal receiver hilang`.
10. Matikan semua daya, sambungkan motor, lalu angkat roda dari lantai untuk pengujian awal.
11. Nyalakan transmitter terlebih dahulu, pastikan stick netral, baru nyalakan robot.

## 9. Ringkasan pin ESP32

| GPIO | Sambungan |
| --- | --- |
| GPIO2 | LED status bawaan |
| GPIO4 | ZK-BM1 IN1 |
| GPIO14 | ZK-BM1 IN2 |
| GPIO16 | FS-iA6B i-BUS melalui penurun level |
| GPIO18 | ZK-BM1 IN3 |
| GPIO19 | ZK-BM1 IN4 |

Referensi:

- [Dokumentasi ZK-BM1](https://docs.cirkitdesigner.com/component/86071865-61ac-4a34-bb10-e60bb542f0c1/zk-bm1-10a-motor-driver)
- [Manual ringkas FS-iA6B](https://www.manualslib.com/manual/3647931/Flysky-Fs-Ia6b.html)
