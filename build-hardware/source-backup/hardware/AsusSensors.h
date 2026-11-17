#pragma once
#include <QString>

struct CpuSensorSample {
    double temperatureC = -1;
    double packagePowerWatts = -1;
    QString temperatureSource = QStringLiteral("Unavailable");
    QString packagePowerSource = QStringLiteral("Unavailable (no verified Ryzen 7735HS package-power provider)");
};

// Read-only, fixed endpoint. No arbitrary firmware commands or control interface.
class AsusSensors {
public:
    CpuSensorSample sample() const;
};
