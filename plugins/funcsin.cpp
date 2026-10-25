#include "plugin_api.h"
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace {
    // Внутренняя функция, способная бросать стандартные исключения C++
    double computeSinDeg(double degrees) {
        if (!std::isfinite(degrees)) {
            throw std::domain_error("Аргумент sin не является конечным числом");
        }
        const double pi = std::acos(-1.0);
        double radians = degrees * (pi / 180.0);
        return std::sin(radians);
    }

    int CALL_CONV executeSin(double x, double* outResult, char* errorBuf, std::size_t errorBufSize) {
        if (!outResult) return -1;
        try {
            *outResult = computeSinDeg(x);
            return 0;
        }
        catch (const std::exception& e) {
            if (errorBuf && errorBufSize > 0) {
                strncpy_s(errorBuf, errorBufSize, e.what(), _TRUNCATE);
            }
            return 1;
        }
    }

    const PluginDescriptor kDescriptor = {
        PLUGIN_ABI_VERSION,
        "sin", // Имя функции зашито внутри плагина и не зависит от имени .dll файла
        &executeSin
    };
}

PLUGIN_API const PluginDescriptor* CALL_CONV RegisterCalculatorPlugin() {
    return &kDescriptor;
}