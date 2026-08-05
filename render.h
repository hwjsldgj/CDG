#ifndef RENDER_H
#define RENDER_H

#include <windows.h>

void InitConsole();
void Draw();
void ResetConsoleWindow(HANDLE hConsole);
void EnsureBufferSize(HANDLE hConsole);
bool IsConsoleForeground();

#endif