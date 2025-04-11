# 🧱 BrickBase

BrickBase is the core module of the system. It handles Bluetooth communication, interprets code, manages module statuses, and acts as the interface between hardware and client. This subproject includes both software (PlatformIO) and hardware (KiCad) components.

---

## 📁 Project Structure

```
BrickBase/
├── .pio/                 # PlatformIO build output
├── .venv/                # Python virtual environment
├── .vscode/              # VSCode project settings
├── hardware/             # KiCad hardware design files
├── include/              # Header files
├── lib/                  # External libraries
├── scripts/              # Utility scripts (e.g. DebugClient.py)
├── src/                  # Main source code (C++)
│   ├── ble_gatt_api.cpp / .hpp
│   ├── ble_server.cpp / .hpp
│   ├── main.cpp
│   └── CMakeLists.txt
├── test/                 # Unit or integration tests
├── sdkconfig.esp32dev    # ESP32 configuration
├── platformio.ini        # PlatformIO project configuration
├── README.md             # Project documentation
└── .gitignore            # Git ignore rules
```

---

## ⚙️ Building the Project

To build and upload the firmware to the ESP32 using PlatformIO:

1. **Install PlatformIO:**
    - As a VSCode extension or via CLI: https://platformio.org/install

2. **Build the project:**
   ```bash
   pio run
   ```

3. **Upload to ESP32:**
   ```bash
   pio run --target upload
   ```

4. **Monitor the serial output:**
   ```bash
   pio device monitor
   ```

>[!WARNING]
>Be sure you ESP has at least 4mb for flash!
---

## 🧩 How It Works

BrickBase runs a Bluetooth Low Energy (BLE) server on the ESP32. It exposes GATT characteristics that can be accessed by a client (such as a GUI app). The core responsibilities are:

- 🧠 **Code Interpretation:** Accept and run logic from the client.
- 📡 **Bluetooth Server:** Communicate over BLE using a custom GATT profile.
- 📊 **Module Monitoring:** Provide feedback on hardware module statuses.

Python scripts like \`scripts/DebugClient.py\` can simulate or debug BLE communication.

---

## 🔧 Hardware

The `hardware` folder contains KiCad files used to design the hardware modules. These designs interface with the ESP32 board running the BrickBase firmware.

- Ensure to follow proper ESP32 pin mappings defined in the code
- Power delivery and signal lines are documented in the KiCad project

---

## 🧪 Testing

Basic test cases and setup can be found in the \`test/\` directory. Integration with PlatformIO allows easy extension of test coverage using Unity or other frameworks.

---

## 📌 Notes

- Designed for **ESP32-DevKit v1**
- Built with **PlatformIO**
- BLE GATT profile customized for this project’s requirements

---
