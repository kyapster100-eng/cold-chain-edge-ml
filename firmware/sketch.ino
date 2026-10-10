#include "ColdChainML.h"
#include <Wire.h>
#include <DHT.h>
#include <math.h>

// ==========================================
// PIN & SENSOR CONFIGURATION
// ==========================================
#define DHTPIN 4
#define DHTTYPE DHT22
#define LDRPIN 34
#define MPU_ADDR 0x68

DHT dht(DHTPIN, DHTTYPE);

// ML Model Instance
Eloquent::ML::Port::DecisionTree mlModel;

// ==========================================
// ARRHENIUS KINETIC DECAY STATE
// ==========================================
float cumulative_loss_pct = 0.0;
unsigned long last_sample_ms = 0;

const float Ea_R = 8000.0;          // Activation energy over gas constant (K)
const float REF_TEMP_K = 277.15;    // Reference 4°C in Kelvin
const float A_FACTOR = exp(8000.0 / 277.15);
const float INITIAL_RSL_HOURS = 120.0;

// ==========================================
// LIGHTWEIGHT HELPER FUNCTIONS
// ==========================================

// Lightweight I2C read for MPU6050 (No heavy Adafruit libraries needed)
void initMPU() {
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x6B); // Power Management Register
    Wire.write(0);    // Wake up MPU6050
    Wire.endTransmission(true);
}

float readMPUVibration() {
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x3B); // Accel X register
    Wire.endTransmission(false);
    if (Wire.requestFrom(MPU_ADDR, 6, true) == 6) {
        int16_t ax = Wire.read() << 8 | Wire.read();
        int16_t ay = Wire.read() << 8 | Wire.read();
        int16_t az = Wire.read() << 8 | Wire.read();
        
        // Convert to m/s^2 (±2g range scale factor = 16384 LSB/g)
        float ax_ms2 = (ax / 16384.0) * 9.81;
        float ay_ms2 = (ay / 16384.0) * 9.81;
        float az_ms2 = (az / 16384.0) * 9.81;
        
        float magnitude = sqrt(ax_ms2 * ax_ms2 + ay_ms2 * ay_ms2 + az_ms2 * az_ms2);
        return fabs(magnitude - 9.81); // Subtract 1g static gravity
    }
    return 0.05; // Fallback default
}

int evaluateEdgeML(float temp, float humidity, float light, float vibration) {
    float features[4] = {temp, humidity, light, vibration};
    return mlModel.predict(features);
}

float updateArrheniusLoss(float temp_c, float dt_hours) {
    float T_k = temp_c + 273.15;
    float k_rate = exp(-Ea_R / T_k) * A_FACTOR;
    float step_loss = (100.0 / INITIAL_RSL_HOURS) * k_rate * dt_hours;
    
    cumulative_loss_pct += step_loss;
    if (cumulative_loss_pct > 100.0) cumulative_loss_pct = 100.0;
    
    return cumulative_loss_pct;
}

// ==========================================
// SETUP
// ==========================================
void setup() {
    Serial.begin(115200);
    Wire.begin(21, 22);

    dht.begin();
    initMPU();

    Serial.println("==========================================");
    Serial.println(" ESP32 COLD CHAIN EDGE ML SIMULATION RUNNING ");
    Serial.println("==========================================");
    
    last_sample_ms = millis();
}

// ==========================================
// MAIN LOOP
// ==========================================
void loop() {
    unsigned long now = millis();
    if (now - last_sample_ms >= 2000) { // Sample every 2 seconds
        float dt_hours = (now - last_sample_ms) / 3600000.0;
        last_sample_ms = now;

        // 1. Read DHT22
        float temp = dht.readTemperature();
        float humidity = dht.readHumidity();
        if (isnan(temp)) temp = 4.0;       
        if (isnan(humidity)) humidity = 85.0;

        // 2. Read Light Sensor
        float light = analogRead(LDRPIN);

        // 3. Read Raw MPU6050 Vibration
        float vibration = readMPUVibration();

        // 4. Update Kinetic Arrhenius Model
        float quality_loss = updateArrheniusLoss(temp, dt_hours);
        float rsl_hours = max(0.0f, (100.0f - quality_loss) * 1.2f);

        // 5. Execute Edge Machine Learning Inference
        int ml_status = evaluateEdgeML(temp, humidity, light, vibration);
        const char* status_str = (ml_status == 0) ? "SAFE" : ((ml_status == 1) ? "ELEVATED_RISK" : "SPOILED");

        // 6. Print Debug Output
        Serial.println("------------------------------------------");
        Serial.printf("Temp: %.2f °C | Humidity: %.2f %%\n", temp, humidity);
        Serial.printf("Light ADC: %.1f | Vibration: %.3f m/s²\n", light, vibration);
        Serial.printf("Quality Loss: %.2f %% | RSL: %.2f Hours\n", quality_loss, rsl_hours);
        Serial.printf(">>> Edge ML Prediction: [%s]\n", status_str);

        // 7. Format JSON Payload via standard C-string (Zero overhead)
        char jsonBuffer[256];
        snprintf(jsonBuffer, sizeof(jsonBuffer),
            "{\"temp\":%.2f,\"humidity\":%.2f,\"light\":%.1f,\"vibration\":%.3f,\"loss\":%.2f,\"rsl\":%.2f,\"status\":%d,\"label\":\"%s\"}",
            temp, humidity, light, vibration, quality_loss, rsl_hours, ml_status, status_str);

        Serial.print("Generated JSON Payload: ");
        Serial.println(jsonBuffer);
    }
}