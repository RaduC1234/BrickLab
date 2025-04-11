# BrickLab

**Learn IoT by playing!**  
BrickLab is a mobile educational platform that helps students **learn IoT device programming** through an intuitive, **Scratch-style** block programming interface, **interactive schematics**, and **step-by-step tutorials** — all designed to make learning **fun and accessible**.

---

## ✨ Features

- 📱 **User-friendly Android mobile app**
- 🧩 **Drag-and-drop block-based coding** (inspired by Scratch)
- 🔌 **Interactive hardware schematics**
- 🎯 **Hands-on tutorials for real-world IoT devices**
- 🚀 **Learn by building and programming**

---

## 🛠 Project Overall Structure

### 1. Software

| Area                | Details                                                                                                                                                                         |
|---------------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| **Mobile App**       | Developed in **Android Studio** using **React Native** and **TypeScript**. The app features a friendly drag-and-drop interface for code block programming.                      |
| **Hardware Programming** | Using **ESP-IDF** framework for programming **ESP32** modules, and **AVR 8-bit** for Atiny85 microcontrollers on the modules.                                                   |
| **Technologies**     | - Android Studio<br>- React Native<br>- TypeScript<br>- ESP-IDF Framework<br>- AVR C Programming<br>- I2C Protocol<br>- Bluetooth Low Energy - Generic Attribute Profile (Gatt) |

---

### 2. Hardware

| Component            | Purpose |
|----------------------|---------|
| **ESP32**             | Main microcontroller for Wi-Fi / Bluetooth IoT applications. |
| **Atiny85 MCUs**    | Secondary microcontrollers for smaller or offline projects. |
| **Sensors**           | (Temperature, humidity, light, etc.) for real-world IoT experiments. |
| **Actuators**         | (LEDs, Motors, Buzzers) to interact with the environment. |
| **Other Components**  | - Breadboards<br>- Resistors<br>- Capacitors<br>- Wires and Connectors |

---

## 📚 Learning Approach

1. **Assemble** the hardware following an interactive schematic.
2. **Drag-and-drop** code blocks inside the app to program the device.
3. **Upload** the generated code to the microcontroller via Wi-Fi or Bluetooth.
4. **Watch** your IoT creation come to life!
5. **Iterate and improve** by exploring new challenges and tutorials.

---

## 📁 Project Structure

The project is divided into two primary subprojects:

### 1. **BrickBase**
This is the core of the system. It includes:
- The software, built with PlatformIO
- The hardware design, developed using KiCad
- A Bluetooth server that handles:
    - Code interpretation
    - Module status monitoring
    - Communication with the client

### 2. **🖥️ Brick GUI**
_(Description to be added)_


## 🚀 Getting Started

Coming soon — full documentation, setup instructions, and example projects!