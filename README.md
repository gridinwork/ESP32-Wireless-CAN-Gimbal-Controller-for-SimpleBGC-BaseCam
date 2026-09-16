# ESP32 Wireless CAN Gimbal Controller for SimpleBGC / BaseCam

ESP32-S3 wired and BLE control system for a 3-axis SimpleBGC/BaseCam camera gimbal with CAN motor drivers. This repository documents the system architecture, controller setup, CAN motor preparation, wiring, operating modes and further development by GEC Engineering.

> **Project status:** hardware and documentation integration. Firmware source code is intentionally not published yet; it will be added only after modification, review and testing.

[Русская версия](README_RU.md)

---

## 1. Project overview

The purpose of this project is to provide external control of a three-axis camera gimbal through an ESP32-S3 while retaining the stabilization and motor-control capabilities of a SimpleBGC/BaseCam controller.

The ESP32 can process three independent joystick channels for **Roll**, **Pitch** and **Yaw** and send the resulting commands to the BaseCam controller through its Serial API. The BaseCam controller then manages the gimbal axes and CAN-connected motor drivers.

Two configurations are considered:

1. **Direct wired control** — one ESP32-S3 reads all three joystick channels and communicates directly with the BaseCam controller over UART.
2. **Wireless BLE control** — one ESP32-S3 acts as the handheld joystick controller and sends commands over Bluetooth Low Energy to a second ESP32-S3 mounted on the gimbal. The second ESP32 communicates with BaseCam over UART.

The original ESP32 software concept was based on the MIT-licensed work by Austin Allen / Elation Sports Technologies. See [Attribution](ATTRIBUTION.md).

---

## 2. Main system components

- SimpleBGC/BaseCam 32-bit gimbal controller
- ESP32-S3 development board — 1 board for direct control or 2 boards for BLE control
- Three single-axis joystick modules
- Three-axis gimbal mechanics
- Three CAN motor drivers / CAN-enabled gimbal motors
- Main motor power supply suitable for the selected drivers and motors
- USB connection for SimpleBGC GUI configuration
- UART connection between ESP32-S3 and BaseCam
- CAN_H / CAN_L bus between BaseCam and the motor drivers

Always verify the voltage limits, polarity and pinout of the actual hardware before applying power.

---

## 3. SimpleBGC GUI setup before installing the gimbal motors

### 3.1 Install and connect SimpleBGC GUI

Download the appropriate SimpleBGC32 GUI version from the official BaseCam Electronics website. Connect the BaseCam controller to the computer by USB, start the GUI, select the controller COM port and press **Connect**.

After connection, confirm that the controller parameters can be read correctly before changing the configuration.

Official downloads: https://www.basecamelectronics.com/downloads/

### 3.2 Check controller and GUI firmware

Open the **Upgrade / Firmware Upgrade** section and verify that the controller firmware and GUI version are compatible with the hardware being used. Update them when required for the selected BaseCam controller and CAN modules.

Do not interrupt power or USB communication while firmware is being written.

### 3.3 Configure the serial control input

Open:

**RC Settings → Input Configuration**

Configure the input used for ESP32 communication so the controller accepts commands from the **Serial API**. In the reference configuration, `RC_ROLL` is assigned to **Serial port (Serial API, etc.)**.

### 3.4 Main IMU selection

Open:

**Hardware → Main IMU Sensor**

For a controller with an onboard IMU, the onboard sensor can be enabled as a fallback when the main external IMU is not detected. Boards without an onboard IMU should be configured according to their actual sensor arrangement.

### 3.5 Serial communication speed

In the **Hardware** tab, locate the serial connection settings and configure:

- Main serial port speed: **115200 baud**
- RC_SERIAL port speed: **115200 baud**

The ESP32 UART configuration must use the same communication speed.

### 3.6 IMU calibration

