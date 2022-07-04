#pragma once
#include<windows.h>
//Clear GotoXy SetTitle SetColor SetCursor SetConsoleSize

void Clear(void);

void GotoXY(int _x, int _y);

void SetTitle(char* _szConsoleName);

void SetColor(unsigned char _bgColor, unsigned char _TextColor);

//void SetCursor(BOOL _bShow);

void SetConsoleSize(int _col, int _lines);

