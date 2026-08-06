#ifndef CONSOLE_H
#define CONSOLE_H

#include <windows.h>

void InitConsole();
void EnsureBufferSize(HANDLE hConsole);
void ResetConsoleWindow(HANDLE hConsole);
bool IsConsoleForeground();

#endif