#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <windows.h>

int SafeParseInt(const std::string& val, int defaultVal);
double SafeParseDouble(const std::string& val, double defaultVal);

std::wstring Utf8ToWide(const std::string& utf8);
int VisualWidth(const wchar_t* str, int len);
int VisualWidth(const std::wstring& str);

void Trim(std::string& s);

#endif