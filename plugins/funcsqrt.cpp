#include "plugin_api.h"
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace {
    double computeSqrt(double x) {
        if (x < 0.0) {
            throw std::domain_error("Квадратный корень sqrt(x) не определен для отрицательных чисел (x < 0)");
        }
        return std::sqrt(x);
    }

    int CALL_CONV executeSqrt(double x, double* outResult, char* errorBuf, std::size_t errorBufSize) {
        if (!outResult) return -1;
        try {
            *outResult = computeSqrt(x);
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
        "sqrt",
        &executeSqrt
    };
}

PLUGIN_API const PluginDescriptor* CALL_CONV RegisterCalculatorPlugin() {
    return &kDescriptor;
}