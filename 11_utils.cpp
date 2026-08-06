#include "11_utils.h"
#include "16_logger.h"
#include <stdexcept>
#include <cwchar>

int SafeParseInt(const std::string& val, int defaultVal) {
    try {
        int v = std::stoi(val);
        if (v < 0) {
            LOG_WARN(std::string("SafeParseInt: 负数 ") + val + "，使用默认值 " + std::to_string(defaultVal));
            return defaultVal;
        }
        return v;
    } catch (const std::invalid_argument& e) {
        LOG_WARN(std::string("SafeParseInt: 无效数字 ") + val + "，使用默认值 " + std::to_string(defaultVal) + "，错误：" + e.what());
        return defaultVal;
    } catch (const std::out_of_range& e) {
        LOG_WARN(std::string("SafeParseInt: 超出范围 ") + val + "，使用默认值 " + std::to_string(defaultVal) + "，错误：" + e.what());
        return defaultVal;
    } catch (...) {
        LOG_WARN(std::string("SafeParseInt: 未知错误 ") + val + "，使用默认值 " + std::to_string(defaultVal));
        return defaultVal;
    }
}

double SafeParseDouble(const std::string& val, double defaultVal) {
    try {
        double v = std::stod(val);
        // 允许负数，不做警告
        return v;
    } catch (const std::invalid_argument& e) {
        LOG_WARN(std::string("SafeParseDouble: 无效数字 ") + val + "，使用默认值 " + std::to_string(defaultVal) + "，错误：" + e.what());
        return defaultVal;
    } catch (const std::out_of_range& e) {
        LOG_WARN(std::string("SafeParseDouble: 超出范围 ") + val + "，使用默认值 " + std::to_string(defaultVal) + "，错误：" + e.what());
        return defaultVal;
    } catch (...) {
        LOG_WARN(std::string("SafeParseDouble: 未知错误 ") + val + "，使用默认值 " + std::to_string(defaultVal));
        return defaultVal;
    }
}

std::wstring Utf8ToWide(const std::string& utf8) {
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, NULL, 0);
    if (len == 0) {
        LOG_ERROR("Utf8ToWide: 转换失败，返回空字符串");
        return L"";
    }
    std::wstring wstr(len - 1, 0);
    int result = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &wstr[0], len);
    if (result == 0) {
        LOG_ERROR("Utf8ToWide: 转换失败，错误码 " + std::to_string(GetLastError()));
        return L"";
    }
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