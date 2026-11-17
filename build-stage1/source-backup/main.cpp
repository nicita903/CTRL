#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "backend/SystemMonitor.h"
#include <QQmlContext>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    SystemMonitor systemMonitor;

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
