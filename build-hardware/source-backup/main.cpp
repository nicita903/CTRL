#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "backend/SystemMonitor.h"
#include <QQmlContext>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <cstdio>
#include <cstring>

int main(int argc, char *argv[])
{
    // Read-only integration check: exercise the exact backend without a GUI/GPU.
    if (argc == 2 && std::strcmp(argv[1], "--telemetry-check") == 0) {
        QCoreApplication app(argc, argv);
        SystemMonitor monitor;
        QTimer::singleShot(4500, &app, [&] {
            const QJsonObject sample{
                {"batteryPercent", monitor.batteryPercent()},
                {"powerSource", monitor.powerSource()},
                {"batteryStatus", monitor.batteryStatus()},
                {"remainingMinutes", monitor.remainingMinutes()},
                {"dischargeWatts", monitor.dischargeWatts()},
                {"designCapacityWh", monitor.designCapacityWh()},
                {"fullChargeCapacityWh", monitor.fullChargeCapacityWh()},
                {"batteryHealthPercent", monitor.batteryHealthPercent()},
                {"batterySource", monitor.batterySource()},
                {"cpuPercent", monitor.cpuPercent()},
                {"ramUsedGB", monitor.ramUsedGB()},
                {"cpuTemperatureC", monitor.cpuTemperatureC()},
                {"cpuTemperatureSource", monitor.cpuTemperatureSource()},
                {"cpuPackagePowerWatts", monitor.cpuPackagePowerWatts()},
                {"cpuPackagePowerSource", monitor.cpuPackagePowerSource()}
            };
            const auto json = QJsonDocument(sample).toJson();
            std::fwrite(json.constData(), 1, json.size(), stdout);
            app.quit();
        });
        return app.exec();
    }
    QGuiApplication app(argc, argv);

    SystemMonitor systemMonitor;
    QQmlApplicationEngine engine;

    engine.rootContext()->setContextProperty(
        "systemMonitor",
        &systemMonitor
        );
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("asus_control", "Main");

    return QGuiApplication::exec();
}
