/****************************************************************************
** Meta object code from reading C++ file 'SystemMonitor.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.12.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../backend/SystemMonitor.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'SystemMonitor.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.12.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN13SystemMonitorE_t {};
} // unnamed namespace

template <> constexpr inline auto SystemMonitor::qt_create_metaobjectdata<qt_meta_tag_ZN13SystemMonitorE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "SystemMonitor",
        "updated",
        "",
        "batteryPercent",
        "onAC",
        "charging",
        "remainingMinutes",
        "cpuPercent",
        "ramUsedGB",
        "ramTotalGB",
        "ramRatio"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'updated'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
        // property 'batteryPercent'
        QtMocHelpers::PropertyData<int>(3, QMetaType::Int, QMC::DefaultPropertyFlags, 0),
        // property 'onAC'
        QtMocHelpers::PropertyData<bool>(4, QMetaType::Bool, QMC::DefaultPropertyFlags, 0),
        // property 'charging'
        QtMocHelpers::PropertyData<bool>(5, QMetaType::Bool, QMC::DefaultPropertyFlags, 0),
        // property 'remainingMinutes'
        QtMocHelpers::PropertyData<int>(6, QMetaType::Int, QMC::DefaultPropertyFlags, 0),
        // property 'cpuPercent'
        QtMocHelpers::PropertyData<double>(7, QMetaType::Double, QMC::DefaultPropertyFlags, 0),
        // property 'ramUsedGB'
        QtMocHelpers::PropertyData<double>(8, QMetaType::Double, QMC::DefaultPropertyFlags, 0),
        // property 'ramTotalGB'
        QtMocHelpers::PropertyData<double>(9, QMetaType::Double, QMC::DefaultPropertyFlags, 0),
        // property 'ramRatio'
        QtMocHelpers::PropertyData<double>(10, QMetaType::Double, QMC::DefaultPropertyFlags, 0),
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<SystemMonitor, qt_meta_tag_ZN13SystemMonitorE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject SystemMonitor::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN13SystemMonitorE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN13SystemMonitorE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN13SystemMonitorE_t>.metaTypes,
    nullptr
} };

void SystemMonitor::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<SystemMonitor *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->updated(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (SystemMonitor::*)()>(_a, &SystemMonitor::updated, 0))
            return;
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast<int*>(_v) = _t->batteryPercent(); break;
        case 1: *reinterpret_cast<bool*>(_v) = _t->onAC(); break;
        case 2: *reinterpret_cast<bool*>(_v) = _t->charging(); break;
        case 3: *reinterpret_cast<int*>(_v) = _t->remainingMinutes(); break;
        case 4: *reinterpret_cast<double*>(_v) = _t->cpuPercent(); break;
        case 5: *reinterpret_cast<double*>(_v) = _t->ramUsedGB(); break;
        case 6: *reinterpret_cast<double*>(_v) = _t->ramTotalGB(); break;
        case 7: *reinterpret_cast<double*>(_v) = _t->ramRatio(); break;
        default: break;
        }
    }
}

const QMetaObject *SystemMonitor::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SystemMonitor::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN13SystemMonitorE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int SystemMonitor::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 1)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 1;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    }
    return _id;
}

// SIGNAL 0
void SystemMonitor::updated()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}
QT_WARNING_POP
