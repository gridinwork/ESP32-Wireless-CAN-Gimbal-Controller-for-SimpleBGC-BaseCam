# Illustrated Setup Guide

This visual companion to the main README documents the reference hardware and SimpleBGC/BaseCam configuration used for the ESP32-S3 gimbal-control project. Firmware source code is intentionally not included yet.

> Reference images originate from the Austin Allen / Elation Sports Technologies project published on Hackster.io / Instructables. See `ATTRIBUTION.md`.

## 1. System overview

The reference prototype combines ESP32-S3 control, a SimpleBGC/BaseCam controller, three CAN-connected gimbal motors and a 4S power source. The final GEC Engineering implementation may use a different mechanical arrangement while retaining the same control architecture.

![Reference ESP32 and BaseCam gimbal-control system](https://content.instructables.com/FN8/KDHB/LZH05JMA/FN8KDHBLZH05JMA.jpg?auto=webp&fit=bounds&frame=1&height=1024&height=300&width=1024auto%3Dwebp)

## 2. SimpleBGC firmware and GUI preparation

Connect the BaseCam controller by USB, select the correct COM port and verify communication before changing parameters. Use the Firmware Upgrade page to check controller/GUI compatibility and update only when appropriate for the exact board revision.

![SimpleBGC firmware upgrade page](https://content.instructables.com/FX9/KS0T/LXRK8FSE/FX9KS0TLXRK8FSE.png?auto=webp&fit=bounds&frame=1)

## 3. IMU configuration

In the Hardware tab, configure the Main IMU Sensor according to the controller and sensor arrangement. On hardware that includes a suitable onboard IMU, the onboard sensor can be enabled as a fallback if the main IMU is not detected.

![SimpleBGC Main IMU configuration](https://hackster.imgix.net/uploads/attachments/1747324/main_imu_sensor_-_red_gbJ5YyKhgr.png?auto=compress%2Cformat&fit=max&h=960&w=1280)

Calibrate the IMU before final commissioning and save the resulting configuration to the controller.

## 4. CAN motors and driver modules

The gimbal motors are connected to dedicated CAN driver modules. Each driver requires the correct power supply, CAN_H/CAN_L connection and a unique CAN address. Motor phase wiring, driver mounting and CAN addressing must be checked before power-up.

The complete prototype below shows the controller, ESP32 hardware, power source and three connected motors.

![Reference CAN motor installation](https://content.instructables.com/FM5/MOML/LZCPU6LL/FM5MOMLLZCPU6LL.jpg?auto=webp&fit=bounds&frame=1&height=1024&height=300&width=1024auto%3Dwebp)

## 5. CAN module detection and firmware

After the CAN drivers are powered and connected, open the CAN/Connected Modules section in SimpleBGC GUI and verify that every required driver is detected. Each module must have the expected ID before motor calibration or operation.

![Detected CAN modules in SimpleBGC GUI](https://hackster.imgix.net/uploads/attachments/1747339/can_modules_scan_devices_markup_MBQV1EI2MD.png?auto=compress%2Cformat&fit=max&h=960&w=1280)

Only flash firmware intended for the exact CAN-driver hardware revision. After flashing, reconnect and confirm the reported firmware version before proceeding.

## 6. Direct connection — one ESP32-S3

In the direct configuration, one ESP32-S3 reads the three joystick channels and sends processed Roll/Pitch/Yaw commands to BaseCam over UART/Serial API. The BaseCam controller then commands the three CAN motor drivers.

![Direct ESP32-S3 to BaseCam wiring](https://content.instructables.com/FXC/8LE9/LZCPU8J9/FXC8LE9LZCPU8J9.png?fit=bounds&frame=true&height=1024&width=1024)

The reference implementation uses GPIO6, GPIO7 and GPIO15 for the three joystick signals, GPIO18 for ESP32 RX and GPIO17 for ESP32 TX. UART RX/TX are cross-connected to the BaseCam serial interface. The final GEC Engineering pin map will be confirmed when the modified firmware is published.

A close view of the reference joystick/ESP32 arrangement is shown below.

![ESP32-S3 and joystick reference wiring](https://content.instructables.com/F5I/UCSY/LZCPU6GV/F5IUCSYLZCPU6GV.jpg?auto=webp&fit=bounds&frame=1&height=1024&height=300&width=1024auto%3Dwebp)

## 7. Bluetooth Low Energy — two ESP32-S3 boards

For wireless operation, the first ESP32-S3 reads the joysticks and sends Roll/Pitch/Yaw data over BLE. A second ESP32-S3 installed on the gimbal receives the data and forwards the corresponding commands to BaseCam through UART and the Serial API.

![BLE ESP32-S3 to BaseCam wiring](https://content.instructables.com/FMB/SVXN/LZCPU8N6/FMBSVXNLZCPU8N6.png?fit=bounds&frame=true&height=1024&width=1024)

Data path:

`Joysticks → Controller ESP32-S3 → BLE → Gimbal ESP32-S3 → UART / Serial API → BaseCam → CAN → Motor Drivers`

The gimbal-side electronics remain connected to BaseCam in essentially the same way as the direct version; the joystick inputs move to the handheld/client ESP32.

## 8. Controller hardware reference

The BaseCam controller and ESP32 are mounted together in the reference prototype. This arrangement is useful for bench testing, but the final mechanical installation can place the ESP32 and BaseCam controller elsewhere in the gimbal structure.

![BaseCam controller reference installation](https://content.instructables.com/FML/2Q7Y/LZCPU6DU/FML2Q7YLZCPU6DU.jpg?auto=webp&fit=bounds&frame=1&height=1024&height=300&width=1024auto%3Dwebp)

## 9. Angle and Speed modes

**Angle mode** interprets joystick input as a requested angular position. **Speed mode** interprets joystick displacement as requested rotational speed. The selected mode must match the SimpleBGC configuration and the ESP32 firmware logic. Dead-band settings are useful for suppressing small joystick/ADC variations around center.

## 10. General recommendations

Verify board revision, connector pinout, CAN polarity, supply voltage and CAN IDs before applying motor power. Perform the first configuration with motors disabled, keep a backup of a known-good SimpleBGC profile, use twisted CAN_H/CAN_L wiring with appropriate termination, and use a communication failsafe in the BLE version.
