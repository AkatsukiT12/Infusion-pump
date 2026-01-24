<!-- DEMO VIDEO (AUTO-PLAY AT TOP) -->

<p align="center">
  <video width="720" controls autoplay muted loop>
    <source src="https://drive.google.com/file/d/1-zoyE7b9eoAHnOaxktvvvHKQ4us8PItz/view?usp=sharing" type="video/mp4">
    Your browser does not support the video tag.
  </video>
</p>

---

# Unified IV Infusion Pump System

## Overview

This project presents a **Unified IV Infusion Pump System** that combines **embedded hardware control** with a **Flutter-based mobile monitoring application**. The system is designed to accurately deliver liquid drugs to patients while continuously monitoring critical parameters such as **flow rate**, **remaining liquid level**, and **safety conditions**.

The solution satisfies medical device requirements by implementing:

* Closed-loop control of infusion flow
* Real-time sensing and feedback
* Local and remote operation modes
* Visual and audible alarms
* Mobile-based monitoring for healthcare staff

---

## System Architecture

The system consists of three main layers:

1. **Hardware Layer (Infusion Pump Controller)**

   * Microcontroller-based control (Arduino)
   * L298N H-Bridge motor driver
   * Flow sensor for real-time flow measurement
   * LCD for local display
   * LEDs and buzzer for alarms

2. **Communication Layer**

   * Serial communication between pump controller and external server
   * Periodic transmission of flow rate, liquid level, and activity state

3. **Application Layer (Flutter Mobile App)**

   * Patient management via QR code scanning
   * Real-time monitoring dashboard
   * Visual alerts and status indicators

---

## Hardware Features

### Controlled Parameters

* **Flow Rate (mL/hr)**: Controlled using PWM motor speed with feedback from a flow sensor.
* **Delivered Volume / Liquid Level (%)**: Calculated based on cumulative dispensed volume.

### Sensed Parameters

* Actual flow rate (via flow sensor pulses)
* Remaining liquid level (percentage)
* Motor activity state (ON/OFF)

### Local Display

* 16×2 I2C LCD displays:

  * Pre-adjusted (set) flow rate
  * Measured flow rate
  * Liquid level percentage
  * System mode (IDLE / MANUAL / REMOTE)

### Operation Modes

* **IDLE Mode**: System waiting for command or manual start.
* **MANUAL Mode**: Local control using potentiometer and push button.
* **REMOTE Mode**: Full control via serial commands from the server/app.

---

## Safety and Alarms

The system implements **multiple safety mechanisms**:

1. **Pump Stall Alarm**

   * Triggered when the set flow rate is high but the measured flow rate is significantly low.
   * Actions:

     * Motor stopped
     * Audible buzzer activated
     * Red LED blinking
     * Error message displayed on LCD

2. **Air Bubble Alarm (App-Level)**

   * Server provides an `airBubbleFlag`.
   * If true, the Flutter app immediately shows a high-priority alert to staff.

These alarms ensure patient safety and immediate intervention.

---

## Flutter Mobile Application (New Feature Proposal)

### Purpose

The Flutter application acts as a **remote infusion pump monitoring system**, enhancing usability, safety, and efficiency for medical staff.

### Key Features

#### 1. Patient Registration via QR Code

* When a staff member opens the app:

  * They are prompted to **add a new patient**.
  * The camera opens to scan a QR code.

**QR Code Data Contains:**

* Patient ID
* Drug name or drug code
* Ideal dose rate

#### 2. Real-Time Data from Server

The following parameters are continuously updated from the external server:

* Current dose rate
* Current liquid level (%)
* Infusion activity flag (ON/OFF)
* Air bubble detection flag

#### 3. Patient Cards Dashboard

* Each patient is displayed in a **card-based UI**.
* Card design matches the provided reference layout.

**Card Elements:**

* Patient ID
* Drug name
* Ideal dose rate
* Current dose rate
* Liquid level indicator

#### 4. Visual Status Indicator

* Each patient card includes an **activity flag**:

  * **Active (Infusing)** → Normal colored card
  * **Inactive (Stopped)** → Greyed-out card

This flag is continuously synchronized with the server.

#### 5. Dynamic Liquid Level Indicator

* Sidebar indicator behaves like a mobile battery indicator:

  * **60–100%** → Green
  * **30–59%** → Orange
  * **< 30%** → Red

* The bar dynamically fills according to the percentage value.

* Text label updates in real time (e.g., `90%`, `89%`, etc.).

#### 6. Emergency Alerts

* If the air bubble flag is received as `true`:

  * The app immediately triggers a full-screen alert
  * Audible and visual warning for medical staff

---

## Communication Protocol

### Commands Sent to Pump Controller

* `START:patientID:speed:volume`
* `STOP`
* `SPEED:newSpeed`
* `CAL:newCalibrationFactor`

### Data Sent from Pump Controller

Formatted as:

```
flowRate,liquidLevel,isActive,alarmFlag
```


## Conclusion

This Unified IV Infusion Pump System integrates **embedded control**, **real-time monitoring**, and a **mobile application** to enhance accuracy, safety, and usability. The proposed Flutter-based monitoring feature significantly improves clinical workflow, enables early fault detection, and supports safer patient care.

---
