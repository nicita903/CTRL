#pragma once
#include "AsusWmiController.h"

class FanController {
public:
    static QString validateFactory(const QByteArray &curve);
    static QString validateCustom(const QByteArray &curve, const QByteArray &factory);
    static QVariantList points(const QByteArray &curve);
    static QByteArray encode(const QVariantList &points, QString &error);
    static AsusReply readFactory(const AsusWmiController &asus, bool gpu, int performanceMode);
    static int rpm(const AsusReply &reply);
};
