#include "utils.h"
#include <stdexcept>

int SafeParseInt(const std::string& val, int defaultVal) {
    try {
        int v = std::stoi(val);
        if (v < 0) return defaultVal;
        return v;
    } catch (...) {
        return defaultVal;
    }
}

double SafeParseDouble(const std::string& val, double defaultVal) {
    try {
        double v = std::stod(val);
        if (v < 0) return defaultVal;
        return v;
    } catch (...) {
        return defaultVal;
    }
}
