#include <Arduino.h>

#define RX2_PIN 16
#define TX2_PIN 17

#pragma pack(push, 1)
struct TelemetryPayload {
    float temperature;
    uint32_t radiation;
    float attitude;
};

struct TelemetryFrame {
    uint16_t sync = 0xAA55;
    uint8_t type = 0x01;
    uint8_t length = sizeof(TelemetryPayload);
    TelemetryPayload payload;
    uint8_t crc = 0x00; // Simplified CRC placeholder
};
#pragma pack(pop)

TelemetryFrame frame;

uint8_t calculate_crc(uint8_t *data, size_t len) {
    uint8_t crc = 0x00;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
    }
    return crc;
}

void setup() {
    // USB Serial for Ground Station GDS
    Serial.begin(115200);
    // Hardware Serial2 for STM32 OBC link
    Serial2.begin(115200, SERIAL_8N1, RX2_PIN, TX2_PIN);
}

void loop() {
    // Generate synthetic sensor data
    frame.payload.temperature = 22.5f + (random(-10, 10) / 10.0f);
    frame.payload.radiation = 120 + random(0, 15);
    frame.payload.attitude = (millis() % 3600) / 100.0f;
    
    // Compute CRC
    frame.crc = calculate_crc((uint8_t*)&frame.payload, sizeof(TelemetryPayload));

    // Send binary frame over hardware UART to STM32
    Serial2.write((uint8_t*)&frame, sizeof(TelemetryFrame));
    
    // Send binary frame over USB to Python Ground Station
    Serial.write((uint8_t*)&frame, sizeof(TelemetryFrame));

    delay(1000);
}
