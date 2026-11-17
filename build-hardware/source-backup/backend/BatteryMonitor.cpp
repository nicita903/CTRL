#include "BatteryMonitor.h"

#ifdef Q_OS_WIN
#include <windows.h>
#include <winioctl.h>
#include <setupapi.h>
#include <poclass.h>
#include <vector>
#include "BatteryReadings.h"

namespace {
// Battery device interface GUID; no third-party sensor provider is involved.
constexpr GUID batteryInterface{0x72631e54, 0x78a4, 0x11d0,
                               {0xbc, 0xf7, 0x00, 0xaa, 0x00, 0xb7, 0xb3, 0x2a}};
struct DeviceSet {
    HDEVINFO value = SetupDiGetClassDevsW(&batteryInterface, nullptr, nullptr,
                                        DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    ~DeviceSet() { if (value != INVALID_HANDLE_VALUE) SetupDiDestroyDeviceInfoList(value); }
};
struct Handle {
    HANDLE value;
    ~Handle() { if (value != INVALID_HANDLE_VALUE) CloseHandle(value); }
};
template<typename Input, typename Output>
bool query(HANDLE device, DWORD code, Input &input, Output &output) {
    DWORD bytes = 0;
    return DeviceIoControl(device, code, &input, sizeof(input), &output, sizeof(output),
                           &bytes, nullptr) && bytes >= sizeof(output);
}
}
#endif

BatterySample BatteryMonitor::sample() const {
    BatterySample result;
#ifdef Q_OS_WIN
    DeviceSet devices;
    if (devices.value == INVALID_HANDLE_VALUE) {
        result.source = QStringLiteral("Unavailable (battery discovery error %1)").arg(GetLastError());
        return result;
    }
    int systemBatteries = 0;
    for (DWORD index = 0; ; ++index) {
        SP_DEVICE_INTERFACE_DATA interfaceData{};
        interfaceData.cbSize = sizeof(interfaceData);
        if (!SetupDiEnumDeviceInterfaces(devices.value, nullptr, &batteryInterface,
                                         index, &interfaceData)) break;
        DWORD size = 0;
        SetupDiGetDeviceInterfaceDetailW(devices.value, &interfaceData, nullptr, 0, &size, nullptr);
        if (size < sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W)) continue;
        std::vector<unsigned char> storage(size);
        auto *detail = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W *>(storage.data());
        detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
        if (!SetupDiGetDeviceInterfaceDetailW(devices.value, &interfaceData, detail, size, nullptr, nullptr)) continue;
        Handle device{CreateFileW(detail->DevicePath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                  nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr)};
        if (device.value == INVALID_HANDLE_VALUE) {
            result.source = QStringLiteral("Unavailable (battery access error %1)").arg(GetLastError());
            continue;
        }
        ULONG timeout = 0, tag = 0;
        if (!query(device.value, IOCTL_BATTERY_QUERY_TAG, timeout, tag) || tag == BATTERY_TAG_INVALID) continue;
        BATTERY_QUERY_INFORMATION request{};
        request.BatteryTag = tag;
        request.InformationLevel = BatteryInformation;
        BATTERY_INFORMATION info{};
        if (!query(device.value, IOCTL_BATTERY_QUERY_INFORMATION, request, info) ||
            !(info.Capabilities & BATTERY_SYSTEM_BATTERY) ||
            (info.Capabilities & BATTERY_IS_SHORT_TERM)) continue;
        // Target is a single internal battery. Never combine partial/mismatched packs.
        if (++systemBatteries > 1) {
            return BatterySample{-1, -1, -1, -1, QStringLiteral("Unavailable (multiple system batteries)")};
        }
        BATTERY_WAIT_STATUS wait{};
        wait.BatteryTag = tag;
        BATTERY_STATUS status{};
        const bool haveStatus = query(device.value, IOCTL_BATTERY_QUERY_STATUS, wait, status);
        result = decodeBatteryReadings(info, haveStatus ? &status : nullptr);
    }
#endif
    return result;
}
