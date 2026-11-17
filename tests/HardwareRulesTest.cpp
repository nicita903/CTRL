#include "hardware/FanController.h"
#include "hardware/AsusProtocol.h"
#include <cstdio>
#include <limits>

int main() {
    int failures = 0;
    auto check = [&](bool condition, const char *message) {
        if (!condition) { std::fprintf(stderr, "FAIL: %s\n", message); ++failures; }
    };
    QByteArray factory;
    for (int value : {30,40,50,60,70,80,90,100, 0,10,20,35,50,70,90,100}) factory.append(char(value));
    check(FanController::validateFactory(factory).isEmpty(), "valid eight-point factory curve");
    check(asusCurvePayload(factory) == factory, "standard curve response");
    QByteArray padded = factory + QByteArray(48, '\0');
    check(asusCurvePayload(padded) == factory, "observed 64-byte zero-padded transport");
    padded[63] = 1;
    check(asusCurvePayload(padded).isEmpty(), "reject unrecognized nonzero transport suffix");
    check(asusCurvePayload(factory + QByteArray(16, '\0')).isEmpty(), "reject unverified transport sizes");
    auto custom = factory;
    custom[9] = 15;
    check(FanController::validateCustom(custom, factory).isEmpty(), "safe duty increase");
    custom[9] = 5;
    check(!FanController::validateCustom(custom, factory).isEmpty(), "reject reduced cooling");
    custom = factory; custom[1] = 41;
    check(!FanController::validateCustom(custom, factory).isEmpty(), "reject unverified temperature bins");
    custom = factory; custom[1] = 30;
    check(!FanController::validateFactory(custom).isEmpty(), "reject repeated temperatures");
    custom = factory; custom[10] = 101;
    check(!FanController::validateFactory(custom).isEmpty(), "reject duty above 100 percent");
    custom = factory; custom[10] = 0;
    check(!FanController::validateFactory(custom).isEmpty(), "reject decreasing duty");
    check(!FanController::validateFactory(QByteArray(16, '\0')).isEmpty(), "reject empty response");
    check(!FanController::validateFactory(factory.first(15)).isEmpty(), "reject truncated response");
    QString error;
    check(FanController::encode(FanController::points(factory), error) == factory && error.isEmpty(), "lossless wire encoding");
    auto points = FanController::points(factory);
    points[0] = QVariantMap{{"temperature", 30}, {"duty", 256}};
    check(FanController::encode(points, error).isEmpty() && !error.isEmpty(), "prevent byte overflow");
    points[0] = QVariantMap{{"temperature", 30}, {"duty", -1}};
    check(FanController::encode(points, error).isEmpty(), "prevent negative values");
    points[0] = QVariantMap{{"temperature", 30}, {"duty", 0.5}};
    check(FanController::encode(points, error).isEmpty(), "prevent fractional truncation");
    points[0] = QVariantMap{{"temperature", 30}, {"duty", std::numeric_limits<double>::quiet_NaN()}};
    check(FanController::encode(points, error).isEmpty(), "prevent NaN");
    check(FanController::rpm({CapabilityState::Supported, {}, {}, 28}) == 2800, "ASUS hundreds-of-RPM units");
    check(FanController::rpm({CapabilityState::Supported, {}, {}, 0}) == 0, "real stopped fan");
    check(FanController::rpm({CapabilityState::Unsupported, {}, {}, 28}) == -1, "unavailable fan");
    check(HardwareIdentity{"ASUSTeK COMPUTER INC.", "ASUS TUF Gaming A15 FA507NV_FA507NV"}.allowlisted(), "target model recognized");
    check(!HardwareIdentity{"ASUSTeK COMPUTER INC.", "FA507NVR"}.allowlisted(), "reject nearby model");
    check(!HardwareIdentity{"Another vendor", "FA507NV"}.allowlisted(), "require ASUS manufacturer");
    return failures ? 1 : 0;
}
