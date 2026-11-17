#include "backend/BatteryReadings.h"
#include <cmath>
#include <cstdio>

int main() {
    int failures = 0;
    const auto check = [&](bool ok, const char *message) {
        if (!ok) { std::fprintf(stderr, "FAIL: %s\n", message); ++failures; }
    };
    BATTERY_INFORMATION info{};
    info.Capabilities = BATTERY_SYSTEM_BATTERY;
    info.DesignedCapacity = 90000;
    info.FullChargedCapacity = 64800;
    BATTERY_STATUS status{};
    status.PowerState = BATTERY_DISCHARGING;
    status.Rate = -12500;
    auto result = decodeBatteryReadings(info, &status);
    check(result.designCapacityWh == 90 && result.fullChargeCapacityWh == 64.8, "mWh to Wh");
    check(std::abs(result.healthPercent - 72) < 0.001, "health from capacities");
    check(result.dischargeWatts == 12.5, "negative milliwatts converted to discharge watts");
    status.Rate = static_cast<LONG>(BATTERY_UNKNOWN_RATE);
    check(decodeBatteryReadings(info, &status).dischargeWatts == -1, "unknown rate sentinel");
    status.Rate = 12000;
    check(decodeBatteryReadings(info, &status).dischargeWatts == -1, "positive rate is not discharge");
    status.Rate = 0;
    check(decodeBatteryReadings(info, &status).dischargeWatts == -1, "zero rate not invented as discharge");
    status.Rate = -12000;
    status.PowerState = BATTERY_CHARGING;
    check(decodeBatteryReadings(info, &status).dischargeWatts == -1, "charging not discharge");
    status.PowerState = BATTERY_DISCHARGING;
    info.Capabilities |= BATTERY_CAPACITY_RELATIVE;
    result = decodeBatteryReadings(info, &status);
    check(result.dischargeWatts == -1 && result.designCapacityWh == -1 && result.fullChargeCapacityWh == -1,
          "relative units must never be presented as watts or Wh");
    info.Capabilities = BATTERY_SYSTEM_BATTERY;
    info.DesignedCapacity = BATTERY_UNKNOWN_CAPACITY;
    info.FullChargedCapacity = 0;
    result = decodeBatteryReadings(info, nullptr);
    check(result.healthPercent == -1 && result.designCapacityWh == -1 && result.fullChargeCapacityWh == -1,
          "missing capacities and status stay unavailable");
    return failures == 0 ? 0 : 1;
}
