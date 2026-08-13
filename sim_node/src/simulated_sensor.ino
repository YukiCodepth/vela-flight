#include <Arduino.h>

// Standard hardware serial pins for ESP32 UART2
#define RX2_PIN 16
#define TX2_PIN 17

// Mirror the tightly packed struct from the STM32 exactly
#pragma pack(push, 1)
struct TelemetryPayload {
    float temperature;
    uint32_t radiation;
    float attitude;
};

struct TelemetryFrame {
    uint16_t sync = 0xAA55;
    uint8_t type = 0x01;
    uint8_t len = sizeof(TelemetryPayload);
    TelemetryPayload payload;
    uint8_t crc = 0; // Simple placeholder for Phase 2
};
#pragma pack(pop)

float current_temp = 25.0;
uint32_t rad_count = 0;
float phase = 0.0;

void setup() {
    // Serial is for your Windows computer (Debugging)
    Serial.begin(115200); 
    
    // Serial2 is the physical wire going to the STM32
    Serial2.begin(115200, SERIAL_8N1, RX2_PIN, TX2_PIN);
    
    Serial.println("ESP32 Sensor Simulator Online.");
}

void loop() {
    TelemetryFrame frame;
    
    // 1. Generate Synthetic Data
    current_temp += random(-20, 20) / 100.0; // Thermal drift
    if (random(0, 10) > 7) rad_count += 1;   // Particle hits
    phase += 0.1;
    
    frame.payload.temperature = current_temp;
    frame.payload.radiation = rad_count;
    frame.payload.attitude = sin(phase);
    
    // 2. Transmit the binary frame over UART
    uint8_t* raw_bytes = (uint8_t*)&frame;
    Serial2.write(raw_bytes, sizeof(TelemetryFrame));
    
    Serial.println("Telemetry Packet Sent -> STM32");
    
    delay(1000); // 1 Hz Telemetry Cycle
}
