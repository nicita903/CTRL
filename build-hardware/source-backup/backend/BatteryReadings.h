#pragma once
#include "BatteryMonitor.h"
#ifdef Q_OS_WIN
#include <windows.h>
#include <winioctl.h>
#include <poclass.h>

inline BatterySample decodeBatteryReadings(const BATTERY_INFORMATION &info, const BATTERY_STATUS *status) {
    BatterySample result;
    result.source = QStringLiteral("Windows battery IOCTL");
    const bool relative = (info.Capabilities & BATTERY_CAPACITY_RELATIVE) != 0;
    const auto known = [](ULONG value) { return value != BATTERY_UNKNOWN_CAPACITY && value > 0; };
    if (known(info.DesignedCapacity) && known(info.FullChargedCapacity))
        result.healthPercent = 100.0 * info.FullChargedCapacity / info.DesignedCapacity;
    if (relative) {
        result.source += QStringLiteral(" (relative units; watts/Wh unavailable)");
        return result;
    }
    if (known(info.DesignedCapacity)) result.designCapacityWh = info.DesignedCapacity / 1000.0;
    if (known(info.FullChargedCapacity)) result.fullChargeCapacityWh = info.FullChargedCapacity / 1000.0;
    if (status && (status->PowerState & BATTERY_DISCHARGING) &&
        !(status->PowerState & BATTERY_CHARGING) &&
        static_cast<ULONG>(status->Rate) != BATTERY_UNKNOWN_RATE && status->Rate < 0)
        result.dischargeWatts = -static_cast<double>(status->Rate) / 1000.0;
    return result;
}
#endif
