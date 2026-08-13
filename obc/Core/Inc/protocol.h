#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

#define SYNC_WORD 0xAA55
#define TYPE_TELEMETRY 0x01

// Force the compiler to pack this struct tightly (no padding)
#pragma pack(push, 1)

typedef struct {
    float temperature;   // 4 bytes
    uint32_t radiation;  // 4 bytes
    float attitude;      // 4 bytes
} TelemetryPayload;

typedef struct {
    uint16_t sync;
    uint8_t type;
    uint8_t len;
    TelemetryPayload payload; // 12 bytes
    uint8_t crc;              // 1 byte
} TelemetryFrame;             // Total: 17 bytes

#pragma pack(pop)

#endif
