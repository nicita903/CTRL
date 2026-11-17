#pragma once
#include "HardwareCapabilities.h"

class CpuController {
public:
    static QVariantMap inspectBoost();
    static QString setBoost(bool ac, int value, const QString &expectedScheme);
};
