#include "plugin_helpers.h"

namespace {
    double computeCosDeg(double degrees) {
        plugin_helpers::requireFinite(degrees, "cos");
        return std::cos(degrees * (plugin_helpers::kPi / 180.0));
    }
}

DEFINE_CALC_PLUGIN("cos", computeCosDeg)
