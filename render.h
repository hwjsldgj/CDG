#ifndef RENDER_H
#define RENDER_H

#include <windows.h>

void InitConsole();
void Draw();
void DrawMenu();
void DrawGameOver();               // 新增：绘制结束画面
void ShowGameOver();               // 保留旧版（可废弃，但暂保留）
void ResetConsoleWindow(HANDLE hConsole);
void EnsureBufferSize(HANDLE hConsole);
bool IsConsoleForeground();

#endif