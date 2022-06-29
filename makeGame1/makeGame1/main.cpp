#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include "Console.h"
#define MAX 30

/*
|----|
|    |  console:(30,30) ¡ã:(14,28)
|____|

·£´ýÇÔ¼ö srand(time(NULL))    <time.h>
rand_x = (rand()%15)   <stdlib.h>

*/
typedef struct STAR{
	int star_x = 0;
	int star_y = 0;
	bool StarAct = false;


};
int main(void) {
	int x = 14, y = 28;
	int bx = 0, by = 0;
	bool bullet= false;
	int i=0;

	STAR star[MAX];
	
	srand(time(NULL));
	SetConsoleSize(30, 30);
	while (1) {

		Clear();
		for (i = 0; i < MAX; i++) {
			if (!star[i].StarAct) {
				star[i].star_x = (rand() % 15) * 2; //0,2,4,6...28
				star[i].star_y = 0;
				star[i].StarAct = true;
				break;

			}
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
		for (i = 0; i < MAX; i++) {
			if (star) {
				GotoXY(star[i].star_x, star[i].star_y);
				printf("¡Ù");
				star[i].star_y++;
				if (star[i].star_y > y)
					star[i].StarAct = false;
			}
		}


		GotoXY(x,y);
		printf("¡ã");
		Sleep(60);
	}
	system("pause");

	
	return 0;
}










