# 🩺 Hypertension Wristband

<p align="center">
  <img src="https://img.shields.io/badge/Platform-ESP32-blue?style=for-the-badge&logo=espressif" />
  <img src="https://img.shields.io/badge/Sensor-MAX30101%20%7C%20MAX32664D-red?style=for-the-badge" />
  <img src="https://img.shields.io/badge/Communication-Bluetooth-lightblue?style=for-the-badge&logo=bluetooth" />
  <img src="https://img.shields.io/badge/Type-SSIP%20Project-green?style=for-the-badge" />
  <img src="https://img.shields.io/badge/IDE-Arduino-teal?style=for-the-badge&logo=arduino" />
</p>

<p align="center">
  A smartwatch-style wearable prototype that estimates <strong>Blood Pressure</strong> and <strong>Heart Rate</strong> using <strong>PPG (Photoplethysmography)</strong> optical sensing — no cuff required.
</p>

---

## 📌 Overview

**Hypertension-Wristband** is a non-invasive, continuous blood pressure monitoring system built as a wearable wristband prototype. Traditional blood pressure monitors rely on inflatable cuffs and are bulky and inconvenient for continuous monitoring. This project explores an alternative approach using optical PPG sensing combined with physiological signal processing.

The system uses the **MAX30101** optical sensor to capture light absorption changes caused by blood volume fluctuations in the wrist. These signals are processed by the **MAX32664D** biometric sensor hub, and the resulting physiological data is sent to an **ESP32 microcontroller** which estimates Systolic Blood Pressure (SBP), Diastolic Blood Pressure (DBP), and Heart Rate (HR). The data is then transmitted wirelessly via **Bluetooth** to a mobile device.

---

## ✨ Features

| Feature | Description |
|---|---|
| 💓 Heart Rate Monitoring | Continuous real-time HR measurement via PPG |
| 🩸 Blood Pressure Estimation | SBP & DBP derived from pulse wave analysis |
| 📡 Bluetooth Transmission | Wireless data sent to phone or serial terminal |
| 🔋 Low Power Design | Optimized for wearable battery life |
| 🛡️ Safety Validation | Multi-gate filtering: confidence, HR range, BP clamp, pulse pressure check |
| 📱 Mobile App Ready | Output formatted for easy mobile app integration |

---

## 🔧 Hardware Components

| Component | Model | Role |
|---|---|---|
| PPG Sensor | MAX30101 | Emits Red + IR light; measures reflected intensity changes |
| Sensor Hub | MAX32664D | Signal filtering, noise reduction, feature extraction |
| Microcontroller | ESP32 | Main processor + Bluetooth communication |
| Interface | I2C Bus | Communication between MAX32664D and ESP32 |
| Power | LiPo Battery | Portable power supply for wearable use |
| Output | Mobile App / Serial | Data visualization |

---

## 🏗️ System Architecture

```
┌───────────┐     ┌───────────┐     ┌──────────┐     ┌──────────────┐     ┌─────────────┐
│  MAX30101  │────▶│ MAX32664D │────▶│  ESP32   │────▶│  Bluetooth   │────▶│  Mobile App │
│ PPG Sensor │     │ Signal Hub│     │ (BLE MCU)│     │  (Serial BT) │     │  / Terminal │
└───────────┘     └───────────┘     └──────────┘     └──────────────┘     └─────────────┘
  Optical           I2C Comm.        Estimation         Wireless               Display
  Sensing           + Processing     Algorithm          Transmission
```

---

## ⚙️ Working Principle

### Step 1 — PPG Signal Acquisition

The **MAX30101** sensor sits on the underside of the wristband, pressed gently against the wrist. It emits Red (660 nm) and Infrared (880 nm) light into the skin. As the heart pumps blood, the volume of blood in the capillaries changes with each beat, altering how much light is absorbed vs. reflected back.

This variation in reflected light intensity forms the **PPG waveform**, which encodes:

- **Heart Rate** — derived from the peak-to-peak interval of the waveform
- **Pulse Wave Amplitude** — related to stroke volume and arterial compliance
- **Pulse Rise Time** — correlated with arterial stiffness
- **Inter-Beat Interval (IBI)** — used for HRV and BP estimation

---

### Step 2 — Signal Processing (MAX32664D)

The raw PPG signal from MAX30101 is fed into the **MAX32664D**, a dedicated biometric sensor hub by Maxim Integrated. It handles all the heavy lifting:

- Low-pass and band-pass **filtering** to remove noise
- **Motion artifact reduction** using accelerometer-aided algorithms
- **Feature extraction**: heart rate, confidence score, sensor status
- Outputs clean physiological parameters to ESP32 via **I2C**

The **confidence score** (0–100) and **status byte** are used downstream to gate out poor-quality readings.

---

### Step 3 — Blood Pressure Estimation (ESP32)

The ESP32 receives pre-processed data and runs a **PTT-based linear approximation** algorithm to estimate blood pressure. The equations are calibrated so that at a typical resting heart rate of 75 BPM, the output matches a normal adult baseline:

```
SBP = 95.0  + (0.333 × HR)     →  at HR=75 BPM, SBP ≈ 120 mmHg
DBP = 41.25 + (0.517 × HR)     →  at HR=75 BPM, DBP ≈ 80 mmHg
```

