# Edge-IoT Multimodal Cold-Chain Quality Prediction

An ESP32-based edge machine learning system that evaluates cold-chain cargo degradation in real time using multimodal sensor fusion across Temperature (T), Relative Humidity (H), Ambient Light (L), and Vibration (V).

---

## Project Overview
Traditional cold-chain loggers rely on rigid single-variable thresholds (T > 8℃) or cloud processing that fails during transit signal dropouts. This project deploys a native C++ Decision Tree model (`ColdChainML.h`) directly onto an ESP32 microcontroller to provide sub-20 ms local spoilage predictions without external network dependencies.

---

##  Key Benchmark Metrics (3,000-Sample Unseen Test Set)

| Metric | Static Threshold Baseline | Thermal Baseline (Arrhenius) | Proposed Edge Decision Tree |
| :--- | :---: | :---: | :---: |
| **Accuracy** | 61.2% | 76.4% | **89.9%** |
| **Weighted F1-Score** | 55.6% | 73.4% | **90.2%** (+35.2%) |
| **Spoilage (`SPOILED`) Recall** | 48.3% | 68.1% | **92.0%** |
| **Inference Latency** | < 1 ms | 4.2 ms | **< 20 ms** (Typ. 2–5 ms) |
| **Memory Footprint** | 0.8 KB | 3.4 KB | **4.2 KB Flash / 0.1 KB RAM** |

---

##  Hardware Pin Mapping

| Component / Sensor | Function | ESP32 GPIO | Interface Type | Notes |
| :--- | :--- | :---: | :--- | :--- |
| **DHT22** | Temp & Humidity | **GPIO 4** | Digital 1-Wire | 10 kΩ pull-up to 3.3V |
| **MPU6050** | Vibration (Z-axis) | **GPIO 21 / 22** | Hardware I2C | SDA = 21, SCL = 22 |
| **LDR Module** | Light Intensity | **GPIO 34** | Analog Input | **ADC1 Channel 6** (avoids Wi-Fi lockouts) |
| **Status LEDs** | Red / Yellow / Green | **GPIO 18 / 19 / 5** | Digital Output | Red = Spoiled, Yellow = Risk, Green = Safe |
| **Buzzer** | Audio Alarm | **GPIO 23** | Digital PWM | Active piezo buzzer |

---

##  Repository Structure
- `dataset/`: Contains the 10,000-sample telemetry dataset CSV used for training and test evaluation.
- `firmware/`: Contains `sketch.ino` (main acquisition loop) and `ColdChainML.h` (compiled C++ inference logic).
- `model_training/`: Jupyter Notebook detailing data processing, decision tree optimization, baseline comparisons, and C++ header generation.
- `docs/`: Technical report PDF and Wokwi simulation layout images.

---

##  How to Run the Firmware
1. Open `firmware/sketch.ino` in the Arduino IDE or VS Code (PlatformIO).
2. Ensure `ColdChainML.h` resides in the same directory as `sketch.ino`.
3. Install required libraries: `DHT sensor library` and `Adafruit MPU6050`.
4. Select board **ESP32 Dev Module** and upload the code.
5. Open Serial Monitor at **115200 baud** to view real-time state predictions.
