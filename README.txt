CTRL — Native ASUS hardware control (Qt 6.12 / C++20 / MinGW 64-bit)

Native monitoring is preserved: Windows battery state/power/capacities/health, CPU/RAM,
ASUS CPU temperature, CPU/GPU fan RPM, and firmware Eco-state inspection.

HARDWARE & COOLING
- Model/firmware capability discovery; unsupported controls are disabled with reasons.
- Staged ASUS Silent/Balanced/Turbo writes and QUIET/AUTO/COOL aliases (Turbo requires AC).
- Windows CPU boost policies independently for AC and battery.
- Factory CPU/GPU curve inspection for each mode; validated duty-only custom edits.
- Staged restoration of validated factory curves and captured session settings.
- Guarded Eco/Standard writes; ECO requires no NVIDIA displays/clients.
- Explicit on-demand NVML inspection and native GPU power limiting when supported.
- Apply confirmation, background writes, backups and per-operation results/readback.

The first firmware write must be the user's own Apply interaction in the application.
The implementation has not been physically write-tested during development.
ASUS active performance-mode/custom-curve readback is unavailable; acknowledgement
is clearly distinguished from readback verification.

CPU power/thermal ranges, GPU thermal/Dynamic Boost ranges, Ultimate/restart mode,
and automatic Optimized switching remain disabled. No overclocking or new driver.
Some firmware factory curves fail validation and are deliberately not editable.
Process and automatic profile modules remain their existing labeled demos.

BUILD / CHECK
Use the existing Qt Creator Qt 6.12 MinGW kit, configure and build.
CTest directory used here: build-hardware
Read-only CLI: appasus_control.exe --hardware-check or --telemetry-check
Software-rendered UI check: appasus_control.exe --qml-check
Qt/MinGW runtime DLLs must be on PATH or deployed with Qt's windeployqt tool.

See docs/hardware-control.md for architecture, safeguards, OEM driver requirements,
permissions, licensing, recovery limitations and native API references.
See docs/hardware-verification.md for observed capabilities and exact changed paths.
Original sources were copied into build-hardware/source-backup before this phase.
