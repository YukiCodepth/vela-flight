#!/usr/bin/env python3
"""
VELA FLIGHT SOFTWARE - TELEMETRY DICT GENERATOR (NASA JPL STYLE)
---------------------------------------------------------------
Reads 'telemetry_dictionary.yaml' and auto-generates the C header 
file (protocol.h) to guarantee zero mismatch between flight code and GDS.
"""

import os
import sys

try:
    import yaml
except ImportError:
    print("[ERROR] 'PyYAML' is required. Install it using: pip install PyYAML")
    sys.exit(1)

YAML_PATH = "tools/telemetry_dictionary.yaml"
OUTPUT_HEADER = "flight_software/Core/Inc/protocol.h"

def generate_header():
    if not os.path.exists(YAML_PATH):
        print(f"[FATAL] Telemetry dictionary not found at {YAML_PATH}")
        sys.exit(1)

    with open(YAML_PATH, 'r') as f:
        data = yaml.safe_load(f)

    fields = data.get("telemetry_fields", [])
    sync_word = data.get("sync_word", "0xAA55")

    # Construct the C header contents
    c_code = f"""/**
 * AUTO-GENERATED FILE BY dict_generator.py
 * DO NOT EDIT MANUFACTURED CODE DIRECTLY. MODIFY telemetry_dictionary.yaml INSTEAD.
 */

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

#define TELEMETRY_SYNC_WORD {sync_word}

#pragma pack(push, 1)
typedef struct {{
"""

    for field in fields:
        c_code += f"    {field['type']} {field['name']}; // {field['description']}\n"

    c_code += """} TelemetryPayload_t;

typedef struct {
    uint16_t sync;
    uint8_t type;
    uint8_t length;
    TelemetryPayload_t payload;
    uint8_t crc;
} TelemetryFrame_t;
#pragma pack(pop)

#endif // PROTOCOL_H
"""

    # Ensure output directory exists
    os.makedirs(os.path.dirname(OUTPUT_HEADER), exist_ok=True)

    with open(OUTPUT_HEADER, 'w') as f:
        f.write(c_code)

    print(f"[SUCCESS] Auto-generated C header successfully -> '{OUTPUT_HEADER}'")

if __name__ == "__main__":
    generate_header()