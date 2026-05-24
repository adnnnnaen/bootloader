import serial
import struct
import time
import sys
import zlib

PORT = "COM5"
BAUDRATE = 115200
FIRMWARE_PATH = r"C:\Users\moham\Documents\STM32\firmware\BootApp2\Debug\BootApp2.bin"
CHUNK_SIZE = 256

SOF = 0xAA
ACK = 0x06
NAK = 0x15
CMD_PING       = 0x01
CMD_BEGIN_OTA  = 0x03
CMD_DATA       = 0x04
CMD_END_OTA    = 0x05

def crc16_ccitt(data):
    crc = 0xFFFF
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            if crc & 0x8000:
                crc = (crc << 1) ^ 0x1021
            else:
                crc <<= 1
            crc &= 0xFFFF
    return crc

def build_frame(seq, payload):
    length = len(payload)
    header = struct.pack("<BH", seq, length)
    crc_input = header + payload
    crc = crc16_ccitt(crc_input)
    return bytes([SOF]) + crc_input + struct.pack("<H", crc)

def send_and_wait_ack(ser, frame, timeout=3.0):
    time.sleep(0.2)
    ser.reset_input_buffer()
    ser.write(frame)
    ser.flush()
    deadline = time.time() + timeout
    received = []
    while time.time() < deadline:
        b = ser.read(1)
        if not b:
            continue
        received.append(b[0])
        if b[0] == ACK or b[0] == NAK:
            return b[0]
    return None
def main():
    try:
        with open(FIRMWARE_PATH, "rb") as f:
            firmware = f.read()
    except FileNotFoundError:
        print(f"[ERREUR] Fichier introuvable : {FIRMWARE_PATH}")
        sys.exit(1)

    size = len(firmware)
    crc32 = zlib.crc32(firmware) & 0xFFFFFFFF
    print(f"[INFO] Firmware : {size} octets, CRC32 = 0x{crc32:08X}")
    print(f"[INFO] Nombre de chunks : {(size + CHUNK_SIZE - 1) // CHUNK_SIZE}")

    try:
        ser = serial.Serial(PORT, BAUDRATE, timeout=0.1)
    except serial.SerialException as e:
        print(f"[ERREUR] {e}")
        sys.exit(1)

    print(f"[OK] Port {PORT} ouvert")
    print("[INFO] Reset ta Nucleo MAINTENANT...")

    end_send = time.time() + 5.0
    last_send = 0
    ota_entered = False

    while time.time() < end_send:
        if time.time() - last_send > 0.2:
            ser.write(b'U')
            ser.flush()
            last_send = time.time()
        data = ser.read(256)
        if data:
            txt = data.decode('utf-8', errors='replace')
            print(txt, end='', flush=True)
            if "OTA mode entered" in txt:
                ota_entered = True
                break

    if not ota_entered:
        print("\n[ERREUR] Mode OTA non confirmé")
        ser.close()
        sys.exit(1)

    print("\n[INFO] Mode OTA confirmé")
    time.sleep(1.0)
    ser.reset_input_buffer()

    seq = 1
    print(f"\n[TX] PING")
    response = send_and_wait_ack(ser, build_frame(seq, bytes([CMD_PING])))
    if response != ACK:
        print(f"[ERREUR] PING : pas d'ACK (got {response})")
        ser.close()
        sys.exit(1)
    print("[OK] PING ACK")

    seq += 1
    print(f"\n[TX] BEGIN_OTA(size={size})")
    payload = bytes([CMD_BEGIN_OTA]) + struct.pack("<I", size)
    response = send_and_wait_ack(ser, build_frame(seq, payload), timeout=5.0)
    if response != ACK:
        print(f"[ERREUR] BEGIN_OTA : pas d'ACK (got {response})")
        ser.close()
        sys.exit(1)
    print("[OK] BEGIN_OTA ACK")

    print(f"\n[TX] Envoi du firmware en chunks de {CHUNK_SIZE} octets...")
    offset = 0
    chunk_num = 0
    total_chunks = (size + CHUNK_SIZE - 1) // CHUNK_SIZE

    while offset < size:
        chunk = firmware[offset : offset + CHUNK_SIZE]
        seq = (seq % 255) + 1
        payload = bytes([CMD_DATA]) + struct.pack("<I", offset) + chunk
        response = send_and_wait_ack(ser, build_frame(seq, payload), timeout=3.0)
        if response != ACK:
            print(f"\n[ERREUR] DATA chunk {chunk_num} (offset={offset}) : pas d'ACK (got {response})")
            ser.close()
            sys.exit(1)
        chunk_num += 1
        offset += len(chunk)
        if chunk_num % 20 == 0:
            print(f"  ... {chunk_num}/{total_chunks} chunks envoyés")

    print(f"[OK] Tous les {chunk_num} chunks envoyés ({offset} octets)")

    seq = (seq % 255) + 1
    print(f"\n[TX] END_OTA(crc=0x{crc32:08X})")
    payload = bytes([CMD_END_OTA]) + struct.pack("<I", crc32)
    response = send_and_wait_ack(ser, build_frame(seq, payload), timeout=5.0)
    if response != ACK:
        print(f"[ERREUR] END_OTA : pas d'ACK (got {response})")
        ser.close()
        sys.exit(1)
    print("[OK] END_OTA ACK")

    print("\n[INFO] OTA réussie ! Lecture des logs de reboot (5s)...")
    end_listen = time.time() + 5.0
    while time.time() < end_listen:
        data = ser.read(256)
        if data:
            print(data.decode('utf-8', errors='replace'), end='', flush=True)

    print("\n[INFO] Fin")
    ser.close()

if __name__ == "__main__":
    main()