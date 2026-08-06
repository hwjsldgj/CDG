#ifndef RENDER_H
#define RENDER_H

#include <windows.h>

void InitConsole();
void Draw();
void DrawMenu();
void DrawPauseMenu();              // 新增
void DrawGameOver();
void ResetConsoleWindow(HANDLE hConsole);
void EnsureBufferSize(HANDLE hConsole);
bool IsConsoleForeground();

#endif