#ifndef RENDER_H
#define RENDER_H

#include <windows.h>

void InitConsole();
void Draw();
void DrawMenu();                   // 新增
void ShowGameOver();
void ResetConsoleWindow(HANDLE hConsole);
void EnsureBufferSize(HANDLE hConsole);
bool IsConsoleForeground();

#endif