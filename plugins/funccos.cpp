#include "plugin_api.h"
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace {
    double computeCosDeg(double degrees) {
        if (!std::isfinite(degrees)) {
            throw std::domain_error("Аргумент cos не является конечным числом");
        }
        const double pi = std::acos(-1.0);
        return std::cos(degrees * (pi / 180.0));
    }

    int CALL_CONV executeCos(double x, double* outResult, char* errorBuf, std::size_t errorBufSize) {
        if (!outResult) return -1;
        try {
            *outResult = computeCosDeg(x);
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
        "cos",
        &executeCos
    };
}

PLUGIN_API const PluginDescriptor* CALL_CONV RegisterCalculatorPlugin() {
    return &kDescriptor;
}