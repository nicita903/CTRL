#include "HardwareManager.h"
#include "AsusWmiController.h"
#include "CpuController.h"
#include "FanController.h"
#include "NvidiaController.h"
#include <QDir>
#include <QSaveFile>
#include <QStandardPaths>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaObject>
#include <cmath>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace {
bool onAC() {
#ifdef Q_OS_WIN
    SYSTEM_POWER_STATUS status{};
    return GetSystemPowerStatus(&status) && status.ACLineStatus == 1;
#else
    return false;
#endif
}
QVariantMap fromReply(const AsusReply &reply) { return capability(reply.state, reply.reason, reply.value); }
int readValue(const QVariantMap &state, const QString &key) { return state.value(key).toMap().value("value", -1).toInt(); }
QString capKey(const QString &key) { return key == "boostAC" || key == "boostDC" ? "boost" : key; }
bool supported(const QVariantMap &state, const QString &key) { return state.value(capKey(key)).toMap().value("enabled").toBool(); }
QVariantList performanceOptions() {
    QVariantList values{QVariantMap{{"label", "SILENT"}, {"value", 2}}, QVariantMap{{"label", "BALANCED"}, {"value", 0}}};
    if (onAC()) values.append(QVariantMap{{"label", "TURBO"}, {"value", 1}});
    return values;
}
QString saveBackup(const QVariantMap &state, const QVariantMap &pending) {
    const auto folder = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/hardware-backups";
    if (!QDir().mkpath(folder)) return {};
    const auto path = folder + "/before-" + QDateTime::currentDateTimeUtc().toString("yyyyMMdd-HHmmss-zzz") + ".json";
    QSaveFile file(path);
    const auto data = QJsonDocument(QJsonObject::fromVariantMap({{"before", state}, {"requested", pending},
        {"note", "Fan curves are firmware factory curves, not a readback of any previous custom curve"}})).toJson();
    if (!file.open(QIODevice::WriteOnly) || file.write(data) != data.size() || !file.commit()) return {};
    return path;
}
QString verifyGpuMode(AsusWmiController &asus, int requested) {
    const auto reply = asus.setGpuEco(requested == 1);
    if (!reply.ok()) return reply.reason;
    const auto readback = asus.read(AsusEndpoint::GpuEco);
    if (readback.ok() && readback.value == requested) return {};
    // Never power-gate a GPU to roll back without a fresh client/display guard.
    return "GPU firmware acknowledged write; readback failed. No automatic GPU rollback; inspect state";
}
}

