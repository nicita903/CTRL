#include "SystemMonitor.h"

#include <QThread>
#include <QMetaObject>
#include <QMutex>
#include <QWaitCondition>
#include <algorithm>

#ifdef Q_OS_WIN
class TelemetryThread final : public QThread {
public:
    explicit TelemetryThread(SystemMonitor *target) : m_target(target) {}
    void setInterval(int interval) {
        QMutexLocker lock(&m_mutex);
        if (m_interval == interval) return;
        m_interval = interval;
        m_wake.wakeOne();
    }
    void stop() {
        QMutexLocker lock(&m_mutex);
        requestInterruption();
        m_wake.wakeOne();
    }
protected:
    void run() override {
        BatteryMonitor battery;
        AsusSensors asus;
        while (!isInterruptionRequested()) {
            const auto batterySample = battery.sample();
            const auto cpuSample = asus.sample();
            QMetaObject::invokeMethod(m_target, [target=m_target, batterySample, cpuSample] {
                target->acceptHardwareSample(batterySample, cpuSample);
            }, Qt::QueuedConnection);
            QMutexLocker lock(&m_mutex);
            if (!isInterruptionRequested()) m_wake.wait(&m_mutex, m_interval);
        }
    }
private:
    SystemMonitor *m_target;
    QMutex m_mutex;
    QWaitCondition m_wake;
    int m_interval = 2000;
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
        m_telemetryThread->stop();
        m_telemetryThread->wait();
        delete m_telemetryThread;
    }
#endif
}

void SystemMonitor::acceptHardwareSample(const BatterySample &battery, const CpuSensorSample &cpu) {
    m_battery = battery;
    m_cpuSensors = cpu;
    m_dischargeWatts = battery.dischargeWatts;
    m_cpuTemperatureC = cpu.temperatureC;
    m_cpuTemperatureSource = cpu.temperatureSource;
    emit updated();
}

void SystemMonitor::setMonitoringContext(bool background, int page) {
    const int interval = background ? 30000 : ((page == 0 || page == 2) ? 2000 : 6000);
    m_timer.setInterval(interval);
#ifdef Q_OS_WIN
    if (m_telemetryThread) m_telemetryThread->setInterval(interval);
#endif
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
        m_batteryPercent = power.BatteryLifePercent > 100 || (power.BatteryFlag != 255 && (power.BatteryFlag & 128))
                             ? -1 : power.BatteryLifePercent;
        m_onAC = power.ACLineStatus == 1;
        const bool flagsKnown = power.BatteryFlag != 255;
        const bool batteryPresent = flagsKnown && !(power.BatteryFlag & 128);
        m_charging = batteryPresent && (power.BatteryFlag & 8) != 0;
        m_powerSource = power.ACLineStatus == 255 ? QStringLiteral("Unavailable")
                      : (m_onAC ? QStringLiteral("AC") : QStringLiteral("Battery"));
        m_batteryStatus = !flagsKnown ? QStringLiteral("Unavailable")
                        : !batteryPresent ? QStringLiteral("No battery")
                        : m_charging ? QStringLiteral("Charging")
                        : power.ACLineStatus == 0 ? QStringLiteral("Discharging")
                        : m_onAC ? QStringLiteral("Connected, not charging") : QStringLiteral("Unavailable");
        m_remainingMinutes = power.BatteryLifeTime == static_cast<DWORD>(-1)
                             ? -1 : static_cast<int>(power.BatteryLifeTime / 60);
    }

    else {
        m_batteryPercent = -1;
        m_remainingMinutes = -1;
        m_onAC = m_charging = false;
        m_powerSource = m_batteryStatus = QStringLiteral("Unavailable");
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
