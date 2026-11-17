#include "AsusSensors.h"
#include "AsusWmiController.h"
#include "FanController.h"

CpuSensorSample AsusSensors::sample() const {
    CpuSensorSample result;
    AsusWmiController asus;
    result.cpuFanRpm = FanController::rpm(asus.read(AsusEndpoint::CpuFan));
    result.gpuFanRpm = FanController::rpm(asus.read(AsusEndpoint::GpuFan));
    const auto mode = asus.read(AsusEndpoint::Performance);
    if (mode.ok()) result.coolingMode = "Active ASUS mode unavailable";
    const auto eco = asus.read(AsusEndpoint::GpuEco);
    if (eco.ok() && eco.value == 1) result.gpuActivity = "GPU SLEEPING (ECO)";
    else if (eco.ok() && eco.value == 0) result.gpuActivity = "GPU AVAILABLE · activity unmeasured";
    const auto temperature = asus.read(AsusEndpoint::CpuTemperature);
    if (temperature.ok() && temperature.value > 0 && temperature.value <= 125) {
        result.temperatureC = temperature.value;
        result.temperatureSource = "ASUS firmware CPU sensor (not direct AMD Tdie)";
    } else result.temperatureSource = "Unavailable: " + temperature.reason;
    return result;
}