Before final motor installation, calibrate the IMU according to the BaseCam procedure. Place the controller or IMU on a stable surface, calibrate one orientation, then repeat for additional axes/orientations. Multiple correctly calibrated positions improve orientation accuracy.

After changing settings, write/save the configuration to the controller and verify that it remains after reconnecting the GUI.

---

## 4. CAN motor and driver preparation

The CAN motor drivers must be correctly prepared before the complete gimbal is powered and commissioned.

### 4.1 CAN network

The BaseCam controller communicates with the motor drivers over a differential CAN bus:

- `CAN_H` → `CANH` of all CAN motor drivers
- `CAN_L` → `CANL` of all CAN motor drivers
- motor-driver power positive → appropriate DC supply positive
- motor-driver GND → power supply ground

Use a twisted pair for `CAN_H` and `CAN_L` where practical. Keep the wiring organized and verify CAN termination requirements for the actual hardware topology.

Each motor/driver must have the correct CAN address/ID for its role in the system. Do not power the system until duplicate or incorrect IDs have been resolved.

### 4.2 CAN driver firmware update

For BaseCam CAN_DRV modules, connect the modules to the main controller over CAN and provide the required main power supply. CAN_DRV modules are not powered from USB alone.

In SimpleBGC32 GUI:

1. Turn the motors OFF.
2. Open **Upgrade → Connected modules**.
3. Press **Refresh** and confirm that the required CAN module appears.
4. Select the module and check its hardware, firmware and bootloader information.
5. Select the firmware file intended for that exact hardware revision.
6. Press **Flash** and wait until the operation finishes.
7. Restart/reconnect the controller.
8. Return to **Connected modules**, refresh the list and verify the new firmware version.

**Important:** flashing firmware intended for different hardware can make a CAN module unusable. Always identify the exact driver revision before flashing.

Official BaseCam CAN Driver information: https://www.basecamelectronics.com/can_driver/

### 4.3 Motor parameters

After the drivers are detected, configure the motor-specific parameters required by the selected hardware, including motor pole count, encoder configuration and motor electrical parameters where applicable. The reference hardware used 28-pole motors, but this value must not be copied blindly to a different motor.

Perform resistance/inductance and encoder calibration only according to the requirements of the actual motor/driver combination.

---

## 5. Direct ESP32-S3 connection — one ESP32

In the direct configuration, a single ESP32-S3 performs two jobs:

- reads the Roll, Pitch and Yaw joystick inputs using ADC-capable GPIO pins;
- sends processed gimbal commands to the BaseCam controller through UART and the BaseCam Serial API.

Reference signal mapping used by the original implementation:

| Function | ESP32-S3 | Destination |
|---|---|---|
| Joystick 1 signal | GPIO6 | Joystick SIG |
| Joystick 2 signal | GPIO7 | Joystick SIG |
| Joystick 3 signal | GPIO15 | Joystick SIG |
| UART RX | GPIO18 | BaseCam UART1_TX |
| UART TX | GPIO17 | BaseCam UART1_RX |
| Joystick supply | 3V3 | All joystick VCC |
| Ground | GND | Joysticks + BaseCam GND |

UART is cross-connected: ESP32 RX connects to BaseCam TX and ESP32 TX connects to BaseCam RX.

The exact GPIO mapping will be reviewed when the modified GEC Engineering firmware is prepared. Therefore this table currently documents the reference hardware arrangement rather than defining a permanent firmware pinout.

### Direct-control operating principle

After startup, the ESP32 establishes serial communication with the BaseCam controller. It continuously samples the three joystick channels, converts the raw ADC readings into Roll/Pitch/Yaw commands, applies the selected operating mode and transmits the resulting control values through the BaseCam Serial API.

No firmware source code is included in the repository at this stage.

---

## 6. Bluetooth Low Energy control — two ESP32-S3 boards

The wireless version separates the user controls from the gimbal electronics.

### Controller ESP32-S3

