# Stage 1 verification — 17 November 2026

Implemented: native battery-device monitoring, existing Windows CPU/RAM readings,
ASUS firmware CPU temperature, QML bindings and adaptive background polling.
External monitoring providers have been removed from runtime code.

## Verification performed

* Built with Qt 6.12.0, GNU MinGW 13.1.0 64-bit, C++20 and CMake/Ninja.
* CTest: `native_monitoring_readings` passed (1/1). Tests cover mWh/mW conversion,
  capacity-derived health, unknown rate, positive/zero rate, charging, relative units,
  absent status and missing capacities.
* The read-only `--telemetry-check` backend check reported:

| Measurement | Observed value |
| --- | --- |
| Source / status | Battery / Discharging |
| Charge | 28% |
| Discharge power | 14.575 W |
| Design / full capacity | 90.006 / 64.296 Wh |
| Battery health | 71.435% |
| Windows remaining time | 73 minutes |
| ASUS firmware CPU temperature | 48°C |
| CPU utilization | 49.512% (during build activity) |
| RAM used | 16.649 GiB |
| CPU package power | Unavailable |

These are observed samples, not constants in the application. Temperature was retrieved
successfully through the installed ASUS interface; it has not been independently calibrated
against a reference sensor. The QML was compiled by Qt's QML cache compiler. Interactive
visual inspection, AC reconnection, battery hot-plug/error paths, and long-duration idle CPU
measurements were not performed. The low idle CPU target is not claimed as verified.
Optional Vulkan headers were not found; they did not prevent the Qt Quick build.

## Source files modified or added

* `D:\projects\asus_control\asus_control\CMakeLists.txt`
* `D:\projects\asus_control\asus_control\main.cpp`
* `D:\projects\asus_control\asus_control\Main.qml`
* `D:\projects\asus_control\asus_control\backend\SystemMonitor.h`
* `D:\projects\asus_control\asus_control\backend\SystemMonitor.cpp`
* `D:\projects\asus_control\asus_control\backend\BatteryMonitor.h`
* `D:\projects\asus_control\asus_control\backend\BatteryMonitor.cpp`
* `D:\projects\asus_control\asus_control\backend\BatteryReadings.h`
* `D:\projects\asus_control\asus_control\hardware\AsusSensors.h`
* `D:\projects\asus_control\asus_control\hardware\AsusSensors.cpp`
* `D:\projects\asus_control\asus_control\tests\NativeMonitoringTest.cpp`
* `D:\projects\asus_control\asus_control\README.txt`
* `D:\projects\asus_control\asus_control\docs\native-monitoring.md`
* `D:\projects\asus_control\asus_control\docs\stage1-verification.md`

Build output and original source backups are under
`D:\projects\asus_control\asus_control\build-stage1`.

## Remaining work

CPU package power has no verified safe provider in this integration and stays unavailable.
GPU/fan telemetry and hardware controls are later stages. Existing process/profile/tuning
demo controls have not been converted into real optimizations during Stage 1.
Next priority is Stage 2: native process enumeration, identity-based protection, confirmed
termination, startup management and accurately labeled persistent rules.
