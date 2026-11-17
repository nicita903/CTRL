#pragma once
#include <QVariantMap>
#include <QStringList>

enum class CapabilityState { Supported, Unsupported, PermissionRequired, DriverUnavailable, Error };
inline QString stateName(CapabilityState state) {
    switch (state) {
    case CapabilityState::Supported: return "SUPPORTED";
    case CapabilityState::Unsupported: return "UNSUPPORTED";
    case CapabilityState::PermissionRequired: return "PERMISSION REQUIRED";
    case CapabilityState::DriverUnavailable: return "DRIVER UNAVAILABLE";
    default: return "ERROR";
    }
}
inline QVariantMap capability(CapabilityState state, const QString &reason, const QVariant &value = {}) {
    return {{"state", stateName(state)}, {"reason", reason}, {"value", value},
            {"enabled", state == CapabilityState::Supported}};
}
struct HardwareIdentity {
    QString manufacturer;
    QString model;
    bool allowlisted() const {
        const QString base = model.section('_', 0, 0).trimmed().toUpper();
        return (manufacturer.contains("ASUS", Qt::CaseInsensitive) || manufacturer.contains("ASUSTeK", Qt::CaseInsensitive))
            && (base == "ASUS TUF GAMING A15 FA507NV" || base == "FA507NV");
    }
};
