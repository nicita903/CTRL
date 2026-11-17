#pragma once
#include <QObject>
#include <QVariantMap>
#include <QThread>
#include <functional>

class HardwareManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantMap state READ state NOTIFY changed)
    Q_PROPERTY(QVariantMap pending READ pending NOTIFY changed)
    Q_PROPERTY(QVariantList results READ results NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(QString message READ message NOTIFY changed)
public:
    explicit HardwareManager(QObject *parent = nullptr);
    ~HardwareManager() override;
    QVariantMap state() const { return m_state; }
    QVariantMap pending() const { return m_pending; }
    QVariantList results() const { return m_results; }
    bool busy() const { return m_busy; }
    QString message() const { return m_message; }
    Q_INVOKABLE void inspect();
    Q_INVOKABLE void inspectNvidia();
    Q_INVOKABLE void selectCurveMode(int mode);
    Q_INVOKABLE bool stage(const QString &setting, const QVariant &value);
    Q_INVOKABLE void discard();
    Q_INVOKABLE void restoreFactoryCurves();
    Q_INVOKABLE void restoreSavedSettings();
    Q_INVOKABLE void applyConfirmed(bool confirmed);
    static QVariantMap inspectReadOnly(bool discoverWmi = true);
signals:
    void changed();
private:
    void run(std::function<QVariantMap()> job, std::function<void(QVariantMap)> complete);
    QVariantMap m_state;
    QVariantMap m_original;
    QVariantMap m_pending;
    QVariantList m_results;
    QString m_message = "Read-only hardware discovery in progress";
    bool m_busy = false;
    QThread *m_worker = nullptr;
};
