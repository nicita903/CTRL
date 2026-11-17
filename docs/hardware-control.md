# CTRL native hardware control

This extends native monitoring without changing the framework or terminal theme. Hardware
writes are available only through staged, validated operations followed by an explicit Apply
confirmation. Startup, CLI diagnostics, telemetry, capability inspection and editor changes
never write hardware. Process and automatic profile modules remain their existing demos.

## Native interfaces and support

`AsusWmiController` opens the installed ASUS System Control Interface's ATKACPI device.
Its C++ API uses a fixed endpoint enum and exposes only performance mode, Eco flag and
16-byte fan-curve writes. QML cannot supply IOCTLs, device IDs, addresses or raw payloads.
The Windows transport envelope is implemented directly; Linux WMI calls are not invoked.
`root\WMI:AsusAtkWmi_WMNB` is also discovered, without calling its write methods.
Failure of this optional WMI discovery does not disable a working ATK driver interface.

SMBIOS is read using `GetSystemFirmwareTable`. Firmware writes are restricted to ASUS
FA507NV model identities, including `ASUS TUF Gaming A15 FA507NV_FA507NV`.
Every control reports SUPPORTED, UNSUPPORTED, PERMISSION REQUIRED, DRIVER UNAVAILABLE,
or ERROR. SUPPORTED means the read/identity/protocol prerequisites passed, **not that a
write has been physically verified**. Failures disable the affected control until inspection.

### Fans and performance modes

CPU/GPU fan endpoints report hundreds of RPM. Zero is displayed only for a valid zero
response. They share the existing adaptive telemetry worker; no second sensor timer exists.
QUIET/AUTO/COOL are labels for Silent/Balanced/Turbo, not invented firmware modes.
Turbo is refused unless Windows currently reports AC power.

The performance endpoint's status is **not treated as active-mode readback**. On the target
it returned 3. Current mode stays unavailable. A performance write requires a firmware
acknowledgement and is labeled accordingly; CTRL never claims measured mode readback or
automatic recovery of an unreadable previous mode. Users can explicitly select a known
firmware preset again. No guessed Full Speed or Manual mode is sent.

Factory curve selectors are Balanced=0, Silent=1 and Turbo=2; these differ from the
performance-write values Balanced=0, Silent=2, Turbo=1. All three pairs of factory curves
are inspected. Only a valid 16-byte response containing eight increasing Celsius values
and eight nondecreasing percentages in 0–100 is accepted. Invalid/unknown layouts remain
disabled; the app does not repair, guess or replace factory data with an example curve.
The target driver rejects a 16-byte output buffer, but a successful 64-byte read contains
the known leading 16-byte curve followed by 48 zero bytes. This observed padded transport
is accepted only when the entire suffix is zero; the extracted 16 bytes are still fully
validated. Other sizes/nonzero suffixes and failed calls are rejected. A read-only capture
confirmed the CPU curve's layout. Balanced GPU duty values decrease, and Silent curves
repeat the last temperature, so those editors remain disabled. Turbo CPU/GPU and Balanced
CPU curves pass validation. No malformed factory curve is silently corrected.

The independent CPU/GPU editors preserve firmware temperature bins and permit only duty
increases above each factory point. This deliberately does not allow quieter-than-factory
curves, arbitrary temperature ranges, direct fan-speed commands, fan stopping, calibration,
or bypassing EC protection. A preset change and curve edits cannot be staged together.

Because active mode cannot reliably be read, the curve editor has an **explicit base mode**.
A confirmed curve Apply validates all requested curves first, applies that base mode, then
writes each selected curve separately. The confirmation warns that mode selection may reset
old custom curves. Avoid using other firmware-control applications/Fn mode keys during a
transaction. There is no Windows firmware-wide lock against external writers.

Factory restoration stages the selected mode's firmware-provided CPU/GPU curves, then uses
the same confirmed write path. It does not claim to restore every BIOS setting. ASUS's
factory-curve query does not provide active custom-curve readback. Curve results report
firmware acknowledgement only; old custom curves cannot be reconstructed automatically.
The two fan operations can partially succeed, and individual outcomes remain visible.

### CPU policies and limits

Boost uses native PowrProf APIs. The active plan's AC/DC values and supported enumeration
are read before editing. DISABLED=0, ENABLED=1, AGGRESSIVE=2 and EFFICIENT=3 are exposed
only when Windows enumerates them. These are policy names; hardware CPPC interpretation
can differ. Applying one policy verifies the active plan identity, writes the selected AC
or DC value, activates the plan, and reads back. Failure attempts to restore its old value.
Permission failures are reported; the GUI is never silently elevated.

SPL, sPPT, fPPT, CPU thermal limit, GPU thermal target and Dynamic Boost endpoints are
inspected read-only. Endpoint presence alone does not prove a valid writable range or that
a status value is the current limit. No verified range query/model specification has been
integrated, so their sliders stay disabled. In particular, 65/70/75°C CPU targets are not
offered as firmware limits. Software thermal targeting also stays disabled without verified
adjustable power controls. No SMU, EC register probing or new kernel driver is used.

