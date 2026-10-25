#include "plugin_api.h"
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace {
    double computeExp(double x) {
        double res = std::exp(x);
        if (!std::isfinite(res)) {
            throw std::overflow_error("Переполнение при вычислении exp(x)");
        }
        return res;
    }

    int CALL_CONV executeExp(double x, double* outResult, char* errorBuf, std::size_t errorBufSize) {
        if (!outResult) return -1;
        try {
            *outResult = computeExp(x);
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
        "exp",
        &executeExp
    };
}

PLUGIN_API const PluginDescriptor* CALL_CONV RegisterCalculatorPlugin() {
    return &kDescriptor;
}