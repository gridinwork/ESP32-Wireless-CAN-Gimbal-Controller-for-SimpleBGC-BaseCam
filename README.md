# ESP32 Wireless CAN Gimbal Controller for SimpleBGC / BaseCam

ESP32-S3 wired and BLE control system for a 3-axis SimpleBGC/BaseCam camera gimbal with CAN motor drivers. The project covers controller setup, CAN motor preparation, wiring and operating modes.

> **Firmware source code is intentionally not published yet.** It will be added after modification, review and testing.

[Русская версия](README_RU.md) · [Attribution](ATTRIBUTION.md)

---

## 1. Project overview

The system uses ESP32-S3 as an external controller for a SimpleBGC/BaseCam three-axis gimbal. Three joystick channels control Roll, Pitch and Yaw. Commands are sent to BaseCam through the Serial API, while BaseCam handles stabilization and CAN motor control.

Two configurations are supported: a direct wired version with one ESP32-S3 and a wireless BLE version with two ESP32-S3 boards.

![Reference ESP32 and BaseCam gimbal-control system](https://content.instructables.com/FN8/KDHB/LZH05JMA/FN8KDHBLZH05JMA.jpg)

---

## 2. SimpleBGC GUI setup before installing the gimbal motors

Connect the BaseCam controller to the computer by USB, start SimpleBGC32 GUI, select the controller COM port and press **Connect**. Verify communication before changing any parameters.

### Firmware / GUI update

Open **Upgrade / Firmware Upgrade** and verify that the controller firmware and GUI version are compatible with the board revision. Do not interrupt USB or power during an update.

![SimpleBGC firmware upgrade page](https://content.instructables.com/FX9/KS0T/LXRK8FSE/FX9KS0TLXRK8FSE.png)

### Serial API input

Open **RC Settings → Input Configuration** and configure the input used for ESP32 communication as **Serial port (Serial API, etc.)**. In the reference configuration, `RC_ROLL` is assigned to the Serial API.

### Main IMU

Open **Hardware → Main IMU Sensor**. Configure the sensor arrangement according to the actual controller. On boards with a suitable onboard IMU, it may be used as a fallback if the main IMU is not detected.

![SimpleBGC Main IMU configuration](https://hackster.imgix.net/uploads/attachments/1747324/main_imu_sensor_-_red_gbJ5YyKhgr.png)

### Serial speed and calibration

Set both the main serial port and `RC_SERIAL` to **115200 baud**. The ESP32 UART must use the same speed. Calibrate the IMU in multiple stable orientations and save the configuration to the controller.

---

## 3. CAN motors and driver preparation

The reference system uses three CAN-connected gimbal motors. Each motor driver requires the correct power supply, CAN_H/CAN_L connection and a unique CAN ID.

- `CAN_H` BaseCam → `CANH` of all motor drivers
- `CAN_L` BaseCam → `CANL` of all motor drivers
- Supply positive → driver VCC / power input
- Supply negative → driver GND

Use twisted-pair wiring for CAN_H and CAN_L and verify the required CAN termination for the final topology.

![Reference CAN motor installation](https://content.instructables.com/FM5/MOML/LZCPU6LL/FM5MOMLLZCPU6LL.jpg)

### CAN driver firmware

Power the CAN driver modules and connect them to BaseCam. In SimpleBGC32 GUI open **Upgrade → Connected Modules**, refresh the device list and verify each module.

![Detected CAN modules in SimpleBGC GUI](https://hackster.imgix.net/uploads/attachments/1747339/can_modules_scan_devices_markup_MBQV1EI2MD.png)

Select the correct firmware for the exact hardware revision, flash each module separately, reconnect the system and verify the reported firmware version. Configure motor pole count, encoder settings and electrical parameters for the actual motors. The reference DM5005 hardware uses 28 poles; do not copy this value to a different motor without verification.

---

## 4. Direct connection — one ESP32-S3

A single ESP32-S3 reads the Roll, Pitch and Yaw joystick inputs and sends processed commands to BaseCam through UART / Serial API.

![Direct ESP32-S3 to BaseCam wiring](https://content.instructables.com/FXC/8LE9/LZCPU8J9/FXC8LE9LZCPU8J9.png)

Reference pin map:

| Function | ESP32-S3 | Destination |
|---|---|---|
| Joystick 1 signal | GPIO6 | Joystick SIG |
| Joystick 2 signal | GPIO7 | Joystick SIG |
| Joystick 3 signal | GPIO15 | Joystick SIG |
| UART RX | GPIO18 | BaseCam UART1_TX |
| UART TX | GPIO17 | BaseCam UART1_RX |
| Joystick supply | 3V3 | Joystick VCC |
| Ground | GND | Joysticks + BaseCam GND |

UART is cross-connected: ESP32 RX → BaseCam TX and ESP32 TX → BaseCam RX.

![ESP32-S3 and joystick reference wiring](https://content.instructables.com/F5I/UCSY/LZCPU6GV/F5IUCSYLZCPU6GV.jpg)

### Operating principle

After startup, the ESP32 establishes serial communication with BaseCam, continuously reads the three analog joystick channels, converts them into Roll/Pitch/Yaw commands and sends the control values through the BaseCam Serial API. The modified source code will be published later.

---

## 5. Bluetooth Low Energy control — two ESP32-S3 boards

In the wireless version, the first ESP32-S3 is used as the handheld controller. It reads the three joysticks and transmits Roll/Pitch/Yaw values over **Bluetooth Low Energy (BLE)**. A second ESP32-S3 mounted on the gimbal receives the values and forwards commands to BaseCam through UART / Serial API.

![BLE ESP32-S3 to BaseCam wiring](https://content.instructables.com/FMB/SVXN/LZCPU8N6/FMBSVXNLZCPU8N6.png)

Data path:

`Joysticks → Controller ESP32-S3 → BLE → Gimbal ESP32-S3 → UART / Serial API → BaseCam → CAN → Motor Drivers`

The BaseCam/CAN side remains essentially the same as in the direct version; only the joystick inputs move to the remote ESP32.

![BaseCam controller reference installation](https://content.instructables.com/FML/2Q7Y/LZCPU6DU/FML2Q7YLZCPU6DU.jpg)

---

## 6. Angle and Speed control modes

### Angle mode

Joystick input represents a requested angular position. BaseCam moves the selected Roll, Pitch or Yaw axis toward the commanded position. This mode is suitable for direct positional control.

### Speed mode

Joystick displacement represents requested rotational speed. A larger displacement commands faster movement; returning the joystick to its neutral region commands approximately zero rotation. This mode is useful for smooth panning and continuous camera movement.

The selected mode must be consistent between the SimpleBGC configuration and the future ESP32 firmware. A dead-band/dead-zone should be used to suppress small ADC variations around joystick center.

---

## 7. General recommendations

Verify board revision, connector pinout, CAN polarity, supply voltage and CAN IDs before applying motor power. Perform initial configuration with motors disabled, keep a backup of a known-good SimpleBGC profile, use twisted CAN wiring with appropriate termination, and implement a communication failsafe for BLE operation.

---

## Open-source acknowledgment

The ESP32 control concept and initial software approach used as a development reference are based in part on **ESP32 + BaseCam Bluetooth Gimbal Control** by **Austin Allen / Elation Sports Technologies LLC**, released under the MIT License. This repository documents the GEC Engineering integration and subsequent modification work. See [ATTRIBUTION.md](ATTRIBUTION.md) for details.