### GPU controls

Eco/Standard is offered only with a valid binary Eco flag and verified hybrid MUX state.
Eco Apply additionally opens an on-demand NVML session and requires successful queries
excluding active displays and both compute and graphics clients. Any unavailable query or
active client refuses the switch. No process is killed and no NVIDIA service is stopped.
NVML closes before power-gating. The flag is read back; failed readback is not reported as
success and does not trigger an unsafe automatic power-gating rollback.

GPU modes can reconfigure displays; confirmation is required. Ultimate/MUX writes and
restart workflows are disabled. Optimized automatic switching is not implemented.

NVML is loaded only from the Windows system directory and only after explicit NVIDIA
inspection or confirmed GPU operations. Startup/read-only diagnostics never initialize it.
Eco inspection skips NVML. Power/temperature are labeled on-demand snapshots, not live
telemetry. Standard mode alone does not establish GPU activity; it is shown as activity
unmeasured. GPU SLEEPING is shown when the firmware Eco flag confirms disabling the dGPU.

Power control requires the exact RTX 4060 Laptop device, a stable UUID, successful native
constraint/current/default-limit queries and a compatible set-limit export. Only integral
watt values inside the driver-reported range, capped at the native default limit, are
accepted. Readback is required; failure attempts to restore the previous milliwatt limit.
No overclocking, clock offsets, firmware modifications or invented laptop limits exist.
Lower power or stronger cooling can reduce temperature; staying below 75°C is not promised.

## Transactions, backups, permissions and recovery

Firmware/control I/O runs on a serialized background job. Pending edits remain on the GUI
thread and are copied into the worker. Apply rechecks identity/capabilities, validates curves
before mode writes, and commits a JSON backup before any write. Backup failure aborts the
transaction. Backups include the requested edits and inspected state and are stored in
`QStandardPaths::AppLocalDataLocation/hardware-backups/before-<UTC timestamp>.json`.
The application name is CTRL. There is no cloud/network access.

Per-setting results distinguish readback success, acknowledgement-only success, refusal and
failure. Operations are not atomic. Exact rollback is attempted only for readable Windows
boost values and NVML power limits. Fan factory restore and saved session boost/GPU values
are separately staged; unknown performance/custom-curve state is not invented. Saved GPU
power restore is offered only after explicit inspection of the same UUID and for a whole-W
saved value. Backups are reviewable JSON; arbitrary files are not accepted as write commands.

The GUI runs at the user's normal privilege level. There is no generic elevated helper and
no Administrator relaunch. If the installed driver or Windows/NVIDIA policy denies an
operation, it remains PERMISSION REQUIRED instead of being worked around. A future helper
must authenticate the user and implement only these fixed operations before privileged
paths can be enabled. Do not install unsigned drivers or disable signature enforcement.
The installed OEM driver is not redistributed; ASUS licensing applies. Qt deployment and
licensing requirements remain. NVML is provided by the installed NVIDIA display driver.

## Sources and licensing

The C++ implementation is independently written against protocol facts and official API
signatures. No G-Helper/Linux source or executable is copied or linked. G-Helper is GPL-3.0;
Linux ASUS WMI is GPL-2.0-or-later. They are protocol references, not runtime dependencies.

* [ASUS protocol reference](https://github.com/seerge/g-helper/blob/main/app/AsusACPI.cs)
* [ASUS GPU mode reference](https://github.com/seerge/g-helper/blob/main/app/Gpu/GPUModeControl.cs)
* [Linux ASUS firmware reference](https://github.com/torvalds/linux/blob/master/drivers/platform/x86/asus-wmi.c)
* [Windows boost policy](https://learn.microsoft.com/en-us/windows-hardware/customize/power-settings/options-for-perf-state-engine-perfboostmode)
* [Windows possible-value enumeration](https://learn.microsoft.com/en-us/windows/win32/api/powrprof/nf-powrprof-powerreadpossiblevalue)
* [NVML device queries](https://docs.nvidia.com/deploy/nvml-api/latest/api/group__nvmlDeviceQueries.html)
* [NVML device commands](https://docs.nvidia.com/deploy/nvml-api/620/api/group__nvmlDeviceCommands.html)

## Checks

Build with Qt 6.12 / MinGW 64-bit / C++20. Run CTest in `build-hardware` for native battery
and fan/identity validation tests. Test fixtures do not enter runtime telemetry.
`appasus_control.exe --hardware-check` prints read-only discovery JSON, with no NVML init.
`--telemetry-check` retains the original native monitoring diagnostic.
`--qml-check` renders the three hardware tabs offscreen using the software renderer,
saves `build-hardware/ui-tab-0.png` through `ui-tab-2.png`, and exits without applying edits.
The first hardware write must be a user's own Apply interaction in the GUI. It was not
performed automatically by development tooling.