The first ESP32-S3 is located in the handheld controller. It reads the three joystick channels and converts them into Roll, Pitch and Yaw control values. These values are transmitted wirelessly over **Bluetooth Low Energy (BLE)**.

The handheld controller can be powered from a suitable 5 V USB source or battery-powered supply.

### Gimbal ESP32-S3

The second ESP32-S3 is installed on the gimbal and acts as the BLE receiver. It receives the three control values, validates/parses them and forwards the corresponding control commands to the BaseCam controller through UART and the Serial API.

The BaseCam-to-CAN motor wiring remains essentially the same as in the direct configuration. The main difference is that the joystick connections are moved from the gimbal-side ESP32 to the handheld ESP32.

### BLE data flow

`Joysticks → Controller ESP32-S3 → BLE → Gimbal ESP32-S3 → UART / Serial API → BaseCam → CAN → Motor Drivers`

The final modified firmware will later add the exact BLE service/characteristic definition, packet format, connection recovery and failsafe behavior.

---

## 7. Speed and angle control modes

The system can operate using two different joystick interpretations.

### Angle mode

In **Angle** mode, joystick input commands a target angular position. Moving the joystick changes the requested Roll, Pitch or Yaw angle; the BaseCam stabilization system drives the corresponding axis toward that target.

This mode is useful when the operator wants direct positional control and expects the gimbal to hold the commanded orientation after the control input returns to its neutral behavior, depending on the configured RC settings.

### Speed mode

In **Speed** mode, joystick displacement commands rotational speed rather than an absolute angle. The farther the joystick is moved from center, the faster the selected axis rotates. Returning the joystick to the neutral region commands approximately zero rotation.

This mode is convenient for smooth camera panning and continuous manual movement.

The selected mode must be consistent between the SimpleBGC configuration and the firmware implementation. Dead-band/dead-zone settings can be used to prevent small ADC variations around joystick center from causing unwanted motion. BaseCam's motor-coordinate/servo-related options can also affect how commands are interpreted and should be configured for the specific mechanical system.

---

## 8. Firmware status

The original open-source implementation demonstrates both direct and BLE control, but **its source code is not being republished here unchanged**.

The firmware for this repository will be published after it has been modified for the GEC Engineering hardware configuration and reviewed/tested. Planned work includes a defined pin map, joystick calibration, filtering/dead-zone handling, BLE connection management, failsafe behavior and clearer configuration of operating modes.

---

## 9. General recommendations

- Verify every connector pinout with the documentation for the exact board revision.
- Check power polarity before connecting the battery or external DC supply.
- Configure and test the controller with motors disabled before the first powered motor test.
- Confirm unique CAN IDs and correct CAN_H/CAN_L wiring before enabling the motor drivers.
- Use twisted CAN_H/CAN_L wiring and appropriate CAN termination for the final harness.
- Secure the gimbal mechanically before motor calibration.
- Start testing with conservative motor power/current settings.
- Verify joystick center values and dead zone before allowing full-range motion.
- In the wireless version, implement a failsafe so loss of BLE communication cannot leave a non-zero movement command active.
- Keep a known-good BaseCam configuration backup before changing firmware or motor parameters.

---

## 10. Open-source acknowledgment

The ESP32 control concept and initial software approach used as a development reference for this project are based in part on **ESP32 + BaseCam Bluetooth Gimbal Control** by **Austin Allen / Elation Sports Technologies LLC**, released under the MIT License.

Original project:
https://www.hackster.io/ElationSportsTechnologies/esp32-basecam-bluetooth-gimbal-control-6ec7af

Original repository:
https://github.com/TheESTest/BaseCam-ESP32-Controller

BaseCam / SimpleBGC is a product ecosystem of BaseCam Electronics. Official BaseCam documentation should be used as the primary reference for controller firmware, GUI operation and CAN module procedures.

This repository documents the GEC Engineering integration, hardware-specific adaptation and subsequent firmware development. See [ATTRIBUTION.md](ATTRIBUTION.md) for details.
