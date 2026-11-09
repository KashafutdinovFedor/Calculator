#include "plugin_helpers.h"

namespace {
    double computeSinDeg(double degrees) {
        plugin_helpers::requireFinite(degrees, "sin");
        return std::sin(degrees * (plugin_helpers::kPi / 180.0));
    }
}


DEFINE_CALC_PLUGIN("sin", computeSinDeg)
