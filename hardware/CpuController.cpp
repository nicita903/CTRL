#include "CpuController.h"
#ifdef Q_OS_WIN
#include <windows.h>
#include <powrprof.h>
#include <cstring>
namespace {
constexpr GUID processor{0x54533251, 0x82be, 0x4824, {0x96, 0xc1, 0x47, 0xb6, 0x0b, 0x74, 0x0d, 0x00}};
constexpr GUID boost{0xbe337238, 0x0d82, 0x4146, {0xa9, 0x60, 0x4f, 0x37, 0x49, 0xd4, 0x70, 0xc7}};
QString schemeId(const GUID &guid) { return QString::fromLatin1(QByteArray(reinterpret_cast<const char *>(&guid), sizeof(guid)).toHex()); }
CapabilityState errorState(DWORD status) { return status == ERROR_ACCESS_DENIED ? CapabilityState::PermissionRequired : CapabilityState::Unsupported; }
}
#endif
QVariantMap CpuController::inspectBoost() {
    QVariantMap result;
#ifdef Q_OS_WIN
    GUID *scheme = nullptr;
    DWORD error = PowerGetActiveScheme(nullptr, &scheme);
    if (error != ERROR_SUCCESS) return capability(errorState(error), QString("PowerGetActiveScheme error %1").arg(error));
    result["scheme"] = schemeId(*scheme);
    DWORD ac = 0, dc = 0;
    error = PowerReadACValueIndex(nullptr, scheme, &processor, &boost, &ac);
    if (error == ERROR_SUCCESS) error = PowerReadDCValueIndex(nullptr, scheme, &processor, &boost, &dc);
    LocalFree(scheme);
    if (error != ERROR_SUCCESS) return capability(errorState(error), QString("Boost policy read error %1").arg(error));
    QVariantList options;
    for (ULONG index = 0; index < 32; ++index) {
        DWORD type = 0, value = 0, size = sizeof(value);
        error = PowerReadPossibleValue(nullptr, &processor, &boost, &type, index,
                                       reinterpret_cast<PUCHAR>(&value), &size);
        if (error == ERROR_NO_MORE_ITEMS || error == ERROR_FILE_NOT_FOUND) break;
        if (error != ERROR_SUCCESS || type != REG_DWORD || size != sizeof(DWORD)) continue;
        const QString label = value == 0 ? "DISABLED" : value == 1 ? "ENABLED" : value == 2 ? "AGGRESSIVE" : value == 3 ? "EFFICIENT" : "";
        if (!label.isEmpty()) options.append(QVariantMap{{"label", label}, {"value", uint(value)}});
    }
    result.insert(capability(options.isEmpty() ? CapabilityState::Unsupported : CapabilityState::Supported,
        options.isEmpty() ? "Windows did not enumerate supported boost values" : "Active Windows plan; AC and battery policies are separate"));
    result["ac"] = uint(ac);
    result["dc"] = uint(dc);
    result["options"] = options;
#else
    result = capability(CapabilityState::Unsupported, "Windows only");
#endif
    return result;
}

QString CpuController::setBoost(bool ac, int value, const QString &expectedScheme) {
#ifdef Q_OS_WIN
    const auto current = inspectBoost();
    if (!current.value("enabled").toBool()) return current.value("reason").toString();
    if (current.value("scheme").toString() != expectedScheme) return "Active Windows plan changed; inspect and stage again";
    bool allowed = false;
    for (const auto &item : current.value("options").toList())
        if (item.toMap().value("value").toInt() == value) allowed = true;
    if (!allowed) return "Boost policy value not enumerated by Windows";
    const auto bytes = QByteArray::fromHex(expectedScheme.toLatin1());
    if (bytes.size() != sizeof(GUID)) return "Invalid saved plan identity";
    GUID scheme;
    std::memcpy(&scheme, bytes.constData(), sizeof(scheme));
    const DWORD previous = current.value(ac ? "ac" : "dc").toUInt();
    auto write = [&](DWORD index) {
        return ac ? PowerWriteACValueIndex(nullptr, &scheme, &processor, &boost, index)
                  : PowerWriteDCValueIndex(nullptr, &scheme, &processor, &boost, index);
    };
    DWORD error = write(DWORD(value));
    if (error != ERROR_SUCCESS) return QString("Boost write error %1%2").arg(error).arg(error == ERROR_ACCESS_DENIED ? " (PERMISSION REQUIRED)" : "");
    GUID *active = nullptr;
    const bool same = PowerGetActiveScheme(nullptr, &active) == ERROR_SUCCESS && schemeId(*active) == expectedScheme;
    if (active) LocalFree(active);
    error = same ? PowerSetActiveScheme(nullptr, &scheme) : ERROR_RETRY;
    DWORD readback = ~0u;
    if (error == ERROR_SUCCESS) error = ac ? PowerReadACValueIndex(nullptr, &scheme, &processor, &boost, &readback)
                                          : PowerReadDCValueIndex(nullptr, &scheme, &processor, &boost, &readback);
    if (error != ERROR_SUCCESS || readback != DWORD(value)) {
        const DWORD rollback = write(previous);
        const DWORD activated = same && rollback == ERROR_SUCCESS ? PowerSetActiveScheme(nullptr, &scheme) : ERROR_RETRY;
        return QString("Boost apply/readback failed (%1); previous value restore=%2, activation=%3").arg(error).arg(rollback).arg(activated);
    }
    return {}; // Success requires both activation and readback.
#else
    Q_UNUSED(ac); Q_UNUSED(value); Q_UNUSED(expectedScheme); return "Windows only";
#endif
}
