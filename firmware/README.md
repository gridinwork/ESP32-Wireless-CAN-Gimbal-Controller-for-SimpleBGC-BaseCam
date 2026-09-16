# Firmware

Modified ESP32-S3 firmware for the ESP32 Wireless CAN Gimbal Controller for SimpleBGC / BaseCam project.

Two configurations are supported:

- **Direct mode:** one ESP32-S3 reads three joystick axes and communicates with SimpleBGC/BaseCam over UART / Serial API.
- **BLE mode:** a handheld ESP32-S3 reads the joysticks and sends Roll/Pitch/Yaw values over BLE to a second ESP32-S3 installed on the gimbal. The gimbal-side ESP32 forwards the commands to BaseCam over UART.

## Firmware projects

### ESP32_BLE_Controller
Handheld BLE joystick transmitter.

Target board: ESP32-S3-DevKitC-1.
Joystick GPIO: GPIO6 / GPIO7 / GPIO15.
BLE packet: ASCII `x,y,z`.
Send interval: 20 ms.

### ESP32_BaseCam_Gimbal
Gimbal-side BaseCam controller and BLE receiver. It can also be used alone in Direct mode.

UART to BaseCam:
- RX: GPIO18
- TX: GPIO17
- 115200 8N1

The modified firmware adds BLE reconnect handling, communication failsafe, packet validation, configurable Direct/BLE and Angle/Speed modes, joystick center calibration, dead zone, filtering and command clamping.

## Build environment

- PlatformIO Core 6.2.0 used for validation
- `espressif32@6.7.0`
- Arduino-ESP32 2.0.16
- Board: `esp32-s3-devkitc-1`

Both modified projects were reported as successfully compiled in the supplied build report.

See [README_RU.md](README_RU.md) for Russian documentation, [CHANGELOG_RU.md](CHANGELOG_RU.md) for the detailed modification list, and [BUILD_REPORT_RU.md](BUILD_REPORT_RU.md) for build validation.

The original copyright and license notices present in the modified source are retained.