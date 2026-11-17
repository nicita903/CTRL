# CTRL native monitoring — Stage 1

This is the Stage 1 baseline. Current staged hardware writes and their limitations are
documented in [hardware-control.md](hardware-control.md).

## Data sources

* Percentage, charge state, AC/battery source and Windows remaining-time estimate:
  `GetSystemPowerStatus`. Unknown source is displayed explicitly, not interpreted as battery.
* CPU utilization: deltas from `GetSystemTimes`. Memory: `GlobalMemoryStatusEx`.
* Battery rate and capacities: SetupAPI battery-interface enumeration, read-only handles,
  `IOCTL_BATTERY_QUERY_TAG`, `IOCTL_BATTERY_QUERY_INFORMATION`,
  `IOCTL_BATTERY_QUERY_STATUS`. Only system batteries are selected; short-term UPS
  batteries are excluded. Multiple identified internal packs are unsupported in this stage.
  Discovery is repeated each sample, so battery removal or tag changes recover without restart.
* Negative milliwatts are converted to positive discharge watts only when discharging.
  Unknown, zero, positive charging rates and failed requests stay unavailable.
  Relative capacity/rate units are never labeled Wh/W. Health is full/design capacity,
  and is not hard-coded to the owner's estimated 72%. Health may exceed 100% when
  firmware reports full capacity above nominal design capacity.
* CPU temperature: read-only DSTS query of ASUS CPU endpoint `0x00120094`, through
  the existing `\\.\ATKACPI` device. The returned presence flag, response size and
  Celsius range are validated on every sample. This is ASUS's firmware CPU temperature,
  **not a claim of direct AMD Tctl/Tdie measurement**. Unsupported/error responses clear
  the previous value. No generic ACPI thermal zone is substituted for CPU temperature.
* CPU package power: unavailable. No verified integrated provider for the 7735HS is
  included. ASUS PPT settings are limits, not measurements. No SMU/register probing,
  Ryzen Master desktop compatibility assumptions, or vulnerable low-level driver is used.

The application neither connects to other monitoring applications nor launches them.
It makes no network requests, installs no driver and issues no hardware-control writes.
No NVIDIA API is initialized by Stage 1, so these readers do not query/wake the dGPU.

## Driver, permissions and licensing

Battery access uses the Windows battery stack already installed with Windows.
ASUS temperature access requires the genuine, signed **ASUS System Control Interface**
driver supplied for the laptop by ASUS/Windows Update. Driver support and endpoint
availability must be verified on the actual FA507NV firmware. Install/update only through
ASUS's support page or Windows Update, retaining Windows signature enforcement.
CTRL does not redistribute this proprietary driver; ASUS's distribution/license terms apply.
Armoury Crate and G-Helper processes are not required.

The driver device is opened with read/write access for the ASUS transport;
the only command in this integration is a fixed read request. No DEVS/INIT command or
arbitrary endpoint/IOCTL API is exposed to QML. A device-access denial is surfaced as
Unavailable with the Windows error number. Do not run the whole GUI elevated as a workaround.
A future privileged implementation requires a separate authenticated, narrowly scoped helper.
As with any kernel driver, only keep a supported, security-patched OEM release installed.

Protocol facts were checked against the upstream G-Helper ASUS implementation (GPL-3.0)
and Linux ASUS WMI driver (GPL-2.0-or-later). CTRL's C++ reader is independently written;
no source from either project, driver binary, or library is copied or linked.
No new third-party component is included by this stage; existing Qt licensing
and deployment obligations still apply.

Primary references:

* https://learn.microsoft.com/en-us/windows/win32/power/enumerating-battery-devices
* https://learn.microsoft.com/en-us/windows/win32/power/battery-information-str
* https://learn.microsoft.com/en-us/windows/win32/power/battery-status-str
* https://github.com/seerge/g-helper/blob/main/app/AsusACPI.cs
* https://github.com/torvalds/linux/blob/master/drivers/platform/x86/asus-wmi.c

## Scheduling, UI and recovery

Device discovery and driver calls run on a dedicated worker, with queued delivery to the
GUI. Foreground Overview/Hardware poll every 2 seconds; other pages every 6 seconds;
minimized/hidden windows every 30 seconds. A condition variable permits prompt shutdown
and interval changes without 250 ms wakeups. Synchronous OEM calls are outside CTRL's
control; shutdown waits for an in-flight call to finish safely.

The terminal theme, navigation, five modules and animations are retained. Native bindings
replace temperature/power reads and add battery capacity/health/status. Fake fan RPM is
removed. The interval field describes the adaptive policy. Later-stage tuning and process
controls remain the existing explicitly marked demo UI; they do not optimize the machine.

There are no settings changes to restore in Stage 1. Before editing, original source files
were copied into `build-stage1/source-backup`. Existing unrelated source/build changes
were retained. Build output is isolated in `build-stage1`.

## Verification

Configure with Qt 6.12 / C++20 / MinGW 64-bit and BUILD_TESTING=ON, then build.
Run `ctest --test-dir build-stage1 --output-on-failure` for conversion/unknown-value tests.
These test fixtures are synthetic and are never used by the application's telemetry.
Run `build-stage1/appasus_control.exe --telemetry-check` from a Qt/MinGW environment
to collect JSON from the exact backend without creating a window or a rendering context.
`-1` represents unavailable numerical telemetry; source fields explain the integration.
This diagnostic reads sensors only; it does not change settings.

Hardware acceptance: compare the percentage/source with Windows, compare design/full
capacities with `powercfg /batteryreport`, test AC connect/disconnect, confirm actual
negative discharge rate while on battery, and compare firmware CPU temperature with ASUS
diagnostics. Do not disconnect power or run battery discharge experiments automatically.
An AC-connected check cannot verify discharging watts. Successful build/tests do not prove
sensor availability or the idle CPU target on a particular machine.