QVariantMap HardwareManager::inspectReadOnly(bool discoverWmi) {
    QVariantMap state;
    const auto identity = AsusWmiController::identify();
    state["manufacturer"] = identity.manufacturer;
    state["model"] = identity.model;
    state["identityVerified"] = identity.allowlisted();
    if (discoverWmi) state["wmi"] = AsusWmiController::discoverWmi();
    AsusWmiController asus;
    const auto mode = asus.read(AsusEndpoint::Performance);
    state["performance"] = capability(mode.state, mode.reason, -1);
    auto performance = state["performance"].toMap();
    performance["rawStatus"] = mode.value;
    performance["options"] = performanceOptions();
    performance["reason"] = mode.ok() ? "Endpoint present; active mode readback unavailable. Writes have acknowledgement only. Turbo requires AC" : mode.reason;
    state["performance"] = performance;
    QVariantMap factoryModes;
    for (int selectedMode : {0, 1, 2}) {
      QVariantMap curves;
      for (bool gpu : {false, true}) {
        const auto curve = mode.ok() ? FanController::readFactory(asus, gpu, selectedMode)
                                    : AsusReply{mode.state, "A valid performance mode is required before reading factory curves", {}, -1};
        auto cap = capability(curve.state, curve.ok() ? "Eight-point factory curve read; only duty increases at factory temperatures allowed. Custom-write acknowledgement only" : curve.reason,
                               FanController::points(curve.bytes));
        cap["mode"] = selectedMode;
        cap["rawResponseHex"] = QString::fromLatin1(curve.bytes.toHex());
        if (!identity.allowlisted()) {
            cap["state"] = "UNSUPPORTED"; cap["enabled"] = false;
            cap["reason"] = "Firmware writes require verified ASUS FA507NV identity";
        }
        curves[gpu ? "gpuCurve" : "cpuCurve"] = cap;
      }
      factoryModes[QString::number(selectedMode)] = curves;
    }
    state["factoryCurves"] = factoryModes;
    state["curveMode"] = 0; // Explicit editor selection, never a claim of active mode.
    state["cpuCurve"] = factoryModes.value("0").toMap().value("cpuCurve");
    state["gpuCurve"] = factoryModes.value("0").toMap().value("gpuCurve");
    const auto eco = asus.read(AsusEndpoint::GpuEco);
    const auto mux = asus.read(AsusEndpoint::GpuMux);
    state["gpuMux"] = fromReply(mux);
    auto gpuMode = fromReply(eco);
    if (eco.ok() && (eco.value > 1 || !mux.ok() || mux.value != 1))
        gpuMode = capability(CapabilityState::Unsupported, "GPU switching requires verified hybrid MUX state; Ultimate/restart workflow is not implemented", eco.value);
    gpuMode["options"] = QVariantList{QVariantMap{{"label", "ECO"}, {"value", 1}}, QVariantMap{{"label", "STANDARD"}, {"value", 0}}};
    if (gpuMode.value("enabled").toBool()) gpuMode["reason"] = "Firmware mode read verified. ECO Apply requires no NVIDIA clients/displays; switching may reconfigure displays";
    state["gpuMode"] = gpuMode;
    state["gpuPower"] = capability(CapabilityState::Unsupported, "Explicit NVIDIA inspection required (may wake the dGPU); no automatic NVML initialization");
    state["boost"] = CpuController::inspectBoost();
    const std::pair<const char *, AsusEndpoint> limited[] = {
        {"spl", AsusEndpoint::Spl}, {"sppt", AsusEndpoint::Sppt}, {"fppt", AsusEndpoint::Fppt},
        {"cpuThermal", AsusEndpoint::CpuThermal}, {"gpuThermal", AsusEndpoint::GpuThermal},
        {"dynamicBoost", AsusEndpoint::DynamicBoost}};
    for (const auto &[key, endpoint] : limited) {
        const auto reply = asus.read(endpoint);
        state[key] = reply.ok() ? capability(CapabilityState::Unsupported,
            "Endpoint responds, but firmware does not provide a verified writable range; control disabled", reply.value) : fromReply(reply);
    }
    if (!identity.allowlisted()) {
        for (const auto *key : {"performance", "cpuCurve", "gpuCurve", "gpuMode"}) {
            auto cap = state[key].toMap();
            cap["state"] = "UNSUPPORTED"; cap["enabled"] = false;
            cap["reason"] = "Firmware writes restricted to a verified ASUS FA507NV SMBIOS identity";
            state[key] = cap;
        }
    }
    return state;
}

