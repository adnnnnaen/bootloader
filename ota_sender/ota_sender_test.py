import serial
import struct
import time
import sys

PORT = "COM5"
BAUDRATE = 115200

SOF = 0xAA
ACK = 0x06
NAK = 0x15
CMD_PING = 0x01

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

def main():
    try:
        ser = serial.Serial(PORT, BAUDRATE, timeout=2.0)
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
        print("\n[ERREUR] Le bootloader n'a pas confirmé l'entrée en mode OTA")
        ser.close()
        sys.exit(1)

    print("\n[INFO] Mode OTA confirmé, on envoie un PING...")
    time.sleep(0.3)

    seq = 1
    payload = bytes([CMD_PING])
    frame = build_frame(seq, payload)

    ser.reset_input_buffer()

    print(f"[TX] Trame PING : {frame.hex(' ')}")
    ser.write(frame)
    ser.flush()

    response = ser.read(1)
    if not response:
        print("[ERREUR] Pas de réponse (timeout)")
    elif response[0] == ACK:
        print(f"[OK] ACK reçu (0x{response[0]:02X}) - PING réussi")
    elif response[0] == NAK:
        print(f"[KO] NAK reçu (0x{response[0]:02X})")
    else:
        print(f"[?] Réponse inattendue : 0x{response[0]:02X}")

    print("\n[INFO] Lecture des logs restants (2s)...")
    end_listen = time.time() + 2.0
    while time.time() < end_listen:
        data = ser.read(256)
        if data:
            print(data.decode('utf-8', errors='replace'), end='', flush=True)

    print("\n[INFO] Fin")
    ser.close()

if __name__ == "__main__":
    main()