#!/usr/bin/env python3

import hid
import argparse
import sys

SONY_VID = 0x054C

DS4_PIDS = [
    0x05C4,  # CUH-ZCT1
    0x09CC,  # CUH-ZCT2
]

DS3_PIDS = [
    0x0268,  # DualShock 3
    0x0267,  # DualShock 3 (older)
]

CONTROLLERS = {
    "ds4": {
        "name": "DualShock 4",
        "pids": DS4_PIDS,
        "get_report": 0x12,
        "set_report": 0x13,
        "report_len": 64,
        "controller_slice": slice(1, 7),
        "host_slice": slice(10, 16),
        "reverse_mac": False,
    },
    "ps3": {
        "name": "DualShock 3",
        "pids": DS3_PIDS,
        "get_report": 0xF2,
        "set_report": 0xF5,
        "report_len": 18,
        "controller_slice": slice(4, 10),
        "host_slice": slice(11, 17),
        "reverse_mac": True,
    },
}

def bytes_to_mac(data, reverse=False):
    if reverse:
        data = data[::-1]
    return ":".join(f"{x:02X}" for x in data)

def mac_to_bytes(mac, reverse=False):
    res = bytearray(int(x, 16) for x in mac.split(":"))
    if reverse:
        res = res[::-1]
    return bytes(res)

def autodetect_controller():
    """
    Memindai semua perangkat HID yang terhubung untuk mendeteksi tipe controller otomatis.
    """
    print("Memindai controller Sony yang terhubung...")
    device_list = hid.enumerate(SONY_VID)
    
    if not device_list:
        return None, None, None

    # Cari kesocokan PID dari daftar device yang terdeteksi
    for dev_info in device_list:
        pid = dev_info['product_id']
        
        for c_type, config in CONTROLLERS.items():
            if pid in config["pids"]:
                try:
                    h = hid.device()
                    # Membuka menggunakan path lebih aman jika ada lebih dari 1 device
                    h.open_path(dev_info['path'])
                    return h, pid, config
                except OSError:
                    # Coba buka menggunakan VID/PID biasa jika open_path gagal
                    try:
                        h.open(SONY_VID, pid)
                        return h, pid, config
                    except OSError:
                        pass
    return None, None, None

def find_controller(controller_type):
    """
    Mencari controller spesifik berdasarkan input user (manual mode).
    """
    controller = CONTROLLERS.get(controller_type)
    if not controller:
        raise ValueError(f"Controller type '{controller_type}' tidak dikenal")

    for pid in controller["pids"]:
        try:
            h = hid.device()
            h.open(SONY_VID, pid)
            return h, pid, controller
        except OSError:
            pass

    raise RuntimeError(f"{controller['name']} tidak ditemukan. Pastikan terhubung via USB.")

def get_pairing_info(dev, controller):
    report = dev.get_feature_report(controller["get_report"], controller["report_len"])

    if len(report) < 16 and controller["name"] == "DualShock 4":
        raise RuntimeError(f"Feature report 0x{controller['get_report']:02X} gagal dibaca")

    controller_mac = bytes_to_mac(report[controller["controller_slice"]], reverse=controller["reverse_mac"])
    host_mac = bytes_to_mac(report[controller["host_slice"]], reverse=controller["reverse_mac"])

    return controller_mac, host_mac

def set_host_mac(dev, host_mac, controller):
    if controller["name"] == "DualShock 4":
        payload = bytearray(23)
        payload[0] = controller["set_report"]
        payload[1:7] = mac_to_bytes(host_mac, reverse=controller["reverse_mac"])
        payload[7:23] = b"\x00" * 16
    
    elif controller["name"] == "DualShock 3":
        payload = bytearray(8)
        payload[0] = controller["set_report"]
        payload[1] = 0x00
        payload[2:8] = mac_to_bytes(host_mac, reverse=controller["reverse_mac"])
    
    written = dev.send_feature_report(payload)

    if written <= 0:
        raise RuntimeError(f"Gagal menulis report 0x{controller['set_report']:02X}")

def main():
    parser = argparse.ArgumentParser(
        description="DualShock 3 / 4 Bluetooth Pairing Tool (Autodetect)"
    )

    parser.add_argument(
        "-t", "--type",
        choices=["ds4", "ps3"],
        help="Paksa tipe controller: ds4 atau ps3 (Opsional, bawaannya otomatis)"
    )

    parser.add_argument(
        "--set",
        metavar="MAC",
        help="Set host bluetooth MAC (Format -> AA:BB:CC:DD:EE:FF)"
    )

    args = parser.parse_args()

    try:
        # Penanganan Autodetect vs Manual
        if args.type:
            print(f"Mode Manual: Mencari {CONTROLLERS[args.type]['name']}...")
            dev, pid, controller = find_controller(args.type)
        else:
            dev, pid, controller = autodetect_controller()
            if not dev:
                raise RuntimeError("Tidak ada DualShock 3 atau DualShock 4 yang terdeteksi. Pastikan kabel USB terhubung.")
        
        print(f"\n[+] Perangkat Ditemukan: {controller['name']} (PID: 0x{pid:04X})")

        controller_mac, host_mac = get_pairing_info(dev, controller)
        print(f"    MAC Controller : {controller_mac}")
        print(f"    MAC Host Aktif : {host_mac}")

        if args.set:
            # Validasi input format MAC sederhana
            if len(args.set).split(":") != 6:
                raise ValueError("Format MAC Address salah. Gunakan format seperti: AA:BB:CC:DD:EE:FF")
                
            print(f"\nMengubah host menjadi {args.set.upper()}...")
            set_host_mac(dev, args.set, controller)
            print("[+] Berhasil disetel! Silakan cabut kabel USB dan tekan tombol PS.")

        dev.close()

    except Exception as e:
        print(f"\n[!] Error: {e}", file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    main()