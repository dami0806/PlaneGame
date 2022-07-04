#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include "Console.h"
#define MAX 30

/*
0,0
+
*/

int main() {
	int x, y;
	SetConsoleSize(30, 30);
	while (1) {
		
		
		
		if (GetAsyncKeyState(VK_LEFT & 0x8000)) {
			x--;
			if (x < 0) x = 0;
		}
		if (GetAsyncKeyState(VK_RIGHT & 0x8000)) {
			x++;
			if (x > 28) x = 28;
		}GotoXY(14, 28);
		printf("бу");
		
	
	}system("pause");
	return 0;
}
