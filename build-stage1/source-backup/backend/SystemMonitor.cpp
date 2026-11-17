#include "SystemMonitor.h"

#include <QThread>
#include <QMetaObject>
#include <algorithm>
#include <cmath>

#ifdef Q_OS_WIN
#include <wbemidl.h>
#include <objbase.h>
#include <oleauto.h>

namespace {

struct ComSession {
    HRESULT result = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    ~ComSession() { if (SUCCEEDED(result)) CoUninitialize(); }
    bool available() const { return SUCCEEDED(result); }
};

struct WmiSession {
    IWbemLocator *locator = nullptr;
    IWbemServices *services = nullptr;
    ~WmiSession() {
        if (services) services->Release();
        if (locator) locator->Release();
    }
    bool open(const wchar_t *space) {
        if (FAILED(CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER,
                                    IID_IWbemLocator, reinterpret_cast<void**>(&locator)))) return false;
        BSTR ns = SysAllocString(space);
        HRESULT hr = locator->ConnectServer(ns, nullptr, nullptr, nullptr, 0, nullptr, nullptr, &services);
        SysFreeString(ns);
        if (FAILED(hr) || !services) return false;
        // For local WMI; acceptable if host COM security was initialized earlier.
        CoSetProxyBlanket(services, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE,
                          nullptr, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE,
                          nullptr, EOAC_NONE);
        return true;
    }
};

bool getNumber(IWbemClassObject *o, const wchar_t *name, double &out) {
    VARIANT v;
    VariantInit(&v);
    HRESULT hr = o->Get(name, 0, &v, nullptr, nullptr);
    if (FAILED(hr) || v.vt == VT_NULL || v.vt == VT_EMPTY) { VariantClear(&v); return false; }
    VARIANT converted;
    VariantInit(&converted);
    hr = VariantChangeType(&converted, &v, 0, VT_R8);
    if (SUCCEEDED(hr)) out = converted.dblVal;
    VariantClear(&converted);
    VariantClear(&v);
    return SUCCEEDED(hr) && std::isfinite(out);
}

QString getString(IWbemClassObject *o, const wchar_t *name) {
    VARIANT v;
    VariantInit(&v);
    QString value;
    if (SUCCEEDED(o->Get(name, 0, &v, nullptr, nullptr)) && v.vt == VT_BSTR && v.bstrVal)
        value = QString::fromWCharArray(v.bstrVal);
    VariantClear(&v);
    return value;
}

IWbemClassObject *firstObject(IWbemServices *service, const wchar_t *query) {
    BSTR language = SysAllocString(L"WQL");
    BSTR sql = SysAllocString(query);
    IEnumWbemClassObject *results = nullptr;
    HRESULT hr = service->ExecQuery(language, sql,
              WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &results);
    SysFreeString(sql);
    SysFreeString(language);
    if (FAILED(hr) || !results) return nullptr;
    IWbemClassObject *obj = nullptr;
    ULONG count = 0;
    hr = results->Next(350, 1, &obj, &count);
    results->Release();
    return (SUCCEEDED(hr) && count == 1) ? obj : nullptr;
}

// Windows' root\\wmi BatteryStatus class reports DischargeRate in milliwatts
// on many laptops. Some firmware omits it. Do not guess when absent.
double readBatteryWatts() {
    WmiSession wmi;
    if (!wmi.open(L"ROOT\\WMI")) return -1.0;
    IWbemClassObject *obj = firstObject(wmi.services, L"SELECT * FROM BatteryStatus");
    if (!obj) return -1.0;
    double discharging = 0, milliWatts = 0;
    const bool ok = getNumber(obj, L"Discharging", discharging) &&
                    getNumber(obj, L"DischargeRate", milliWatts);
    obj->Release();
    if (!ok || std::abs(discharging) < 0.5 || milliWatts <= 0 || milliWatts > 500000) return -1.0;
    return milliWatts / 1000.0;
}

// LibreHardwareMonitor is optional: run it with WMI exposure enabled.
// Never substitute ACPI thermal zones (often unrelated to CPU die temperature).
double readCpuTemp() {
    WmiSession wmi;
    if (!wmi.open(L"ROOT\\LibreHardwareMonitor")) return -1.0;
    BSTR language = SysAllocString(L"WQL");
    BSTR sql = SysAllocString(L"SELECT Name, SensorType, Value FROM Sensor WHERE SensorType='Temperature'");
    IEnumWbemClassObject *results = nullptr;
    HRESULT hr = wmi.services->ExecQuery(language, sql,
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &results);
    SysFreeString(sql);
    SysFreeString(language);
    if (FAILED(hr) || !results) return -1.0;
    double temp = -1.0;
    int bestPriority = -1;
    for (int i=0; i<100; ++i) {
        IWbemClassObject *obj = nullptr;
        ULONG count = 0;
        hr = results->Next(200, 1, &obj, &count);
        if (FAILED(hr) || count == 0) break;
        const QString name = getString(obj, L"Name");
        double value = -1;
        const bool valid = getNumber(obj, L"Value", value);
        obj->Release();
        const QString n = name.toLower();
        int priority = -1;
        if (n == "cpu (tctl/tdie)" || n == "cpu tctl/tdie") priority = 4;
        else if (n == "cpu (tdie)" || n == "cpu tdie") priority = 3;
        else if (n == "cpu package") priority = 2;
        else if (n.startsWith("cpu") && n.contains("temperature")) priority = 1;
        if (valid && value > 0 && value < 120 && priority > bestPriority) {
            temp = value;
            bestPriority = priority;
        }
    }
    results->Release();
    return temp;
}

} // namespace

