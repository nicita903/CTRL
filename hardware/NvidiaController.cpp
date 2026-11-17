#include "NvidiaController.h"
#ifdef Q_OS_WIN
#include <windows.h>
#include <array>
#include <algorithm>

namespace {
using Device = void *;
using Status = int;
class Nvml {
public:
    Nvml() {
        wchar_t system[MAX_PATH]{};
        if (!GetSystemDirectoryW(system, MAX_PATH)) return;
        const auto path = QString::fromWCharArray(system) + "\\nvml.dll";
        module = LoadLibraryExW(reinterpret_cast<const wchar_t *>(path.utf16()), nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (!module) { error = QString("DRIVER UNAVAILABLE: System32 NVML load error %1").arg(GetLastError()); return; }
        auto init = function<Status (*)()>("nvmlInit_v2");
        auto count = function<Status (*)(unsigned *)>("nvmlDeviceGetCount_v2");
        auto get = function<Status (*)(unsigned, Device *)>("nvmlDeviceGetHandleByIndex_v2");
        auto name = function<Status (*)(Device, char *, unsigned)>("nvmlDeviceGetName");
        auto id = function<Status (*)(Device, char *, unsigned)>("nvmlDeviceGetUUID");
        if (!init || !count || !get || !name || !id) return;
        const int status = init();
        if (status != 0) { error = QString("NVML initialization error %1").arg(status); return; }
        initialized = true;
        unsigned devices = 0;
        if (count(&devices) != 0) return;
        for (unsigned index = 0; index < std::min(devices, 16u); ++index) {
            Device candidate = nullptr;
            std::array<char, 128> buffer{};
            if (get(index, &candidate) != 0 || name(candidate, buffer.data(), buffer.size()) != 0) continue;
            const auto model = QString::fromLatin1(buffer.data());
            if (!model.contains("RTX 4060 Laptop GPU")) continue;
            buffer.fill(0);
            if (id(candidate, buffer.data(), buffer.size()) != 0) continue;
            device = candidate; uuid = QString::fromLatin1(buffer.data()); error.clear(); return;
        }
    }
    ~Nvml() {
        if (initialized) { auto shutdown = function<Status (*)()>("nvmlShutdown"); if (shutdown) shutdown(); }
        if (module) FreeLibrary(module);
    }
    template<typename T> T function(const char *name) const { return module ? reinterpret_cast<T>(GetProcAddress(module, name)) : nullptr; }
    Status number(const char *name, unsigned &value) const {
        auto query = function<Status (*)(Device, unsigned *)>(name);
        return query && device ? query(device, &value) : 3; // NVML_ERROR_NOT_SUPPORTED
    }
    QVariantMap inspect() const {
        if (!device) return capability(CapabilityState::DriverUnavailable, error);
        unsigned minimum = 0, maximum = 0, current = 0, defaultLimit = 0;
        auto constraints = function<Status (*)(Device, unsigned *, unsigned *)>("nvmlDeviceGetPowerManagementLimitConstraints");
        const int status = constraints ? constraints(device, &minimum, &maximum) : 3;
        const bool valid = status == 0 && number("nvmlDeviceGetPowerManagementLimit", current) == 0
            && number("nvmlDeviceGetPowerManagementDefaultLimit", defaultLimit) == 0
            && minimum > 0 && minimum <= current && current <= maximum && defaultLimit >= minimum && defaultLimit <= maximum;
        const unsigned ceiling = std::min(maximum, defaultLimit);
        auto result = capability(valid && (minimum + 999) / 1000 <= ceiling / 1000 ? CapabilityState::Supported
                                : status == 4 ? CapabilityState::PermissionRequired : CapabilityState::Unsupported,
                                valid ? "NVML driver-reported range; power reductions only; write not yet tested"
                                      : QString("NVML laptop power control unavailable (status %1)").arg(status));
        result["uuid"] = uuid;
        result["currentMilliwatts"] = valid ? QVariant(current) : QVariant(-1);
        result["defaultMilliwatts"] = valid ? QVariant(defaultLimit) : QVariant(-1);
        result["minimumWatts"] = valid ? int((minimum + 999) / 1000) : 0;
        result["maximumWatts"] = valid ? int(ceiling / 1000) : 0;
        unsigned power = 0, temperature = 0;
        result["powerWatts"] = number("nvmlDeviceGetPowerUsage", power) == 0 ? power / 1000.0 : -1;
        auto temp = function<Status (*)(Device, unsigned, unsigned *)>("nvmlDeviceGetTemperature");
        result["temperatureC"] = temp && temp(device, 0, &temperature) == 0 ? int(temperature) : -1;
        result["sampleNote"] = "On-demand snapshot only; NVML shut down after inspection";
        return result;
    }
    HMODULE module = nullptr;
    bool initialized = false;
    Device device = nullptr;
    QString uuid;
    QString error = "NVIDIA RTX 4060 Laptop GPU or required NVML entry points unavailable";
};
}
#endif
QVariantMap NvidiaController::inspectPower() {
#ifdef Q_OS_WIN
    return Nvml().inspect();
#else
    return capability(CapabilityState::Unsupported, "Windows only");
#endif
}
QString NvidiaController::setPower(int watts, const QString &expectedUuid) {
#ifdef Q_OS_WIN
    Nvml nvml;
    const auto current = nvml.inspect();
    if (!current.value("enabled").toBool()) return current.value("reason").toString();
    if (expectedUuid.isEmpty() || nvml.uuid != expectedUuid) return "NVIDIA device identity changed; inspect again";
    if (watts < current.value("minimumWatts").toInt() || watts > current.value("maximumWatts").toInt()) return "Power target outside the driver-reported safe range";
    auto set = nvml.function<Status (*)(Device, unsigned)>("nvmlDeviceSetPowerManagementLimit");
    if (!set) return "UNSUPPORTED: NVML power write export absent";
    const unsigned previous = current.value("currentMilliwatts").toUInt();
    const int status = set(nvml.device, unsigned(watts) * 1000);
    if (status != 0) return QString("NVML power write status %1%2").arg(status).arg(status == 4 ? " (PERMISSION REQUIRED)" : "");
    unsigned readback = 0;
    if (nvml.number("nvmlDeviceGetPowerManagementLimit", readback) != 0 || readback != unsigned(watts) * 1000)
        return QString("NVML readback failed; rollback status %1").arg(set(nvml.device, previous));
    return {};
#else
    Q_UNUSED(watts); Q_UNUSED(expectedUuid); return "Windows only";
#endif
}
QString NvidiaController::canDisableGpu() {
#ifdef Q_OS_WIN
    Nvml nvml;
    if (!nvml.device) return nvml.error;
    unsigned displayActive = 0;
    if (nvml.number("nvmlDeviceGetDisplayActive", displayActive) != 0 || displayActive != 0)
        return "ECO refused: dGPU display activity could not be excluded; disconnect dGPU displays";
    for (const auto *symbol : {"nvmlDeviceGetComputeRunningProcesses_v3", "nvmlDeviceGetGraphicsRunningProcesses_v3"}) {
        auto processes = nvml.function<Status (*)(Device, unsigned *, void *)>(symbol);
        unsigned count = 0;
        if (!processes || processes(nvml.device, &count, nullptr) != 0 || count != 0)
            return "ECO refused: dGPU clients could not be excluded; close GPU applications";
    }
    return {}; // NVML session is closed before ASUS power-gating.
#else
    return "Windows only";
#endif
}
