/**
 * AUTO-GENERATED FILE BY dict_generator.py
 * DO NOT EDIT MANUFACTURED CODE DIRECTLY. MODIFY telemetry_dictionary.yaml INSTEAD.
 */

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

#define TELEMETRY_SYNC_WORD 0xAA55

#pragma pack(push, 1)
typedef struct {
    float temperature; // Core thermal sensor reading in Celsius
    uint32_t radiation; // Cumulative cosmic radiation particle hits
    float attitude; // Spacecraft attitude angle in radians
} TelemetryPayload_t;

typedef struct {
    uint16_t sync;
    uint8_t type;
    uint8_t length;
    TelemetryPayload_t payload;
    uint8_t crc;
} TelemetryFrame_t;
#pragma pack(pop)

#endif // PROTOCOL_H
