# Native hardware-control verification — 17 November 2026

## Implemented

Native model and capability discovery; five capability states; read-only ASUS WMI/ATK
inspection; CPU/GPU fan telemetry on the existing adaptive worker; staged firmware
performance modes and cooling aliases; validated independent fan editors; staged factory
curve restoration; Windows AC/DC boost policies; guarded Eco/Standard switching; opt-in
NVML power controls; Apply/Discard, backups, per-setting outcomes and supported rollback.
The first write requires an explicit Apply confirmation. No hardware write was performed
by development tooling. Other monitoring continues to use the native Stage 1 backend.

## Observed on this machine (read-only)

* SMBIOS: **ASUSTeK COMPUTER INC. / ASUS TUF Gaming A15 FA507NV_FA507NV**.
* ASUS ATK interface responds; `root\WMI:AsusAtkWmi_WMNB` exists outside the sandbox.
  The sandbox itself denies WMI discovery; this is distinct from a missing OEM driver.
* Fan samples: CPU **2100 RPM**, GPU **2000 RPM** (other samples varied as expected).
* Performance endpoint is present and returned status **3**. This is not interpreted as
  the active performance mode. Mode writes have acknowledgement only.
* Windows enumerated boost values **0/1/2/3**; initial inspected AC and DC values were 0.
* GPU Eco flag **0**, hybrid MUX flag **1**: Standard mode is readable. Eco switching has
  additional display/client safety checks at Apply time.
* Factory reads need a 64-byte output buffer on this driver. The first 16 bytes hold the
  expected curve; the remaining 48 bytes were all zero. The decoder rejects other layouts.

| Selected factory mode | CPU curve | GPU curve |
| --- | --- | --- |
| Balanced | Valid, editor/restoration prerequisites pass | Disabled: decreasing duty values |
| Turbo | Valid, editor/restoration prerequisites pass | Valid, editor/restoration prerequisites pass |
| Silent | Disabled: repeated final temperature | Disabled: repeated final temperature |

Observed Balanced CPU factory points (°C/%):
45/2, 49/9, 54/15, 68/18, 74/22, 79/25, 84/32, 89/37.
These are captured firmware values, not application constants or manufactured readings.

## Disabled or unavailable

* SPL/sPPT/fPPT and GPU thermal/Dynamic Boost endpoints respond, but no verified writable
  range is available in this integration. Their raw status is not presented as a current
  power/thermal limit. The controls remain disabled.
* CPU thermal endpoint returns Windows error 1. 65/70/75°C firmware targets and software
  thermal regulation remain unavailable.
* GPU power/temperature snapshots require explicit user NVML inspection; that inspection
  was not run during development, to avoid waking the dGPU unnecessarily. Laptop power
  constraint/write support remains unverified.
* Ultimate/restart mode, automatic Optimized switching, overclocking and a privileged
  helper are not implemented. Permission-denied operations do not elevate the GUI.
* Active ASUS performance mode and prior custom curves cannot be read back reliably;
  exact automatic rollback of those values is unavailable. Firmware presets and validated
  factory curves are available as explicitly selected recovery actions.

## Verification performed

* Qt **6.12.0**, MinGW **13.1.0 64-bit**, C++20, CMake/Ninja build succeeded after fixing
  a Windows DWORD/QVariant conversion issue and the observed padded curve transport.
* CTest **2/2 passed**: native battery conversions plus hardware safety rules covering
  valid/malformed curves, decreased cooling, fixed temperature bins, overflow, negative/
  fractional/NaN values, padding validation, RPM units and exact model restrictions.
* Read-only native hardware diagnostic exited **0**. Capture:
  `D:\projects\asus_control\asus_control\build-hardware\hardware-verified.json`.
* Offscreen Qt Quick check exited **0**, using software rendering. All three Hardware &
  Cooling tabs were rendered and inspected; the error log was empty. Captures:
  `D:\projects\asus_control\asus_control\build-hardware\ui-tab-0.png` through
  `ui-tab-2.png`. These are software-rendered checks, not live control tests.
* No firmware mode, curve, GPU state or Windows power policy was changed during testing.
  Every control write, rollback, post-write readback, first-use confirmation and factory
  restoration still requires physical user verification, one supported control at a time.
* Long-duration idle CPU/battery cost and physical GPU sleep behavior were not measured.
  Missing optional Vulkan headers did not prevent compilation.

## Exact source files changed or added in this phase

* `D:\projects\asus_control\asus_control\CMakeLists.txt`
* `D:\projects\asus_control\asus_control\main.cpp`
* `D:\projects\asus_control\asus_control\Main.qml`
* `D:\projects\asus_control\asus_control\backend\SystemMonitor.h`
* `D:\projects\asus_control\asus_control\hardware\AsusSensors.h`
* `D:\projects\asus_control\asus_control\hardware\AsusSensors.cpp`
* `D:\projects\asus_control\asus_control\hardware\HardwareCapabilities.h`
* `D:\projects\asus_control\asus_control\hardware\AsusProtocol.h`
* `D:\projects\asus_control\asus_control\hardware\AsusWmiController.h`
* `D:\projects\asus_control\asus_control\hardware\AsusWmiController.cpp`
* `D:\projects\asus_control\asus_control\hardware\CpuController.h`
* `D:\projects\asus_control\asus_control\hardware\CpuController.cpp`
* `D:\projects\asus_control\asus_control\hardware\FanController.h`
* `D:\projects\asus_control\asus_control\hardware\FanController.cpp`
* `D:\projects\asus_control\asus_control\hardware\NvidiaController.h`
* `D:\projects\asus_control\asus_control\hardware\NvidiaController.cpp`
* `D:\projects\asus_control\asus_control\hardware\HardwareManager.h`
* `D:\projects\asus_control\asus_control\hardware\HardwareManager.cpp`
* `D:\projects\asus_control\asus_control\tests\HardwareRulesTest.cpp`
* `D:\projects\asus_control\asus_control\README.txt`
* `D:\projects\asus_control\asus_control\docs\native-monitoring.md`
* `D:\projects\asus_control\asus_control\docs\hardware-control.md`
* `D:\projects\asus_control\asus_control\docs\hardware-verification.md`

Build output is isolated under `D:\projects\asus_control\asus_control\build-hardware`.
Original sources were backed up under its `source-backup` directory before editing.
Existing unrelated changes and the earlier build directory were preserved.

See [hardware-control.md](hardware-control.md) for transaction semantics, recovery
limitations, permissions, signed OEM driver requirements and primary API/protocol sources.
