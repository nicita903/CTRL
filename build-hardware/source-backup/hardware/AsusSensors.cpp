#include "AsusSensors.h"
#ifdef Q_OS_WIN
#include <windows.h>
#include <array>
#endif

CpuSensorSample AsusSensors::sample() const {
    CpuSensorSample result;
#ifdef Q_OS_WIN
    // Uses the installed ASUS System Control Interface driver, not a bundled driver.
    // Protocol reference and licensing notes: docs/native-monitoring.md.
    HANDLE device = CreateFileW(L"\\\\.\\ATKACPI", GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (device == INVALID_HANDLE_VALUE) {
        result.temperatureSource = QStringLiteral("Unavailable (ASUS interface error %1)").arg(GetLastError());
        return result;
    }
    // DSTS, argument length, CPU temperature device ID, reserved argument.
    // Deliberately no INIT, DEVS, GPU queries, SMU access or register writes.
    std::array<DWORD, 4> request{0x53545344, 8, 0x00120094, 0};
    std::array<DWORD, 4> response{};
    DWORD bytes = 0;
    const bool ok = DeviceIoControl(device, 0x0022240c, request.data(), sizeof(request),
                                    response.data(), sizeof(response), &bytes, nullptr);
    const DWORD error = ok ? ERROR_SUCCESS : GetLastError();
    CloseHandle(device);
    if (!ok || bytes < sizeof(DWORD)) {
        result.temperatureSource = QStringLiteral("Unavailable (ASUS temperature query error %1)").arg(error);
        return result;
    }
    // Presence flag plus degrees Celsius; reject unsupported methods and extra flags.
    const DWORD value = response[0];
    const DWORD degrees = value & 0xffff;
    if ((value & 0xffff0000) != 0x00010000 || degrees == 0 || degrees > 125) {
        result.temperatureSource = QStringLiteral("Unavailable (ASUS CPU temperature endpoint unsupported)");
        return result;
    }
    result.temperatureC = degrees;
    result.temperatureSource = QStringLiteral("ASUS firmware CPU sensor (not direct AMD Tdie)");
#endif
    return result;
}
