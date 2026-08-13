#!/usr/bin/env python3
"""
VELA FLIGHT SOFTWARE - GROUND DATA SYSTEM (GDS)
-----------------------------------------------
Ingests binary telemetry frames over UART, validates frame sync,
unpacks packed C structs, displays real-time telemetry, and logs 
structured datasets for downstream Edge AI / ML training.
"""

import sys
import time
import struct
import csv
from datetime import datetime

try:
    import serial
    import serial.tools.list_ports
except ImportError:
    print("[ERROR] 'pyserial' is required. Install it using: pip install pyserial")
    sys.exit(1)

# Protocol Constants (Matching protocol.h)
SYNC_WORD = 0xAA55
FRAME_HEADER_SIZE = 4  # Sync (2B) + Type (1B) + Len (1B)
PAYLOAD_SIZE = 12       # Temp (float4) + Rad (uint32) + Att (float4)
CRC_SIZE = 1            # CRC8 (1B)
TOTAL_FRAME_SIZE = FRAME_HEADER_SIZE + PAYLOAD_SIZE + CRC_SIZE

LOG_FILE = "telemetry_dataset.csv"

def find_serial_port():
    """Scans and returns available COM/tty ports."""
    ports = serial.tools.list_ports.comports()
    if not ports:
        print("[GDS] No active serial ports detected.")
        return None
    
    print("\n[GDS] Available Serial Ports:")
    for i, port in enumerate(ports):
        print(f"  [{i}] {port.device} - {port.description}")
    
    selection = input("\nSelect port index or enter port name (e.g., 4 or COM8): ").strip()
    
    # Accept direct port names like COM8
    if selection.upper().startswith("COM") or selection.startswith("/dev/"):
        return selection.upper()
    
    # Accept index numbers like 4
    idx = int(selection) if selection.isdigit() and int(selection) < len(ports) else 0
    return ports[idx].device

def initialize_csv():
    """Initializes CSV header for dataset generation."""
    try:
        with open(LOG_FILE, mode='a', newline='') as f:
            writer = csv.writer(f)
            # Write header if file is empty
            if f.tell() == 0:
                writer.writerow(["timestamp", "temperature_c", "radiation_hits", "attitude_rad"])
        print(f"[GDS] Dataset logging initialized -> '{LOG_FILE}'")
    except Exception as e:
        print(f"[GDS WARNING] Could not initialize CSV log: {e}")

def parse_telemetry_frame(payload_bytes):
    """
    Unpacks binary payload using struct format string:
    'f' = float (4 bytes), 'I' = uint32 (4 bytes), 'f' = float (4 bytes)
    Little-endian order '<fIf'
    """
    try:
        temp, rad, att = struct.unpack("<fIf", payload_bytes)
        return temp, rad, att
    except struct.error as e:
        print(f"\n[GDS ERROR] Frame unpacking failed: {e}")
        return None

def main():
    print("==================================================")
    print("   VELA FLIGHT - GROUND CONTROL TELEMETRY SYSTEM   ")
    print("==================================================")

    port = find_serial_port()
    if not port:
        print("[GDS FATAL] Cannot proceed without a valid serial port.")
        sys.exit(1)

    baud = 115200
    print(f"[GDS] Connecting to {port} at {baud} baud...")

    try:
        ser = serial.Serial(port, baud, timeout=1.0)
        time.sleep(1) # Allow serial bus to stabilize
        print(f"[GDS STATUS] Connected. Waiting for frame synchronization...\n")
    except serial.SerialException as e:
        print(f"[GDS FATAL] Serial connection error: {e}")
        sys.exit(1)

    initialize_csv()

    frame_count = 0
    crc_errors = 0

    try:
        while True:
            # Hunt for Sync Word (0x55 0xAA in byte stream)
            b1 = ser.read(1)
            if b1 == b'\x55':
                b2 = ser.read(1)
                if b2 == b'\xaa':
                    # Sync Found! Read remaining frame bytes
                    remaining_bytes = ser.read(TOTAL_FRAME_SIZE - 2)
                    
                    if len(remaining_bytes) == (TOTAL_FRAME_SIZE - 2):
                        frame_type = remaining_bytes[0]
                        payload_len = remaining_bytes[1]
                        payload = remaining_bytes[2:2 + PAYLOAD_SIZE]
                        received_crc = remaining_bytes[-1]

                        # Decode telemetry payload
                        telemetry = parse_telemetry_frame(payload)
                        
                        if telemetry:
                            temp, rad, att = telemetry
                            frame_count += 1
                            timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f")[:-3]

                            # Live Dashboard Terminal Render
                            print(f"\r[Telemetry #{frame_count:05d} | {timestamp}] "
                                  f"TEMP: {temp:6.2f} °C  |  "
                                  f"RAD: {rad:6d} hits  |  "
                                  f"ATTITUDE: {att:6.3f} rad", end="", flush=True)

                            # Write to ML Training Dataset
                            with open(LOG_FILE, mode='a', newline='') as f:
                                writer = csv.writer(f)
                                writer.writerow([timestamp, f"{temp:.2f}", rad, f"{att:.3f}"])
            
            # Non-blocking yield
            time.sleep(0.001)

    except KeyboardInterrupt:
        print(f"\n\n[GDS] Mission Session Ended.")
        print(f"[GDS STATS] Total Frames Processed: {frame_count} | CRC Drops: {crc_errors}")
        ser.close()

if __name__ == "__main__":
    main()