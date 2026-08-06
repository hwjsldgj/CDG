#include "11_utils.h"
#include <stdexcept>
#include <cwchar>

int SafeParseInt(const std::string& val, int defaultVal) {
    try {
        int v = std::stoi(val);
        if (v < 0) return defaultVal;
        return v;
    } catch (...) { return defaultVal; }
}

double SafeParseDouble(const std::string& val, double defaultVal) {
    try {
        double v = std::stod(val);
        if (v < 0) return defaultVal;
        return v;
    } catch (...) { return defaultVal; }
}

std::wstring Utf8ToWide(const std::string& utf8) {
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, NULL, 0);
    if (len == 0) return L"";
    std::wstring wstr(len - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &wstr[0], len);
    return wstr;
}

int VisualWidth(const wchar_t* str, int len) {
    int w = 0;
    for (int i = 0; i < len; ++i) {
        if (str[i] >= 0x4E00 && str[i] <= 0x9FA5) w += 2;
        else w += 1;
    }
    return w;
}

int VisualWidth(const std::wstring& str) {
    return VisualWidth(str.c_str(), (int)str.length());
}

void Trim(std::string& s) {
    s.erase(0, s.find_first_not_of(" \t\r\n"));
    s.erase(s.find_last_not_of(" \t\r\n") + 1);
}