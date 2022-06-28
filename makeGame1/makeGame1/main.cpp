#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include "Console.h"


/*
|----|
|    |  console:(30,30) ¡ã:(14,28)
|____|

·£´ýÇÔ¼ö srand(time(NULL))    <time.h>
rand_x = (rand()%15)   <stdlib.h>

*/
int main(void) {
	int x = 14, y = 28;
	int bx = 0, by = 0;
	bool bullet= false;

	int star_x = 0, star_y = 0;
	bool star = false;
	
	srand(time(NULL));
	SetConsoleSize(30, 30);
	while (1) {

		Clear();
		if (!star) {
			star_x = (rand() % 15) * 2; //0,2,4,6...28
			star_y = 0;
			star = true;

		}
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
			printf("¡Ü");
			by--;
			if (by < 0)bullet = false;
		}
		if (star) {
			GotoXY(star_x,star_y);
			printf("¡Ù");
			star_y++;
			if (star_y >y)
				star = false;
		}


		GotoXY(x,y);
		printf("¡ã");
		Sleep(60);
	}
	system("pause");

	
	return 0;
}










