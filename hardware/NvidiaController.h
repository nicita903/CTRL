#pragma once
#include "HardwareCapabilities.h"

class NvidiaController {
public:
    // Only called by an explicit user action; never from a polling timer/discovery.
    static QVariantMap inspectPower();
    static QString setPower(int watts, const QString &expectedUuid);
    static QString canDisableGpu();
};
