CTRL — Stage 1 native monitoring (Qt 6.12 / C++20 / MinGW 64-bit)

LIVE BACKEND
- Windows battery percentage, status, source and remaining-time estimate.
- Native battery-device discharge rate, design/full capacities and health when reported.
- Existing Windows CPU utilization and memory monitoring preserved.
- Integrated read-only ASUS firmware CPU temperature query when the installed OEM driver
  and firmware expose it. This is not a direct AMD Tdie reading.
- Adaptive worker polling: 2 s Overview/Hardware, 6 s other pages, 30 s minimized/hidden.
- Missing readings show Unavailable. No external monitoring application is required.

NOT IMPLEMENTED IN THIS STAGE
- Ryzen 7735HS measured CPU package power (no verified safe provider included).
- GPU/fan monitoring and hardware/process/power controls; existing controls remain demos.
- Hard-coded fan RPM has been removed.

BUILD / CHECK
Use the existing Qt Creator Qt 6.12 MinGW kit, configure and build.
With BUILD_TESTING=ON: ctest --test-dir build-stage1 --output-on-failure
Read-only backend check: build-stage1/appasus_control.exe --telemetry-check
Qt and MinGW runtime DLLs must be available, as for any Qt application.
For distribution use the Qt windeployqt tool and comply with Qt licensing.

See docs/native-monitoring.md for data units, failure handling, OEM signed-driver
requirements, permissions, protocol references, licensing and verification limitations.
Stage 1 changes no system settings, installs no drivers and makes no network requests.
