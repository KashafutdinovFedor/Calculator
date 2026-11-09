#include "plugin_helpers.h"

namespace {
    double computeDeg(double radians) {
        plugin_helpers::requireFinite(radians, "deg");
        return radians * (180.0 / plugin_helpers::kPi);
    }
}

DEFINE_CALC_PLUGIN("deg", computeDeg)
