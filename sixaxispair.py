#!/usr/bin/env python3

"""
sixaxispair.py - Improved DualShock 3/4 Bluetooth pairing helper using hidapi

Features added:
- Robust MAC validation and normalization
- Safe HID handling (try/finally) and improved autodetect
- Better DS3/DS4 feature report handling and validation
- Verbose logging, dependency checks and --auto host MAC detection
"""

import argparse
import sys
import subprocess
import re
import os
import glob

try:
    import hid
except Exception as e:
    print("[ERROR] Missing dependency: hid (hidapi). Install with: pip install hidapi", file=sys.stderr)
    sys.exit(2)

SONY_VID = 0x054C

DS4_PIDS = [
    0x05C4,  # CUH-ZCT1
    0x09CC,  # CUH-ZCT2
]

DS3_PIDS = [
    0x0268,  # DualShock 3
    0x0267,  # DualShock 3 (older)
]

# Controller configs — report_len is a suggested safe size to request
CONTROLLERS = {
    "ds4": {
        "name": "DualShock 4",
        "pids": DS4_PIDS,
        "get_report": 0x12,
        "set_report": 0x13,
        "report_len": 64,  # safe upper bound for DS4 feature report
        "controller_slice": slice(1, 7),
        "host_slice": slice(10, 16),
        "reverse_mac": True,
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


def log(msg, verbose=False):
    if verbose:
        print(msg)


def is_valid_mac(mac: str) -> bool:
    if not isinstance(mac, str):
        return False
    mac = mac.strip()
    return bool(re.fullmatch(r"([0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2}", mac))


def normalize_mac(mac: str) -> str:
    return mac.strip().upper()


def bytes_to_mac(data, reverse=False):
    if reverse:
        data = data[::-1]
    return ":".join(f"{x:02X}" for x in data)


def mac_to_bytes(mac, reverse=False):
    mac = normalize_mac(mac)
    parts = mac.split(":")
    res = bytearray(int(x, 16) for x in parts)
    if reverse:
        res = res[::-1]
    return bytes(res)


def open_hid_device_safe(dev_info, verbose=False):
    """Try to open a hid device using path first, then vid/pid. Returns opened device or raises."""
    path = dev_info.get("path")
    vid = dev_info.get("vendor_id")
    pid = dev_info.get("product_id")
    h = hid.device()
    try:
        if path:
            log(f"Opening HID by path: {path}", verbose)
            h.open_path(path)
            return h
        else:
            log(f"Opening HID by VID/PID: {vid:04X}/{pid:04X}", verbose)
            h.open(vid, pid)
            return h
    except OSError:
        try:
            # Last resort: try open with vid/pid if path open failed
            if vid is not None and pid is not None:
                h.open(vid, pid)
                return h
        except OSError:
            h.close()
            raise


def autodetect_controller(verbose=False):
    """Scan HID devices for supported Sony controllers and return opened device + controller config."""
    print("Memindai controller Sony yang terhubung...")
    device_list = hid.enumerate(SONY_VID)

    if not device_list:
        return None, None, None

    # Prefer devices with matching PIDs and where we can open the correct interface
    for dev_info in device_list:
        pid = dev_info.get("product_id")
        log(f"Found device: path={dev_info.get('path')} vid={dev_info.get('vendor_id'):04X} pid={pid:04X} prod={dev_info.get('product_string')} man={dev_info.get('manufacturer_string')} iface={dev_info.get('interface_number')}", verbose)

        for c_type, config in CONTROLLERS.items():
            if pid in config["pids"]:
                # Attempt to open the HID interface safely
                try:
                    h = open_hid_device_safe(dev_info, verbose=verbose)
                    return h, pid, config
                except OSError as e:
                    log(f"Failed to open device (path or VID/PID): {e}", verbose)
                    continue

    return None, None, None


def find_controller(controller_type, verbose=False):
    controller = CONTROLLERS.get(controller_type)
    if not controller:
        raise ValueError(f"Controller type '{controller_type}' tidak dikenal")

    # Enumerate and try to open matching PIDs explicitly
    for dev_info in hid.enumerate(SONY_VID):
        pid = dev_info.get("product_id")
        if pid in controller["pids"]:
            try:
                h = open_hid_device_safe(dev_info, verbose=verbose)
                return h, pid, controller
            except OSError:
                continue

    raise RuntimeError(f"{controller['name']} tidak ditemukan. Pastikan terhubung via USB.")


def get_pairing_info(dev, controller, verbose=False):
    get_report = controller["get_report"]
    expected_len = controller.get("report_len", 64)

    try:
        report = dev.get_feature_report(get_report, expected_len)
    except OSError as e:
        raise RuntimeError(f"Gagal membaca feature report 0x{get_report:02X}: {e}")

    if not report or len(report) < 8:
        raise RuntimeError(f"Feature report 0x{get_report:02X} kosong atau terlalu pendek (len={len(report)})")

    # DS4: expect at least slices defined, tolerate multiple lengths
    try:
        controller_mac = bytes_to_mac(report[controller["controller_slice"]], reverse=controller["reverse_mac"])
        host_mac = bytes_to_mac(report[controller["host_slice"]], reverse=controller["reverse_mac"])
    except Exception as e:
        raise RuntimeError(f"Gagal parsing MAC dari report: {e}")

    log(f"Controller report length={len(report)} controller_mac={controller_mac} host_mac={host_mac}", verbose)
    return controller_mac, host_mac


def set_host_mac(dev, host_mac, controller, verbose=False):
    if not is_valid_mac(host_mac):
        raise ValueError("Format MAC Address salah. Gunakan format seperti: AA:BB:CC:DD:EE:FF")

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

    else:
        raise RuntimeError("Controller tidak didukung untuk penulisan MAC")

    # Attempt to send full payload and verify
    try:
        written = dev.send_feature_report(payload)
    except OSError as e:
        raise RuntimeError(f"Gagal menulis report 0x{controller['set_report']:02X}: {e}")

    # hidapi may return number of bytes written; ensure > 0
    if written is None or written <= 0:
        raise RuntimeError(f"Gagal menulis report 0x{controller['set_report']:02X} (written={written})")

    # On some implementations, send_feature_report returns the number of bytes excluding report id.
    # We check at least that some bytes were written.
    log(f"Wrote {written} bytes to report 0x{controller['set_report']:02X}", verbose)


def get_local_bt_mac_auto(verbose=False):
    """Try to discover the local Bluetooth adapter MAC using several fallbacks."""
    # 1) bluetoothctl list
    try:
        out = subprocess.check_output(["bluetoothctl", "list"], stderr=subprocess.DEVNULL, text=True)
        for line in out.splitlines():
            m = re.search(r"Controller\s+([0-9A-F:]{17})", line, re.I)
            if m:
                return normalize_mac(m.group(1))
    except Exception:
        pass

    # 2) hciconfig
    try:
        out = subprocess.check_output(["hciconfig"], stderr=subprocess.DEVNULL, text=True)
        m = re.search(r"([0-9A-F:]{17})", out, re.I)
        if m:
            return normalize_mac(m.group(1))
    except Exception:
        pass

    # 3) /sys/class/bluetooth/hci*/address
    try:
        for path in glob.glob('/sys/class/bluetooth/hci*/address'):
            with open(path, 'r') as f:
                addr = f.read().strip()
                if is_valid_mac(addr):
                    return normalize_mac(addr)
    except Exception:
        pass

    return None


def main():
    parser = argparse.ArgumentParser(description="DualShock 3 / 4 Bluetooth Pairing Tool (Autodetect)")
    parser.add_argument("-t", "--type", choices=["ds4", "ps3"], help="Paksa tipe controller: ds4 atau ps3 (Opsional, bawaannya otomatis)")
    parser.add_argument("--set", metavar="MAC", help="Set host bluetooth MAC (Format -> AA:BB:CC:DD:EE:FF)")
    parser.add_argument("--auto", action="store_true", help="Gunakan MAC lokal dari adapter Bluetooth (bluetoothctl/hciconfig/sysfs)")
    parser.add_argument("-v", "--verbose", action="store_true", help="Tampilkan debug verbose")

    args = parser.parse_args()

    verbose = args.verbose

    try:
        # Choose host MAC if requested --auto
        if args.auto:
            local_mac = get_local_bt_mac_auto(verbose=verbose)
            if not local_mac:
                raise RuntimeError("Tidak dapat menemukan MAC adapter Bluetooth lokal menggunakan bluetoothctl/hciconfig/sysfs")
            print(f"[INFO] Menggunakan MAC lokal: {local_mac}")

        # Penanganan Autodetect vs Manual
        dev = None
        if args.type:
            print(f"Mode Manual: Mencari {CONTROLLERS[args.type]['name']}...")
            dev, pid, controller = find_controller(args.type, verbose=verbose)
        else:
            dev, pid, controller = autodetect_controller(verbose=verbose)
            if not dev:
                raise RuntimeError("Tidak ada DualShock 3 atau DualShock 4 yang terdeteksi. Pastikan kabel USB terhubung.")

        print(f"\n[+] Perangkat Ditemukan: {controller['name']} (PID: 0x{pid:04X})")

        # Show device info if available (best effort)
        try:
            # device info may not be attached to dev after open, so show via enumerate match
            for info in hid.enumerate(SONY_VID):
                if info.get('product_id') == pid:
                    print(f"    VID: 0x{info.get('vendor_id'):04X}")
                    print(f"    PID: 0x{info.get('product_id'):04X}")
                    print(f"    Product: {info.get('product_string')}")
                    print(f"    Manufacturer: {info.get('manufacturer_string')}")
                    print(f"    Path: {info.get('path')}")
                    break
        except Exception:
            pass

        try:
            controller_mac, host_mac = get_pairing_info(dev, controller, verbose=verbose)
            print(f"    MAC Controller : {controller_mac}")
            print(f"    MAC Host Aktif : {host_mac}")

            if args.set or args.auto:
                if args.auto:
                    new_mac = local_mac
                else:
                    new_mac = args.set

                if not is_valid_mac(new_mac):
                    raise ValueError("Format MAC Address salah. Gunakan format seperti: AA:BB:CC:DD:EE:FF")

                print(f"\nMengubah host menjadi {normalize_mac(new_mac)}...")
                set_host_mac(dev, new_mac, controller, verbose=verbose)
                print("[+] Berhasil disetel! Silakan cabut kabel USB dan tekan tombol PS.")

        finally:
            try:
                dev.close()
            except Exception:
                pass

    except Exception as e:
        print(f"\n[!] Error: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()