#include "plugin_api.h"
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace {
    double computeDeg(double radians) {
        if (!std::isfinite(radians)) {
            throw std::domain_error("Аргумент deg не является конечным числом");
        }
        const double pi = std::acos(-1.0);
        return radians * (180.0 / pi);
    }

    int CALL_CONV executeDeg(double x, double* outResult, char* errorBuf, std::size_t errorBufSize) {
        if (!outResult) return -1;
        try {
            *outResult = computeDeg(x);
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
        "deg",
        &executeDeg
    };
}

PLUGIN_API const PluginDescriptor* CALL_CONV RegisterCalculatorPlugin() {
    return &kDescriptor;
}