HardwareManager::HardwareManager(QObject *parent) : QObject(parent) { inspect(); }
HardwareManager::~HardwareManager() { if (m_worker) { m_worker->wait(); delete m_worker; } }
void HardwareManager::run(std::function<QVariantMap()> job, std::function<void(QVariantMap)> complete) {
    if (m_busy) return;
    if (m_worker) { m_worker->wait(); delete m_worker; m_worker = nullptr; }
    m_busy = true;
    emit changed();
    m_worker = QThread::create([this, job=std::move(job), complete=std::move(complete)] {
        const auto result = job();
        QMetaObject::invokeMethod(this, [this, result, complete] {
            m_busy = false; complete(result); emit changed();
        }, Qt::QueuedConnection);
    });
    m_worker->start();
}
void HardwareManager::inspect() {
    if (m_busy) return;
    if (!m_pending.isEmpty()) { m_message = "Discard pending changes before refreshing capabilities"; emit changed(); return; }
    run([] { return inspectReadOnly(); }, [this](QVariantMap result) {
        m_state = result;
        if (m_original.isEmpty()) m_original = result;
        m_message = "Read-only discovery complete; no firmware settings changed";
    });
}
void HardwareManager::inspectNvidia() {
    if (m_busy || !m_pending.isEmpty()) return;
    if (readValue(m_state, "gpuMode") == 1) { m_message = "GPU SLEEPING (ECO); NVIDIA initialization skipped"; emit changed(); return; }
    run([] { return NvidiaController::inspectPower(); }, [this](QVariantMap result) {
        m_state["gpuPower"] = result;
        if (!m_original.contains("inspectedGpuPower")) m_original["inspectedGpuPower"] = result;
        m_message = "NVIDIA on-demand snapshot complete; no ongoing NVIDIA polling";
    });
}
void HardwareManager::selectCurveMode(int mode) {
    if (m_busy || !m_pending.isEmpty() || mode < 0 || mode > 2) return;
    const auto curves = m_state.value("factoryCurves").toMap().value(QString::number(mode)).toMap();
    if (curves.isEmpty()) return;
    m_state["curveMode"] = mode;
    m_state["cpuCurve"] = curves.value("cpuCurve");
    m_state["gpuCurve"] = curves.value("gpuCurve");
    m_message = "Factory curve editor mode selected; applying a curve will first apply this firmware mode";
    emit changed();
}
bool HardwareManager::stage(const QString &setting, const QVariant &value) {
    if (m_busy) return false;
    const QStringList allowed{"performance", "boostAC", "boostDC", "cpuCurve", "gpuCurve", "gpuMode", "gpuPower"};
    auto reject = [&](const QString &reason) { m_message = reason; emit changed(); return false; };
    if (!allowed.contains(setting) || !supported(m_state, setting)) return reject("Control unavailable or not in the operation allowlist");
    if ((setting == "performance" && (m_pending.contains("cpuCurve") || m_pending.contains("gpuCurve")))
        || ((setting == "cpuCurve" || setting == "gpuCurve") && m_pending.contains("performance")))
        return reject("Apply a performance-mode change separately, then reload that mode's factory curves");
    if ((setting == "gpuPower" && m_pending.contains("gpuMode")) || (setting == "gpuMode" && m_pending.contains("gpuPower")))
        return reject("Apply GPU mode and GPU power changes separately");
    if (setting == "cpuCurve" || setting == "gpuCurve") {
        QString error;
        const auto curve = FanController::encode(value.toList(), error);
        const auto factory = FanController::encode(m_state.value(setting).toMap().value("value").toList(), error);
        if (!error.isEmpty()) return reject(error);
        error = FanController::validateCustom(curve, factory);
        if (!error.isEmpty()) return reject(error);
    } else {
        bool ok = false;
        const double number = value.toDouble(&ok);
        if (!ok || !std::isfinite(number) || number != std::floor(number) || number < 0 || number > 1000) return reject("Invalid integral control value");
        const int selected = int(number);
        if (setting == "gpuPower") {
            const auto cap = m_state.value(setting).toMap();
            if (selected < cap.value("minimumWatts").toInt() || selected > cap.value("maximumWatts").toInt()) return reject("GPU target outside the inspected NVML range");
        } else {
            bool found = false;
            for (const auto &option : m_state.value(capKey(setting)).toMap().value("options").toList())
                if (option.toMap().value("value").toInt() == selected) found = true;
            if (!found) return reject("Value not among the discovered supported options");
        }
    }
    if (setting != "cpuCurve" && setting != "gpuCurve") {
        const auto cap = m_state.value(capKey(setting)).toMap();
        const QVariant current = setting == "boostAC" ? cap.value("ac") : setting == "boostDC" ? cap.value("dc")
            : setting == "gpuPower" ? QVariant(cap.value("currentMilliwatts").toUInt() / 1000.0) : cap.value("value");
        if (value.toDouble() == current.toDouble()) {
            m_pending.remove(setting); m_message = "Setting already matches the inspected value; no write staged";
            emit changed(); return true;
        }
    }
    m_pending[setting] = value;
    m_message = "Pending changes validated; no writes until you confirm Apply";
    emit changed(); return true;
}
void HardwareManager::discard() {
    if (m_busy) return;
    m_pending.clear(); m_message = "Pending changes discarded; hardware unchanged"; emit changed();
}
void HardwareManager::restoreFactoryCurves() {
    if (m_busy) return;
    discard();
    bool any = false;
    for (const auto *key : {"cpuCurve", "gpuCurve"})
        if (supported(m_state, key)) any = stage(key, m_state.value(key).toMap().value("value")) || any;
    m_message = any ? "Factory fan curves staged. Apply first selects the editor's firmware base mode, then restores its curves"
                    : "Factory restoration unavailable: no validated factory curves";
    emit changed();
}
void HardwareManager::restoreSavedSettings() {
    if (m_busy) return;
    discard();
    // These are captured values, never advertised as universal OEM defaults.
    if (readValue(m_original, "performance") >= 0 && supported(m_state, "performance") && supported(m_original, "performance")) stage("performance", readValue(m_original, "performance"));
    const auto boost = m_original.value("boost").toMap();
    if (supported(m_state, "boost") && boost.value("scheme") == m_state.value("boost").toMap().value("scheme")) {
        stage("boostAC", boost.value("ac")); stage("boostDC", boost.value("dc"));
    }
    if (supported(m_state, "gpuMode") && supported(m_original, "gpuMode")) stage("gpuMode", readValue(m_original, "gpuMode"));
    const auto savedPower = m_original.value("inspectedGpuPower").toMap();
    if (supported(m_state, "gpuPower") && savedPower.value("enabled").toBool()
        && savedPower.value("uuid") == m_state.value("gpuPower").toMap().value("uuid")
        && savedPower.value("currentMilliwatts").toUInt() % 1000 == 0)
        stage("gpuPower", savedPower.value("currentMilliwatts").toUInt() / 1000);
    m_message = "Available saved boost/GPU values staged; initial ASUS performance mode is unreadable. Restore fan defaults separately; confirm Apply";
    emit changed();
}