class TelemetryThread final : public QThread {
public:
    explicit TelemetryThread(SystemMonitor *target) : m_target(target) {}
protected:
    void run() override {
        ComSession com;
        if (!com.available()) return;
        while (!isInterruptionRequested()) {
            const double watts = readBatteryWatts();
            const double cpuC = readCpuTemp();
            QMetaObject::invokeMethod(m_target, [target=m_target, watts, cpuC] {
                target->acceptHardwareSample(watts, cpuC,
                    cpuC >= 0 ? QStringLiteral("LibreHardwareMonitor WMI")
                              : QStringLiteral("Unavailable (optional sensor provider)"));
            }, Qt::QueuedConnection);
            // Interruptible sleep, avoiding a 6 s shutdown delay.
            for (int i = 0; i < 24 && !isInterruptionRequested(); ++i) msleep(250);
        }
    }
private:
    SystemMonitor *m_target;
};
#endif

SystemMonitor::SystemMonitor(QObject *parent) : QObject(parent) {
    m_timer.setInterval(2000);
    connect(&m_timer, &QTimer::timeout, this, &SystemMonitor::refresh);
    refresh();
    m_timer.start();
#ifdef Q_OS_WIN
    m_telemetryThread = new TelemetryThread(this);
    m_telemetryThread->start();
#endif
}

SystemMonitor::~SystemMonitor() {
#ifdef Q_OS_WIN
    if (m_telemetryThread) {
        m_telemetryThread->requestInterruption();
        m_telemetryThread->wait();
        delete m_telemetryThread;
    }
#endif
}

void SystemMonitor::acceptHardwareSample(double watts, double tempC, const QString &source) {
    m_dischargeWatts = watts;
    m_cpuTemperatureC = tempC;
    m_cpuTemperatureSource = source;
    emit updated();
}

#ifdef Q_OS_WIN
quint64 SystemMonitor::fileTimeTo64(const FILETIME &value) {
    return (static_cast<quint64>(value.dwHighDateTime) << 32) | value.dwLowDateTime;
}
#endif

void SystemMonitor::refresh() {
#ifdef Q_OS_WIN
    SYSTEM_POWER_STATUS power{};
    if (GetSystemPowerStatus(&power)) {
        m_batteryPercent = power.BatteryLifePercent == 255 ? -1 : power.BatteryLifePercent;
        m_onAC = power.ACLineStatus == 1;
        m_charging = m_onAC && (power.BatteryFlag & 8) != 0;
        m_remainingMinutes = power.BatteryLifeTime == static_cast<DWORD>(-1)
                             ? -1 : static_cast<int>(power.BatteryLifeTime / 60);
    }

    MEMORYSTATUSEX memory{};
    memory.dwLength = sizeof(memory);
    if (GlobalMemoryStatusEx(&memory) && memory.ullTotalPhys > 0) {
        constexpr double gib = 1024.0 * 1024.0 * 1024.0;
        m_ramTotalGB = static_cast<double>(memory.ullTotalPhys) / gib;
        m_ramUsedGB = static_cast<double>(memory.ullTotalPhys - memory.ullAvailPhys) / gib;
        m_ramRatio = m_ramUsedGB / m_ramTotalGB;
    }

    FILETIME idle{}, kernel{}, user{};
    if (GetSystemTimes(&idle, &kernel, &user)) {
        const quint64 i = fileTimeTo64(idle);
        const quint64 k = fileTimeTo64(kernel);
        const quint64 u = fileTimeTo64(user);
        if (m_haveCpuSample) {
            const quint64 idleDelta = i - m_prevIdle;
            const quint64 totalDelta = (k - m_prevKernel) + (u - m_prevUser);
            if (totalDelta > 0) {
                m_cpuPercent = std::clamp(100.0 * (1.0 - double(idleDelta) / double(totalDelta)), 0.0, 100.0);
            }
        }
        m_prevIdle = i;
        m_prevKernel = k;
        m_prevUser = u;
        m_haveCpuSample = true;
    }
#endif
    emit updated();
}
