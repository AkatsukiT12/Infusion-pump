# Unified IV Infusion Pump System

[▶️ **Watch Demo Video**](https://drive.google.com/file/d/1-zoyE7b9eoAHnOaxktvvvHKQ4us8PItz/view?usp=sharing)

---

## Overview

The **Unified IV Infusion Pump System** is an integrated medical device designed to safely and accurately deliver liquid drugs intravenously while providing real-time monitoring and alarms. The system combines **embedded hardware control**, **sensor feedback**, **local display**, and a **Flutter-based mobile monitoring application** into one unified solution.

This project fulfills the requirements of controlled drug delivery by regulating **flow rate** and **delivered volume**, sensing the actual output, displaying both set and measured values, and triggering alarms under unsafe conditions.

---

## System Architecture

The system is composed of three tightly integrated layers:

1. **Hardware Layer (Infusion Pump Controller)**

   * Arduino-based controller
   * L298N H-Bridge motor driver
   * DC motors (syringe / infusion pumps)
   * Flow sensor for real-time measurement
   * 16×2 I2C LCD for local display
   * LEDs and buzzer for alarms

2. **Communication Layer**

   * Serial communication with an external server
   * Periodic transmission of flow rate, liquid level, and pump status

3. **Application Layer (Flutter Mobile App)**

   * Patient management and monitoring
   * QR-code-based patient data acquisition
   * Real-time visualization and alerts

---

## Hardware Functionality

### Controlled Parameters

* **Flow Rate (mL/hr)**: Controlled by adjusting motor speed using PWM.
* **Delivered Volume / Liquid Level (%)**: Controlled by stopping infusion once the target volume is reached.

### Sensed Parameters

* Actual flow rate (via flow sensor pulses)
* Remaining liquid level (calculated percentage)
* Pump activity state (ON / OFF)

### Local LCD Display

The LCD continuously shows:

* Pre-adjusted (set) flow rate or operating mode
* Measured flow rate
* Remaining liquid level (%)
* System state (IDLE / MANUAL / REMOTE)

---

## Operating Modes

* **IDLE Mode**
  System is powered on but not delivering medication.

* **MANUAL Mode**
  Local control using a push button and potentiometer. The potentiometer sets the desired flow rate.

* **REMOTE Mode**
  Full control via serial commands received from the server (used by the Flutter app).

---

## Safety and Alarms

The system implements multiple alarms to ensure patient safety:

### 1. Pump Stall / Flow Error Alarm (Hardware-Level)

* Triggered when the measured flow rate is significantly lower than the set rate.
* Actions:

  * Motor is stopped
  * Red LED blinks
  * Buzzer sounds
  * Error message displayed on LCD

### 2. Air Bubble Detection Alarm (App-Level)

* An `airBubbleFlag` is received from the server.
* When true, the Flutter app immediately displays a high-priority alert to medical staff.

---

## Flutter Mobile Application (Monitoring Feature)

### Purpose

The Flutter application enhances the infusion pump by providing **remote monitoring**, **patient management**, and **early fault detection**, improving safety and workflow efficiency.

### Key Features

#### Patient Registration via QR Code

* On app launch, the user can add a patient.
* The phone camera scans a QR code containing:

  * Patient ID
  * Drug name or drug code
  * Ideal dose rate

#### Real-Time Server Data

The following values are continuously updated from the external server:

* Current dose rate
* Current liquid level (%)
* Infusion activity flag (ON / OFF)
* Air bubble detection flag

#### Patient Cards Dashboard

* Each patient is displayed in a card-based UI.
* Card contents include:

  * Patient ID
  * Drug name
  * Ideal dose rate
  * Current dose rate
  * Liquid level indicator

#### Visual Status Indicator

* **Active infusion** → Normal colored card
* **Inactive infusion** → Greyed-out card

#### Dynamic Liquid Level Indicator

* Works similar to a mobile battery indicator:

  * **60–100%** → Green
  * **30–59%** → Orange
  * **< 30%** → Red
* The indicator and percentage text update dynamically in real time.

---

## Communication Protocol

### Commands Sent to Pump Controller

* `START:patientID:speed:volume`
* `STOP`
* `SPEED:newSpeed`
* `CAL:newCalibrationFactor`

### Data Sent from Pump Controller

```
flowRate,liquidLevel,isActive,alarmFlag
```

---

## Conclusion

The **Unified IV Infusion Pump System** demonstrates a complete medical device solution that integrates **accurate drug delivery**, **sensor feedback**, **local and remote monitoring**, and **multi-level safety mechanisms**. The added Flutter-based monitoring feature significantly enhances usability, safety, and effectiveness in clinical environments.

---
