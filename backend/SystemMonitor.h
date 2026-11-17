#pragma once

#include <QObject>
#include <QTimer>
#include <QtGlobal>
#include "BatteryMonitor.h"
#include "../hardware/AsusSensors.h"

#ifdef Q_OS_WIN
#include <windows.h>
#endif

class TelemetryThread;

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
    // -1 means 'not reported' and is NEVER replaced by an invented measurement.
    Q_PROPERTY(double dischargeWatts READ dischargeWatts NOTIFY updated)
    Q_PROPERTY(double cpuTemperatureC READ cpuTemperatureC NOTIFY updated)
    Q_PROPERTY(QString cpuTemperatureSource READ cpuTemperatureSource NOTIFY updated)
    Q_PROPERTY(int cpuFanRpm READ cpuFanRpm NOTIFY updated)
    Q_PROPERTY(int gpuFanRpm READ gpuFanRpm NOTIFY updated)
    Q_PROPERTY(QString coolingMode READ coolingMode NOTIFY updated)
    Q_PROPERTY(QString gpuActivity READ gpuActivity NOTIFY updated)
    Q_PROPERTY(double cpuPackagePowerWatts READ cpuPackagePowerWatts NOTIFY updated)
    Q_PROPERTY(QString cpuPackagePowerSource READ cpuPackagePowerSource NOTIFY updated)
    Q_PROPERTY(double designCapacityWh READ designCapacityWh NOTIFY updated)
    Q_PROPERTY(double fullChargeCapacityWh READ fullChargeCapacityWh NOTIFY updated)
    Q_PROPERTY(double batteryHealthPercent READ batteryHealthPercent NOTIFY updated)
    Q_PROPERTY(QString batterySource READ batterySource NOTIFY updated)
    Q_PROPERTY(QString powerSource READ powerSource NOTIFY updated)
    Q_PROPERTY(QString batteryStatus READ batteryStatus NOTIFY updated)
public:
    explicit SystemMonitor(QObject *parent = nullptr);
    ~SystemMonitor() override;

    int batteryPercent() const { return m_batteryPercent; }
    bool onAC() const { return m_onAC; }
    bool charging() const { return m_charging; }
    int remainingMinutes() const { return m_remainingMinutes; }
    double cpuPercent() const { return m_cpuPercent; }
    double ramUsedGB() const { return m_ramUsedGB; }
    double ramTotalGB() const { return m_ramTotalGB; }
    double ramRatio() const { return m_ramRatio; }
    double dischargeWatts() const { return m_dischargeWatts; }
    double cpuTemperatureC() const { return m_cpuTemperatureC; }
    QString cpuTemperatureSource() const { return m_cpuTemperatureSource; }
    int cpuFanRpm() const { return m_cpuSensors.cpuFanRpm; }
    int gpuFanRpm() const { return m_cpuSensors.gpuFanRpm; }
    QString coolingMode() const { return m_cpuSensors.coolingMode; }
    QString gpuActivity() const { return m_cpuSensors.gpuActivity; }
    double cpuPackagePowerWatts() const { return m_cpuSensors.packagePowerWatts; }
    QString cpuPackagePowerSource() const { return m_cpuSensors.packagePowerSource; }
    double designCapacityWh() const { return m_battery.designCapacityWh; }
    double fullChargeCapacityWh() const { return m_battery.fullChargeCapacityWh; }
    double batteryHealthPercent() const { return m_battery.healthPercent; }
    QString batterySource() const { return m_battery.source; }
    QString powerSource() const { return m_powerSource; }
    QString batteryStatus() const { return m_batteryStatus; }

    // Runs only on the GUI thread; worker results are delivered via a queued call.
    void acceptHardwareSample(const BatterySample &battery, const CpuSensorSample &cpu);
    Q_INVOKABLE void setMonitoringContext(bool background, int page);

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
    TelemetryThread *m_telemetryThread = nullptr;
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
    double m_dischargeWatts = -1.0;
    double m_cpuTemperatureC = -1.0;
    QString m_cpuTemperatureSource = "Unavailable";
    BatterySample m_battery;
    CpuSensorSample m_cpuSensors;
    QString m_powerSource = "Unavailable";
    QString m_batteryStatus = "Unavailable";
};
