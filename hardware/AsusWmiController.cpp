#include "AsusWmiController.h"
#include "AsusProtocol.h"
#include <cstring>
#ifdef Q_OS_WIN
#include <windows.h>
#include <wbemidl.h>
#include <oleauto.h>
#endif

namespace {
QByteArray words(quint32 first, quint32 second) {
    QByteArray data(8, '\0');
    std::memcpy(data.data(), &first, 4);
    std::memcpy(data.data() + 4, &second, 4);
    return data;
}
}
AsusWmiController::AsusWmiController() {
#ifdef Q_OS_WIN
    m_handle = CreateFileW(L"\\\\.\\ATKACPI", GENERIC_READ | GENERIC_WRITE,
                           FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
    if (m_handle == INVALID_HANDLE_VALUE) m_error = GetLastError();
#endif
}
AsusWmiController::~AsusWmiController() {
#ifdef Q_OS_WIN
    if (m_handle != INVALID_HANDLE_VALUE) CloseHandle(m_handle);
#endif
}
AsusReply AsusWmiController::call(quint32 method, const QByteArray &arguments, bool buffer) const {
    AsusReply reply;
#ifdef Q_OS_WIN
    if (m_handle == INVALID_HANDLE_VALUE) {
        reply.state = m_error == ERROR_ACCESS_DENIED ? CapabilityState::PermissionRequired : CapabilityState::DriverUnavailable;
        reply.reason = QString("ASUS interface open error %1").arg(m_error);
        return reply;
    }
    QByteArray request = words(method, arguments.size()) + arguments;
    // Some OEM driver revisions require transport headroom for a 16-byte curve.
    // Only the standard payload or the observed, entirely zero-padded layout is accepted.
    QByteArray output(buffer ? 64 : 16, '\0');
    DWORD returned = 0;
    if (!DeviceIoControl(m_handle, 0x0022240c, request.data(), request.size(), output.data(), output.size(), &returned, nullptr)) {
        const auto error = GetLastError();
        reply.state = error == ERROR_ACCESS_DENIED ? CapabilityState::PermissionRequired : CapabilityState::Error;
        reply.reason = QString("%1ASUS query error %2").arg(error == ERROR_ACCESS_DENIED ? "PERMISSION REQUIRED: " : "").arg(error);
        return reply;
    }
    if (returned < 4 || returned > DWORD(output.size())) { reply.reason = "Invalid ASUS response length"; return reply; }
    reply.bytes = output.first(returned);
    quint32 status = 0;
    std::memcpy(&status, output.constData(), 4);
    if (method == 0x53564544) {
        reply.state = status == 1 ? CapabilityState::Supported : CapabilityState::Error;
        reply.reason = QString("Firmware write acknowledgement %1").arg(status);
    } else if (buffer) {
        const auto payload = asusCurvePayload(reply.bytes);
        reply.state = payload.size() == 16 ? CapabilityState::Supported : CapabilityState::Unsupported;
        reply.reason = reply.ok() ? "Leading 16-byte factory curve; zero transport padding validated separately from curve points"
                                 : QString("Unsupported factory response layout (%1 bytes)").arg(returned);
        if (reply.ok()) reply.bytes = payload;
    } else {
        reply.state = (status & 0xffff0000) == 0x10000 ? CapabilityState::Supported : CapabilityState::Unsupported;
        reply.value = reply.ok() ? int(status & 0xffff) : -1;
        reply.reason = reply.ok() ? "Firmware endpoint present; writes require explicit Apply" : "Firmware endpoint absent or unexpected flags";
    }
#else
    Q_UNUSED(method); Q_UNUSED(arguments); Q_UNUSED(buffer);
    reply.state = CapabilityState::Unsupported;
    reply.reason = "Windows only";
#endif
    return reply;
}
AsusReply AsusWmiController::read(AsusEndpoint endpoint, int mode) const {
    return call(0x53545344, words(quint32(endpoint), quint32(mode)),
                endpoint == AsusEndpoint::CpuCurve || endpoint == AsusEndpoint::GpuCurve);
}
AsusReply AsusWmiController::setPerformance(int mode) const {
    if (mode < 0 || mode > 2) return {CapabilityState::Error, "Invalid performance mode", {}, -1};
    return call(0x53564544, words(quint32(AsusEndpoint::Performance), mode));
}
AsusReply AsusWmiController::setGpuEco(bool eco) const {
    return call(0x53564544, words(quint32(AsusEndpoint::GpuEco), eco ? 1 : 0));
}
AsusReply AsusWmiController::setCurve(bool gpu, const QByteArray &curve) const {
    if (curve.size() != 16) return {CapabilityState::Error, "A fan curve must have 16 bytes", {}, -1};
    QByteArray arguments(4, '\0');
    const quint32 endpoint = quint32(gpu ? AsusEndpoint::GpuCurve : AsusEndpoint::CpuCurve);
    std::memcpy(arguments.data(), &endpoint, 4);
    return call(0x53564544, arguments + curve);
}

HardwareIdentity AsusWmiController::identify() {
    HardwareIdentity result;
#ifdef Q_OS_WIN
    const UINT size = GetSystemFirmwareTable(0x52534d42, 0, nullptr, 0);
    if (size < 8 || size > 16 * 1024 * 1024) return result;
    QByteArray table(size, '\0');
    if (GetSystemFirmwareTable(0x52534d42, 0, table.data(), size) != size) return result;
    quint32 length = 0;
    std::memcpy(&length, table.constData() + 4, 4);
    if (length > size - 8) return result;
    int position = 8;
    const int end = 8 + int(length);
    while (position + 4 <= end) {
        const auto *data = reinterpret_cast<const unsigned char *>(table.constData() + position);
        const int formatted = data[1];
        if (formatted < 4 || position + formatted > end) break;
        int next = position + formatted;
        QStringList strings;
        while (next + 1 < end && !(table[next] == '\0' && table[next + 1] == '\0')) {
            int zero = next;
            while (zero < end && table[zero] != '\0') ++zero;
            if (zero == end) return result;
            strings.append(QString::fromLatin1(table.constData() + next, zero - next));
            next = zero;
            if (next + 1 < end && table[next + 1] != '\0') ++next;
        }
        if (next + 1 >= end) break;
        if (data[0] == 1 && formatted >= 6) {
            auto stringAt = [&](int index) { return index > 0 && index <= strings.size() ? strings[index - 1] : QString(); };
            result.manufacturer = stringAt(data[4]);
            result.model = stringAt(data[5]);
            return result;
        }
        position = next + 2;
    }
#endif
    return result;
}

QString AsusWmiController::discoverWmi() {
#ifdef Q_OS_WIN
    const HRESULT initialized = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(initialized)) return "ERROR: COM initialization";
    IWbemLocator *locator = nullptr;
    IWbemServices *services = nullptr;
    IEnumWbemClassObject *rows = nullptr;
    IWbemClassObject *row = nullptr;
    QString result = "ASUS WMI class unavailable; native ATK transport is inspected separately";
    if (SUCCEEDED(CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER, IID_IWbemLocator,
                                   reinterpret_cast<void **>(&locator)))) {
        BSTR ns = SysAllocString(L"ROOT\\WMI");
        HRESULT hr = locator->ConnectServer(ns, nullptr, nullptr, nullptr, 0, nullptr, nullptr, &services);
        SysFreeString(ns);
        if (SUCCEEDED(hr)) {
            CoSetProxyBlanket(services, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr,
                              RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE);
            BSTR language = SysAllocString(L"WQL");
            BSTR query = SysAllocString(L"SELECT * FROM AsusAtkWmi_WMNB");
            hr = services->ExecQuery(language, query, WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &rows);
            SysFreeString(language); SysFreeString(query);
            ULONG count = 0;
            if (SUCCEEDED(hr) && SUCCEEDED(rows->Next(500, 1, &row, &count)) && count == 1)
                result = "root\\WMI AsusAtkWmi_WMNB present (discovery only)";
            else if (hr == WBEM_E_ACCESS_DENIED) result = "PERMISSION REQUIRED: root\\WMI discovery";
        } else result = QString("WMI discovery error 0x%1").arg(quint32(hr), 8, 16, QChar('0'));
    }
    if (row) row->Release();
    if (rows) rows->Release();
    if (services) services->Release();
    if (locator) locator->Release();
    CoUninitialize();
    return result;
#else
    return "UNSUPPORTED: Windows only";
#endif
}
