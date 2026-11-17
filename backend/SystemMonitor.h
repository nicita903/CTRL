#pragma once

#include <QObject>
#include <QTimer>
#include <QtGlobal>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

class SystemMonitor : public QObject {
    Q_OBJECT
    Q_PROPERTY(int batteryPercent READ batteryPercent NOTIFY updated)
    Q_PROPERTY(bool onAC READ onAC NOTIFY updated)
    Q_PROPERTY(bool charging READ charging NOTIFY updated)
    Q_PROPERTY(int remainingMinutes READ remainingMinutes NOTIFY updated)
    Q_PROPERTY(double cpuPercent READ cpuPercent NOTIFY updated)
    Q_PROPERTY(double ramUsedGB READ ramUsedGB NOTIFY updated)
    Q_PROPERTY(double ramTotalGB READ ramTotalGB NOTIFY updated)
    Q_PROPERTY(double ramRatio READ ramRatio NOTIFY updated)
public:
    explicit SystemMonitor(QObject *parent = nullptr);
    int batteryPercent() const { return m_batteryPercent; }
    bool onAC() const { return m_onAC; }
    bool charging() const { return m_charging; }
    int remainingMinutes() const { return m_remainingMinutes; }
    double cpuPercent() const { return m_cpuPercent; }
    double ramUsedGB() const { return m_ramUsedGB; }
    double ramTotalGB() const { return m_ramTotalGB; }
    double ramRatio() const { return m_ramRatio; }

signals:
    void updated();

private:
    void refresh();
#ifdef Q_OS_WIN
    static quint64 fileTimeTo64(const FILETIME &value);
    quint64 m_prevIdle = 0;
    quint64 m_prevKernel = 0;
    quint64 m_prevUser = 0;
    bool m_haveCpuSample = false;
#endif
    QTimer m_timer;
    int m_batteryPercent = -1;
    bool m_onAC = false;
    bool m_charging = false;
    int m_remainingMinutes = -1;
    double m_cpuPercent = -1.0;
    double m_ramUsedGB = 0.0;
    double m_ramTotalGB = 0.0;
    double m_ramRatio = 0.0;
};
