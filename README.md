# 💡 Remote Electromechanical Light Switch Controller

## 📌 Overview
This project is a custom electromechanical device designed to remotely control standard dorm room light switches. Built using an Arduino Nano, 3D-printed components, and servomotors, the system allows you to turn room lights on and off via an IR remote. 

A key feature of this project is its **motorized lift mechanism**. When the system is not in use, it can be mechanically lifted away from the wall switches, allowing them to be operated manually without interference. 

## ⚙️ Hardware Components
* **Microcontroller:** Arduino NANO
* **Actuators (Switches):** 3x 180° Servomotors 
* **Actuators (Lift Mechanism):** 2x Continuous Rotation (360°) Servomotors
* **Sensors:** IR Receiver
* **Power Supply:** 5V / 3A Dual-Port Charger (Powers both the board and the servos)
* **Other:** Capacitor (for voltage stabilization), jumper wires, custom 3D-printed mechanical parts.

## 🚀 Features & Functionality
* **Individual Light Control:** Buttons `1`, `2`, and `3` on the IR remote toggle their respective light switches using the 180° servos.
* **Lift Mechanism:** Pressing the `UP` arrow raises the entire assembly off the switches, restoring manual access. Pressing the `DOWN` arrow lowers it back into the operational position.
* **Safety Limits:** The code includes bounds checking. If the system is already lifted, it will ignore further `UP` commands to prevent mechanical damage (and vice versa for the `DOWN` command).
* **Safety Lock:** The servos responsible for switching the lights are disabled while the system is in the lifted position.

## 💻 Software Implementation
This repository contains two different software approaches for educational and performance-testing purposes:

### 1. The "Library" Version (`C++` with standard Arduino libraries)
This version uses standard Arduino libraries (`<Servo.h>` and `<IRremote.hpp>`). 
* **Pros:** Easy to read, highly stable, and quick to implement. 
* **Note:** The lift servos are mapped to analog pins (`A0`, `A1`) to avoid hardware timer conflicts with the `Servo.h` library, which utilizes Timer 1.

### 2. The "Register" Version (`Bare-Metal C`)
This version achieves the same functionality using direct port manipulation and hardware registers instead of the `Servo.h` library.
* **Pros:** Deep dive into AVR architecture, fine-grained control over PWM signals, and optimized execution time.
* **Features:** Custom ADC initialization, manual PWM generation for the 180° servos, and a custom sequential PWM generation method for the continuous rotation lift servos to ensure perfect synchronization.

## 🔗 Simulation
You can view and interact with a simulation of the circuit and logic on Tinkercad:
[Tinkercad Simulation Link](https://www.tinkercad.com/things/3BcjFi1SrNA-intrerupator?sharecode=H8IoDuDdiK2GippR8_AS-mVSm8j1dXkg5kdcr09ZVrY)
