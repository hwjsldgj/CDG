#ifndef UTILS_H
#define UTILS_H

#include <string>

int SafeParseInt(const std::string& val, int defaultVal);
double SafeParseDouble(const std::string& val, double defaultVal);

#endif