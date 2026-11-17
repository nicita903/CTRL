#include "FanController.h"
#include <cmath>

QString FanController::validateFactory(const QByteArray &curve) {
    if (curve.size() != 16) return "Firmware did not return eight temperature/duty points";
    int previousTemp = -1, previousDuty = -1;
    for (int index = 0; index < 8; ++index) {
        const int temperature = quint8(curve[index]), duty = quint8(curve[index + 8]);
        if (temperature > 100 || duty > 100 || temperature <= previousTemp || duty < previousDuty)
            return "Factory curve is not a validated monotonic 0–100°C / 0–100% curve";
        previousTemp = temperature; previousDuty = duty;
    }
    if (previousDuty == 0) return "Empty factory curve";
    return {};
}
QString FanController::validateCustom(const QByteArray &curve, const QByteArray &factory) {
    auto error = validateFactory(factory);
    if (!error.isEmpty()) return error;
    error = validateFactory(curve);
    if (!error.isEmpty()) return error;
    for (int index = 0; index < 8; ++index) {
        if (curve[index] != factory[index]) return "Temperature bins must match the firmware factory curve";
        if (quint8(curve[index + 8]) < quint8(factory[index + 8]))
            return "Cooling cannot be reduced below the factory duty at any point";
    }
    return {};
}
QVariantList FanController::points(const QByteArray &curve) {
    QVariantList result;
    if (!validateFactory(curve).isEmpty()) return result;
    for (int index = 0; index < 8; ++index)
        result.append(QVariantMap{{"temperature", quint8(curve[index])}, {"duty", quint8(curve[index + 8])}});
    return result;
}
QByteArray FanController::encode(const QVariantList &points, QString &error) {
    if (points.size() != 8) { error = "Exactly eight fan points required"; return {}; }
    QByteArray curve(16, '\0');
    for (int index = 0; index < 8; ++index) {
        const auto point = points[index].toMap();
        bool temperatureOk = false, dutyOk = false;
        const double temperature = point.value("temperature").toDouble(&temperatureOk);
        const double duty = point.value("duty").toDouble(&dutyOk);
        if (!temperatureOk || !dutyOk || !std::isfinite(temperature) || !std::isfinite(duty)
            || temperature != std::floor(temperature) || duty != std::floor(duty)
            || temperature < 0 || temperature > 100 || duty < 0 || duty > 100) {
            error = "Fan points must be integral temperatures and percentages in 0–100"; return {};
        }
        curve[index] = char(int(temperature)); curve[index + 8] = char(int(duty));
    }
    error = validateFactory(curve);
    return error.isEmpty() ? curve : QByteArray();
}
AsusReply FanController::readFactory(const AsusWmiController &asus, bool gpu, int performanceMode) {
    // Curve selector: Balanced=0, Turbo=2, Silent=1 (not the performance-mode numbering).
    const int selector = performanceMode == 1 ? 2 : performanceMode == 2 ? 1 : 0;
    auto reply = asus.read(gpu ? AsusEndpoint::GpuCurve : AsusEndpoint::CpuCurve, selector);
    if (reply.ok()) {
        const auto error = validateFactory(reply.bytes);
        if (!error.isEmpty()) { reply.state = CapabilityState::Unsupported; reply.reason = error; }
    }
    return reply;
}
int FanController::rpm(const AsusReply &reply) {
    // ASUS endpoint encodes hundreds of RPM, not a fan percentage.
    return reply.ok() && reply.value >= 0 && reply.value <= 120 ? reply.value * 100 : -1;
}
