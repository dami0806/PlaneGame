#include<stdio.h>
#include<stdlib.h>
#include "Console.h"


int main(void) {
	int x = 14, y = 28;
	//콘솔창 크기조정
	SetConsoleSize(30, 30);
	while (1) {
		Clear();
		if (GetAsyncKeyState(VK_LEFT) & 0x8000) {
			x--;
			//화면 넘어가지 않게
			if (x < 0) x = 0;
		}if (GetAsyncKeyState(VK_RIGHT) & 0x8000) {
			x++;
			if (x > 28) x = 28;
		}

		GotoXY(x, y);
		printf("▲");
		//Sleep(50);

	}
	system("pause");
	return 0;
}