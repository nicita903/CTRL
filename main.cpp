#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "backend/SystemMonitor.h"
#include <QQmlContext>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <cstdio>
#include <cstring>
#include "hardware/HardwareManager.h"
#include <QQuickWindow>
#include <QImage>
#include <QDir>
#include <QFontDatabase>
#include <QFont>

int main(int argc, char *argv[])
{
    if (argc == 2 && std::strcmp(argv[1], "--hardware-check") == 0) {
        QCoreApplication app(argc, argv);
        QCoreApplication::setApplicationName("CTRL");
        auto state = HardwareManager::inspectReadOnly();
        const auto sensors = AsusSensors().sample();
        state["cpuFanRpm"] = sensors.cpuFanRpm;
        state["gpuFanRpm"] = sensors.gpuFanRpm;
        state["coolingMode"] = sensors.coolingMode;
        state["gpuActivity"] = sensors.gpuActivity;
        const auto json = QJsonDocument(QJsonObject::fromVariantMap(state)).toJson();
        std::fwrite(json.constData(), 1, json.size(), stdout);
        return 0; // Deliberately no CLI write command.
    }
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
    const bool qmlCheck = argc == 2 && std::strcmp(argv[1], "--qml-check") == 0;
    if (qmlCheck) {
        qputenv("QT_QPA_PLATFORM", "offscreen");
        qputenv("QT_QUICK_BACKEND", "software");
    }
    QGuiApplication app(argc, argv);
    if (qmlCheck) {
        QFontDatabase::addApplicationFont(qEnvironmentVariable("WINDIR", "C:/Windows") + "/Fonts/consola.ttf");
        app.setFont(QFont("Consolas"));
    }
    QCoreApplication::setApplicationName("CTRL");

    SystemMonitor systemMonitor;
    HardwareManager hardwareManager;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("hardwareManager", &hardwareManager);

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

    if (qmlCheck && !engine.rootObjects().isEmpty()) {
        // Offscreen, software-rendered read-only integration check. Never activates controls.
        auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
        if (!window) return 2;
        window->setProperty("page", 2);
        for (int index = 0; index < 3; ++index) {
            QTimer::singleShot(2500 + index * 1500, &app, [window, index] { window->setProperty("hardwareTab", index); });
            QTimer::singleShot(3500 + index * 1500, &app, [window, index, &app] {
                const QString path = QDir::currentPath() + QString("/build-hardware/ui-tab-%1.png").arg(index);
                if (!window->grabWindow().save(path)) { std::fprintf(stderr, "UI capture failed\n"); app.exit(2); }
            });
        }
        QTimer::singleShot(7000, &app, &QCoreApplication::quit);
    }

    return QGuiApplication::exec();
}