These estimates then pass through **four validation gates** before transmission:

| Gate | Condition | Action on Fail |
|---|---|---|
| 1 — Signal Quality | `confidence ≥ 90` AND `status == 3` | Skip reading |
| 2 — HR Range | `40 ≤ HR ≤ 180 BPM` | Skip reading |
| 3 — BP Clamp | SBP: 90–140 mmHg, DBP: 60–90 mmHg | Clamp to limits |
| 4 — Pulse Pressure | `20 ≤ PP ≤ 100 mmHg` | Skip reading |

---

## 💻 Software Workflow

```
[1] Read bioData from MAX32664D (via I2C)
      ↓
[2] Check: confidence ≥ 90 AND status == 3
      ↓ (fail → skip, wait 2s)
[3] Validate: 40 ≤ HR ≤ 180
      ↓ (fail → skip, wait 2s)
[4] Compute SBP_raw and DBP_raw from HR
      ↓
[5] Clamp SBP and DBP to clinical safe range
      ↓
[6] Check pulse pressure (PP = SBP - DBP): 20–100 mmHg
      ↓ (fail → skip, wait 2s)
[7] Format and transmit via Bluetooth Serial
      ↓
[8] Log to Serial Monitor for debugging
      ↓
[9] Wait 2 seconds → loop
```

---

## 📤 Example Output

```
HR:72 SBP:119 DBP:78
HR:74 SBP:119 DBP:79
HR:76 SBP:120 DBP:80
HR:71 SBP:118 DBP:78
```

The Bluetooth device name is: **`BP_Watch`**

Pair your phone to `BP_Watch` and open a Bluetooth Serial terminal (e.g., Serial Bluetooth Terminal on Android) to see live readings.

---

## 🛠️ Getting Started

### Prerequisites

- **Arduino IDE** (1.8.x or 2.x)
- **ESP32 Board Support** — Install via Board Manager:
  ```
  https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
  ```
- **SparkFun Bio Sensor Hub Library** — Install via Library Manager in Arduino IDE
  - Search: `SparkFun Bio Sensor Hub`

### Wiring

| MAX32664D Pin | ESP32 Pin |
|---|---|
| SDA | GPIO 21 |
| SCL | GPIO 22 |
| RESET | GPIO 5 |
| MFIO | GPIO 4 |
| VCC | 3.3V |
| GND | GND |

> ⚠️ MAX32664D operates at **3.3V logic**. Do not connect to 5V.

### Upload

1. Open `bp_wristband.ino` in Arduino IDE
2. Select **Board**: `ESP32 Dev Module`
3. Select the correct **COM Port**
4. Click **Upload**
5. Open **Serial Monitor** at `115200 baud`
6. Pair your phone to Bluetooth device `BP_Watch`

---

## 📊 Blood Pressure Classification (Reference)

| Category | SBP (mmHg) | DBP (mmHg) |
|---|---|---|
| Normal | < 120 | < 80 |
| Elevated | 120–129 | < 80 |
| High (Stage 1) | 130–139 | 80–89 |
| High (Stage 2) | ≥ 140 | ≥ 90 |
| Hypertensive Crisis | > 180 | > 120 |
| Low (Hypotension) | < 90 | < 60 |

---

## 🔭 Future Enhancements

- [ ] Machine Learning based BP estimation (replacing linear approximation)
- [ ] Cloud data storage and sync (Firebase / AWS IoT)
- [ ] Dedicated mobile application (Android/iOS)
- [ ] Historical trend analysis and graphing
- [ ] Emergency alert system (SMS / push notification on high BP)
- [ ] AI-Based Hypertension detection and risk assessment
- [ ] PPG waveform visualization on OLED display
- [ ] Accelerometer-based motion artifact cancellation (hardware level)

---

## 🧰 Technologies Used

- **Embedded C++** (Arduino framework)
- **ESP32** (Dual-core 240 MHz MCU with built-in Bluetooth)
- **I2C Protocol** (Sensor ↔ MCU communication)
- **Bluetooth Serial** (ESP32 Classic BT)
- **SparkFun Bio Sensor Hub Library**
- **PPG Signal Processing** (via MAX32664D sensor hub)
- **Arduino IDE**

---

## ⚠️ Disclaimer

> This project is developed **strictly for educational, research, and prototype demonstration purposes** under the SSIP program.
>
> The blood pressure values produced by this device are **estimates based on a simplified linear model** and have **not been clinically validated**. This device:
> - Does **not** replace certified medical blood pressure monitors
> - Should **not** be used for clinical diagnosis or treatment decisions
> - Should **not** be used as a substitute for professional medical advice
>
> Always consult a qualified healthcare professional for accurate blood pressure readings and medical guidance.

---

## 👨‍💻 Authors

| Name | Role |
|---|---|
| **Fenil Finava** | Developer & Embedded Systems Engineer |
| **Prof. Pinal Hansora** | Project Guide & Mentor |

**Department:** Computer Engineering  
**Program:** SSIP (Student Startup and Innovation Policy) — Government of Gujarat  
**Institution:** [Your College Name]

---

## 📄 License

This project is open-source and available for educational use. If you use this work, please give appropriate credit to the authors.

---

<p align="center">
  Made with ❤️ for healthcare innovation | SSIP Project 2024–25
</p>