void HardwareManager::applyConfirmed(bool confirmed) {
    if (!confirmed || m_busy || m_pending.isEmpty()) return;
    const auto pending = m_pending, expected = m_state;
    run([pending, expected] {
        QVariantList outcomes;
        auto current = inspectReadOnly(false);
        const int curveMode = expected.value("curveMode", 0).toInt();
        const auto selectedCurves = current.value("factoryCurves").toMap().value(QString::number(curveMode)).toMap();
        current["curveMode"] = curveMode;
        current["cpuCurve"] = selectedCurves.value("cpuCurve");
        current["gpuCurve"] = selectedCurves.value("gpuCurve");
        if (pending.contains("gpuPower") && current.value("identityVerified").toBool() && readValue(current, "gpuMode") == 0)
            current["gpuPower"] = NvidiaController::inspectPower(); // Confirmed action: snapshot before backup/write.
        const QString backup = saveBackup(current, pending);
        if (backup.isEmpty()) return QVariantMap{{"state", current}, {"results", outcomes}, {"message", "Backup failed; no writes performed"}, {"failed", pending}};
        AsusWmiController asus;
        QVariantMap failed;
        QString curveModeError;
        if (pending.contains("cpuCurve") || pending.contains("gpuCurve")) {
            if (!supported(current, "performance") || (curveMode == 1 && !onAC()))
                curveModeError = "Curve base mode unavailable or Turbo requires AC; no curve written";
            else if ((pending.contains("cpuCurve") && !supported(current, "cpuCurve")) || (pending.contains("gpuCurve") && !supported(current, "gpuCurve")))
                curveModeError = "Fresh factory-curve verification failed; no mode or curve written";
            else {
                // Validate every requested curve before changing its base mode.
                for (const auto *key : {"cpuCurve", "gpuCurve"}) {
                    if (!pending.contains(key)) continue;
                    QString conversion;
                    const auto proposed = FanController::encode(pending.value(key).toList(), conversion);
                    const auto factory = FanController::encode(current.value(key).toMap().value("value").toList(), conversion);
                    const auto error = FanController::validateCustom(proposed, factory);
                    if (!conversion.isEmpty() || !error.isEmpty()) { curveModeError = conversion.isEmpty() ? error : conversion; break; }
                }
                if (curveModeError.isEmpty()) {
                    const auto acknowledgement = asus.setPerformance(curveMode);
                    if (!acknowledgement.ok()) curveModeError = acknowledgement.reason;
                    outcomes.append(QVariantMap{{"setting", "curveBaseMode"}, {"success", acknowledgement.ok()},
                        {"message", acknowledgement.ok() ? "Base mode acknowledged; active mode readback unavailable; any previous custom curve may be reset" : acknowledgement.reason}});
                }
            }
        }
        for (const auto *name : {"performance", "boostAC", "boostDC", "cpuCurve", "gpuCurve", "gpuMode", "gpuPower"}) {
            const QString key(name);
            if (!pending.contains(key)) continue;
            QString error;
            QString success = "Applied and readback verified";
            if (!supported(current, key)) error = "Fresh capability check rejected this operation";
            else if (key == "performance") {
                const int value = pending.value(key).toInt();
                if (value == 1 && !onAC()) error = "Turbo refused: AC power is required";
                else { const auto reply = asus.setPerformance(value); if (!reply.ok()) error = reply.reason; }
                success = "Firmware acknowledged performance mode; active mode readback unavailable. Previous unreadable mode cannot be restored automatically";
            } else if (key == "boostAC" || key == "boostDC") {
                error = CpuController::setBoost(key == "boostAC", pending.value(key).toInt(), expected.value("boost").toMap().value("scheme").toString());
            } else if (key == "cpuCurve" || key == "gpuCurve") {
                const int mode = curveMode;
                if (!curveModeError.isEmpty()) error = curveModeError;
                else {
                    const bool gpu = key == "gpuCurve";
                    const auto factory = FanController::readFactory(asus, gpu, mode);
                    QString conversion;
                    const auto curve = FanController::encode(pending.value(key).toList(), conversion);
                    error = !factory.ok() ? factory.reason : !conversion.isEmpty() ? conversion : FanController::validateCustom(curve, factory.bytes);
                    if (error.isEmpty()) {
                        const auto reply = asus.setCurve(gpu, curve);
                        if (!reply.ok()) error = reply.reason;
                    }
                    success = "Firmware acknowledged curve write; custom-curve readback is not exposed. Factory restore is available";
                }
            } else if (key == "gpuMode") {
                const int value = pending.value(key).toInt();
                if (value == 1) error = NvidiaController::canDisableGpu();
                const auto muxNow = asus.read(AsusEndpoint::GpuMux);
                if (error.isEmpty() && (!muxNow.ok() || muxNow.value != 1)) error = "GPU MUX changed; Eco/Standard write refused";
                if (error.isEmpty()) error = verifyGpuMode(asus, value);
            } else if (key == "gpuPower") {
                if (!AsusWmiController::identify().allowlisted()) error = "Target hardware identity not verified";
                else if (readValue(current, "gpuMode") != 0) error = "GPU is not in verified Standard mode; inspection required";
                else error = NvidiaController::setPower(pending.value(key).toInt(), expected.value("gpuPower").toMap().value("uuid").toString());
            }
            if (!error.isEmpty()) failed[key] = pending.value(key);
            outcomes.append(QVariantMap{{"setting", key}, {"success", error.isEmpty()}, {"message", error.isEmpty() ? success : error}});
        }
        auto after = inspectReadOnly(false);
        const auto afterCurves = after.value("factoryCurves").toMap().value(QString::number(curveMode)).toMap();
        after["curveMode"] = curveMode;
        after["cpuCurve"] = afterCurves.value("cpuCurve");
        after["gpuCurve"] = afterCurves.value("gpuCurve");
        after["wmi"] = expected.value("wmi");
        for (const auto &outcome : outcomes) {
            const auto item = outcome.toMap();
            if (item.value("success").toBool()) continue;
            const auto key = capKey(item.value("setting").toString());
            if (key == "curveBaseMode") continue;
            auto cap = after.value(key).toMap();
            const auto reason = item.value("message").toString();
            cap["state"] = reason.contains("PERMISSION REQUIRED") ? "PERMISSION REQUIRED" : "ERROR";
            cap["enabled"] = false;
            cap["reason"] = reason + "; discard pending changes and inspect before retrying";
            after[key] = cap;
        }
        return QVariantMap{{"state", after}, {"failed", failed}, {"results", outcomes},
            {"message", QString("%1 change(s) failed. Per-setting results below. Backup: %2").arg(failed.size()).arg(backup)}};
    }, [this](QVariantMap result) {
        m_state = result.value("state").toMap(); m_pending = result.value("failed").toMap();
        m_results = result.value("results").toList(); m_message = result.value("message").toString();
    });
}
