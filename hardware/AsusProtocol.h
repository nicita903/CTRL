#pragma once
#include <QByteArray>
#include <algorithm>

// The FA507NV ATK transport returns a 16-byte leading curve and a zero-filled
// suffix when supplied a 64-byte output buffer. Other layouts are not inferred.
inline QByteArray asusCurvePayload(const QByteArray &response) {
    if (response.size() == 16) return response;
    if (response.size() != 64) return {};
    if (!std::all_of(response.cbegin() + 16, response.cend(), [](char value) { return value == 0; })) return {};
    return response.first(16); // FanController separately validates every curve point.
}
