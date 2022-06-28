#include<stdio.h>
#include<stdlib.h>
#include "Console.h"


/*
|----|
|    |  console:(30,30) бу:(14,28)
|____|

*/
int main(void) {
	int x = 14, y = 28;
	int bx = 0, by = 0;
	bool bullet= false;
	SetConsoleSize(30, 30);
	while (1) {
		Clear();
		if (GetAsyncKeyState(VK_LEFT) & 0x8000) {
			x--;
			if (x < 0) x = 0;
		}
		if (GetAsyncKeyState(VK_RIGHT) & 0x8000) {
			x++;
			if (x > 28) x = 28;
		}
		if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
			if (!bullet) {
				bx = x;
				by = y - 1;
				bullet = true;
			}


		}
		if (bullet) {
			GotoXY(bx, by);
			printf("б▄");
			by--;
			if (by < 0)bullet = false;
		}


		GotoXY(x,y);
		printf("бу");
		Sleep(60);
	}
	system("pause");

	
	return 0;
}










