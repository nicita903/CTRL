#include "SystemMonitor.h"
#include <algorithm>

SystemMonitor::SystemMonitor(QObject *parent) : QObject(parent) {
    m_timer.setInterval(2000); // Low-overhead monitoring: once every 2 seconds.
    connect(&m_timer, &QTimer::timeout, this, &SystemMonitor::refresh);
    refresh();
    m_timer.start();
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
                // GetSystemTimes kernel time already includes idle time.
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
