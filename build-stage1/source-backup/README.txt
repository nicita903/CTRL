VOLT CONTROL — STAGE 2 (Qt 6.12 / MinGW, Windows 10)

WHAT IS LIVE
- Battery percentage, source, remaining time: Windows API (previous Stage 1).
- CPU utilization and RAM: Windows API (previous Stage 1).
- Discharge power: local Windows root\\WMI BatteryStatus.DischargeRate / 1000, IF FIRMWARE REPORTS IT.
- CPU temperature: optional LibreHardwareMonitor WMI Sensor (CPU Package / Tctl-Tdie), IF RUNNING.
- Unavailable values stay unavailable; they are NEVER simulated as real telemetry.

INSTALL
1. Backup your working Main.qml, CMakeLists.txt, backend/SystemMonitor.h and .cpp.
2. Copy Main.qml and CMakeLists.txt into your actual project root.
3. Copy backend/SystemMonitor.h and SystemMonitor.cpp into your backend directory.
4. Keep your existing main.cpp unchanged: it already creates SystemMonitor and exposes it as 'systemMonitor'.
5. Qt Creator > Build > Run CMake; then Build > Rebuild All; Run.

CPU TEMPERATURE
Windows does not reliably expose Ryzen Tdie/Tctl through a standard API.
To enable CPU temperature, install/run LibreHardwareMonitor from its official repository,
start it with the needed sensor permissions, and ensure it exposes root\\LibreHardwareMonitor WMI.
If unavailable, the temperature reads 'Unavailable'. This app does NOT start a driver itself.

BATTERY POWER
Unplug the charger to observe discharge rate. The value is W and comes from firmware.
If firmware reports zero/unknown, the app shows '-- W'. An AC-connected laptop
normally shows charging status, not a discharge figure.

SAFETY
No process termination, blocklist, power, voltage, fan, or thermal-control operations are activated.
The existing controls remain visual simulations.
WMI runs off the UI thread and is polled every ~6 s to limit overhead.

TROUBLESHOOT
PowerShell check on the target Windows machine:
  Get-CimInstance -Namespace root/WMI -ClassName BatteryStatus | Select Discharging,DischargeRate
Optional CPU sensor check:
  Get-CimInstance -Namespace root/LibreHardwareMonitor -ClassName Sensor | Where-Object { $_.SensorType -eq 'Temperature' } | Select Name,Value
If queries return no results, providers are missing or the firmware doesn't supply the sensor.
