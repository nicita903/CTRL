#pragma once
#include "HardwareCapabilities.h"
#include <QByteArray>

// Fixed operations only. This class is never exposed to QML.
enum class AsusEndpoint : quint32 {
    CpuFan = 0x00110013, GpuFan = 0x00110014, CpuTemperature = 0x00120094,
    CpuCurve = 0x00110024, GpuCurve = 0x00110025,
    Performance = 0x00120075, GpuEco = 0x00090020, GpuMux = 0x00090016,
    Spl = 0x001200a3, Sppt = 0x001200a0, Fppt = 0x001200c1,
    CpuThermal = 0x0012009e, GpuThermal = 0x001200c2, DynamicBoost = 0x001200c0
};
struct AsusReply {
    CapabilityState state = CapabilityState::Error;
    QString reason;
    QByteArray bytes;
    int value = -1;
    bool ok() const { return state == CapabilityState::Supported; }
};
class AsusWmiController {
public:
    AsusWmiController();
    ~AsusWmiController();
    AsusWmiController(const AsusWmiController &) = delete;
    AsusWmiController &operator=(const AsusWmiController &) = delete;
    AsusReply read(AsusEndpoint endpoint, int mode = 0) const;
    AsusReply setPerformance(int mode) const;
    AsusReply setGpuEco(bool eco) const;
    AsusReply setCurve(bool gpu, const QByteArray &curve) const;
    static HardwareIdentity identify();
    static QString discoverWmi();
private:
    AsusReply call(quint32 method, const QByteArray &arguments, bool buffer = false) const;
    void *m_handle = reinterpret_cast<void *>(-1);
    unsigned long m_error = 0;
};
