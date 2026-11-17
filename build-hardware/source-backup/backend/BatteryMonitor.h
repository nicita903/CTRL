#pragma once

#include <QString>

struct BatterySample {
    double dischargeWatts = -1;
    double designCapacityWh = -1;
    double fullChargeCapacityWh = -1;
    double healthPercent = -1;
    QString source = QStringLiteral("Unavailable");
};

// All device discovery and I/O runs on the telemetry worker.
class BatteryMonitor {
public:
    BatterySample sample() const;
};
