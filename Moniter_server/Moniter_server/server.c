
/*
====================================================================================================
Name        : Monitoring
Author      : Kim Hwi So
Version     : v1.0
Description : Network Programming Project
			   ������ Ŭ���̾�Ʈ �ڵ��� �ۼ��� �ڵ�(Send, Receive)�� ��õ� 1 - 9�� �ּ� ������ ���� ����
====================================================================================================
*/

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include <process.h>

#define BUF_SIZE 1024
#define MAX_CLIENTLIST_SIZE 100 // �ִ� ���� ������ Ŭ���̾�Ʈ ��
#define FIRST_CLIENT_NUMBER 1 // ���� Ŭ���̾�Ʈ ���� ��ȣ

#define FALSE 0
#define TRUE 1

#define PAUSEVALUE 100
#define NOSENSOR -1

void ErrorHandling(char* message);
void gotoXY(int x, int y);
void setColor(unsigned short text, unsigned short back);
void printBox();
void printClientBox(int x, int y);
void printMiddleLine(char* appString);
void printGuide();
void alignCenter(int columns, char* text);
unsigned WINAPI ClientUI(void* arg);
unsigned WINAPI ClientService(void* arg);

typedef struct ClientSensor {
	boolean pauseFlag; // ���� �Ͻ� �ߴ� ����
	char	sensorNum; // ���� ���� ��ȣ
	double	lowestLimit; // �� ������ ���� ���� ����
	double	highestLimit; // �� ������ �ְ� ���� ����
} Sensor;

typedef struct ClientInfo {
	boolean connection; // ���� ����
	char	clientNum; // Ŭ���̾�Ʈ ��ȣ
	int		cntSensor; // ��ġ�� ���� ��
	Sensor* sensor; // �� ���� ����ü �迭
	SOCKET	sHandle; // ���� ���� ��ũ����
} Client;

typedef struct ClientParam { // �����忡�� ���� ���� ����ü
	Client* clientList; // Ŭ���̾�Ʈ ����Ʈ
	char* cntClient; // ���� Ŭ���̾�Ʈ�� �ο��� ���� ��ȣ�̰� �ϳ� ������ ���� �̿� �Ϸ�� Ŭ���̾�Ʈ ��ȣ
} ClientParam;

boolean			printLog = FALSE; // �α� ȭ�� ��� ����
boolean			printUI = TRUE; // ����͸� UI ��� ����

int main(void) {
	WSADATA			wsaData;
	SOCKET			hServSock, hClntSock;
	SOCKADDR_IN		servAdr, clntAdr, clientAddr;

	int				adrSize;
	int				fdNum, retVal, strLen;
	double			sensorVal; // ���� ��

	char            initBuf[BUF_SIZE]; // ���� ���� �ʱ� ���� ������ ���� ����
	char			pauseBuf[BUF_SIZE]; // ���ܵ� ���� ���� ��ȣ�� ���� ����
	char			sensorValBuf[BUF_SIZE]; // ���� ���� ���� �� ���� ����
	char			cntPauseStartSensor; // �Ͻ� ���� ��û ���� ��
	char			pauseStartCntSensor; // ���� ���� ���� ��

	int				cntClient = FIRST_CLIENT_NUMBER; // �ֱ� ��ϵ� Ŭ���̾�Ʈ ���� ��ȣ�̰� �̿� �Ϸ�� ������ Ŭ���̾�Ʈ ���� ��ȣ
	Client			clientList[MAX_CLIENTLIST_SIZE]; // �� Ŭ���̾�Ʈ ������ ��� ����ü �迭

	Client currentClient; // ���� ������� Ŭ���̾�Ʈ
	int currentClientNum; // ���� ������� Ŭ���̾�Ʈ ��ȣ

	ClientParam clientParam;

	clientParam.clientList = clientList;
	clientParam.cntClient = &cntClient;


	// system("mode con:cols=102 lines=22"); // �ܼ� â ũ�� ����
	setColor(6, 0); // �ܼ� â �ؽ�Ʈ ���� ����

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) // ���� Ȯ��
		ErrorHandling("WSAStartup error");

	hServSock = socket(PF_INET, SOCK_STREAM, 0); // ���� �ּ� ����ü �ʱ�ȭ
	memset(&servAdr, 0, sizeof(servAdr));
	servAdr.sin_family = AF_INET;
	servAdr.sin_addr.s_addr = inet_addr("127.0.0.1");
	servAdr.sin_port = htons(9000);

	if (bind(hServSock, (SOCKADDR*)&servAdr, sizeof(servAdr)) == SOCKET_ERROR) // ���� Ȯ��
		ErrorHandling("Bind error");

	if (listen(hServSock, 5) == SOCKET_ERROR) // ���� Ȯ��
		ErrorHandling("Listen error");

	fd_set cpyReads, cpyReadsSensorNum, cpyClientNumWrites, cpyPauseWrites, reads;
	TIMEVAL timeout;

	FD_ZERO(&reads);
	FD_SET(hServSock, &reads);

	HANDLE hUIThread = (HANDLE)_beginthreadex(NULL, 0, ClientUI, &clientParam, 0, NULL);
	HANDLE hServiceThread = (HANDLE)_beginthreadex(NULL, 0, ClientService, &clientParam, 0, NULL);

	while (1) {
		cpyReads = reads;

		timeout.tv_sec = 5;
		timeout.tv_usec = 0;

		fdNum = select(0, &cpyReads, 0, 0, &timeout); // ��� Read Event Target

		if (fdNum == SOCKET_ERROR) { // ���� Ȯ��
			ErrorHandling("Select Read error\n");
		}
		else if (fdNum == 0) { // �ð� �Ҵ� ����
			// continue;
		}

		for (int i = 0; i < reads.fd_count; i++) {
			if (FD_ISSET(reads.fd_array[i], &cpyReads)) {
				if (reads.fd_array[i] == hServSock) { // ���� ���Ͽ� ���� ��û �̺�Ʈ �߻�
					adrSize = sizeof(clntAdr);
					hClntSock = accept(hServSock, (SOCKADDR*)&clntAdr, &adrSize);
					clientList[cntClient - 1].sHandle = hClntSock; // ���� ����� ������ Ŭ���̾�Ʈ ������ ���� ��ũ���� ����

					clientList[cntClient - 1].clientNum = cntClient; // Ŭ���̾�Ʈ ���� ��ȣ ����
					clientList[cntClient - 1].connection = TRUE;

					if (printLog == TRUE)
						printf("Connected Client: Port: %d, IP: %s\n\n",
							clntAdr.sin_port, inet_ntoa(clntAdr.sin_addr));

					FD_SET(hClntSock, &reads);

					while (1) { // ���� ������ ���Ͽ� �̺�Ʈ�� �߻����� �ʾҴٸ� �ݺ�
						/* 2 */
						/* Receive ���� �� ���� */
						cpyReadsSensorNum = reads;

						timeout.tv_sec = 5;
						timeout.tv_usec = 0;

						fdNum = select(0, &cpyReadsSensorNum, 0, 0, &timeout); // ���� �� ���� Read Event Target

						if (fdNum == SOCKET_ERROR) { // ���� Ȯ��
							ErrorHandling("Select Read error\n");
						}
						else if (fdNum == 0) { // �ð� �Ҵ� ����
							// continue;
						}

						if (FD_ISSET(reads.fd_array[reads.fd_count - 1], &cpyReadsSensorNum)) {  // ���� ��ũ���� ������ ���� ������ ��� �����Ͽ� ������ ���Ͽ� �̺�Ʈ �߻� ���� Ȯ��

							retVal = recv(reads.fd_array[reads.fd_count - 1], (char*)&clientList[cntClient - 1].cntSensor, sizeof(int), 0);  // ���� �� ����

							if (retVal == SOCKET_ERROR) { // ���� Ȯ��
								ErrorHandling("Recieve error");
							}

							clientList[cntClient - 1].sensor = malloc(sizeof(Sensor) * clientList[cntClient - 1].cntSensor); // ���� ����ü �迭 ���� ���� ���� ���� �Ҵ�

							retVal = recv(reads.fd_array[reads.fd_count - 1], initBuf, sizeof(double) * (2 * clientList[cntClient - 1].cntSensor), 0); // ���� ���� ����

							if (retVal == SOCKET_ERROR) { // ���� Ȯ��
								ErrorHandling("Recieve error");
							}

							for (int j = 0; j < clientList[cntClient - 1].cntSensor; j++) {
								clientList[cntClient - 1].sensor[j].pauseFlag = FALSE; // �Ͻ� �������� �ʵ��� ����
								clientList[cntClient - 1].sensor[j].sensorNum = j + 1; // ���� ��ȣ ����
								clientList[cntClient - 1].sensor[j].lowestLimit = *(double*)(initBuf + sizeof(double) * (j * 2)); // �ʱ�ȭ �迭�κ��� �� ������ ���� ���� ���� ����
								if (printLog == TRUE)
									printf("����%02d�� ���� ���� ����: %f\n", j + 1, clientList[cntClient - 1].sensor[j].lowestLimit);
								clientList[cntClient - 1].sensor[j].highestLimit = *(double*)(initBuf + sizeof(double) * (j * 2 + 1)); // �ʱ�ȭ �迭�κ��� �� ������ �ְ� ���� ���� ����
								if (printLog == TRUE)
									printf("����%02d�� �ְ� ���� ����: %f\n", j + 1, clientList[cntClient - 1].sensor[j].highestLimit);
							}
							if (printLog == TRUE)
								puts("");

							break;
						}
					}

					/* 3 */
					/* Send Ŭ���̾�Ʈ ���� ��ȣ �۽� */
					cpyClientNumWrites = reads;

					timeout.tv_sec = 5;
					timeout.tv_usec = 0;

					fdNum = select(0, 0, &cpyClientNumWrites, 0, &timeout); // Ŭ���̾�Ʈ ���� ��ȣ �۽� Write Event Target

					if (fdNum == SOCKET_ERROR) {
						ErrorHandling("Select Write error\n");
					}
					else if (fdNum == 0) {
						continue;
					}

					if (FD_ISSET(reads.fd_array[reads.fd_count - 1], &cpyClientNumWrites)) { // ���� ��ũ���� ������ ���� ������ ��� �����Ͽ� ������ ���Ͽ� �̺�Ʈ �߻� ���� Ȯ��

						// printf("Send to Client: Client Number\n\n");

						retVal = send(reads.fd_array[reads.fd_count - 1], &clientList[cntClient - 1].clientNum, sizeof(char), 0);  // Ŭ���̾�Ʈ ���� ��ȣ �۽�

						if (retVal == SOCKET_ERROR) {
							ErrorHandling("Send error\n");
						}
					}

					cntClient++;
				}
				else {  // ���� ����� ������ ���� �̺�Ʈ �߻�
					/* 6 */
					/* Receive �� ���� �� ���� */
					strLen = recv(reads.fd_array[i], sensorValBuf, BUF_SIZE, 0); // ���� ���� ���� �� ����

					for (int j = 0; j < cntClient; j++) {
						if (reads.fd_array[i] == clientList[j].sHandle) { // ���� �̺�Ʈ �߻��� ������ ���� ��ũ���Ϳ� Ŭ���̾�Ʈ ����Ʈ�� �� ���� ���� ��ũ���� ��
							currentClient = clientList[j]; // ���� �̺�Ʈ �߻��� ���Ͽ� ����� Ŭ���̾�Ʈ ȹ��
							currentClientNum = clientList[j].clientNum; // ���� �̺�Ʈ �߻��� ���Ͽ� ����� Ŭ���̾�Ʈ ���� ��ȣ ȹ��
						}
					}

					if (strLen <= 0) { // ���� Ȯ��
						for (int j = 0; j < cntClient; j++)
							if (reads.fd_array[i] == clientList[j].sHandle) // ���� �̺�Ʈ �߻��� ������ ���� ��ũ���Ϳ� Ŭ���̾�Ʈ ����Ʈ�� �� ���� ���� ��ũ���� ��
								clientList[j].connection = FALSE; // ���� ���� ����� ������ ���� ���� ���������� ����

						currentClient.sensor->pauseFlag = FALSE;

						free(currentClient.sensor);

						closesocket(reads.fd_array[i]); // ���� ���� ����

						if (printLog == TRUE)
							printf("Closed Client: %d, StrLen: %d\n", reads.fd_array[i], strLen);

						FD_CLR(reads.fd_array[i], &reads);
					}
					else {
						if (printLog == TRUE) {
							setColor(6, 0);
							printf("[Ŭ���̾�Ʈ %02d]\n\n", currentClientNum);
							setColor(15, 0);
						}

						cntPauseStartSensor = 0; // ������ ���� ��

						for (int j = 0; j < currentClient.cntSensor; j++) {
							sensorVal = *(double*)(sensorValBuf + j * sizeof(double));

							if (currentClient.sensor[j].lowestLimit <= sensorVal
								&& sensorVal <= currentClient.sensor[j].highestLimit) { // ���� ����
								currentClient.sensor[j].pauseFlag = FALSE;
								if (printLog == TRUE)
									printf("���� %02d: %f\n", j + 1, sensorVal);
							}
							else if (sensorVal == PAUSEVALUE) { // �̹� �Ͻ� ���ܵ� ����
								currentClient.sensor[j].pauseFlag = TRUE;
								if (printLog == TRUE)
									printf("���� %02d: �Ͻ� ����\n", j + 1);
							}
							else { // ���� ���� ��Ż
								currentClient.sensor[j].pauseFlag = TRUE; // �Ͻ� ����
								cntPauseStartSensor++;
								pauseBuf[cntPauseStartSensor] = currentClient.sensor[j].sensorNum; // ���� ���� ���� ���� ��ȣ ����
								if (printLog == TRUE) {
									printf("���� %02d: ", j + 1);
									setColor(4, 0);
									printf("�̻� ����\n");
									setColor(15, 0);
								}
							}
						}
						if (printLog == TRUE)
							puts("");

						pauseBuf[0] = cntPauseStartSensor;

						/* 7 */
						/* Send ���� ���� ���� ���� ���� ��ȣ �۽� */
						cpyPauseWrites = reads;

						timeout.tv_sec = 5;
						timeout.tv_usec = 0;

						fdNum = select(0, 0, &cpyPauseWrites, 0, &timeout); // ���� ���� ���� Write Event Target

						if (fdNum == SOCKET_ERROR) {
							ErrorHandling("Select Write error\n");
						}
						else if (fdNum == 0) {
							// continue;
						}

						if (FD_ISSET(reads.fd_array[i], &cpyPauseWrites)) { // ���� �̺�Ʈ �߻��� ���Ͽ� �̺�Ʈ �߻� ���� Ȯ��
							// printf("Send to Client: Sensor Number\n\n");

							retVal = send(reads.fd_array[i], &pauseBuf, sizeof(char) * cntPauseStartSensor + 1, 0); // ���� ���� ���� ���� ���� ���� ��ȣ�� ����� ���� ����

							if (retVal == SOCKET_ERROR) { // ���� Ȯ��
								ErrorHandling("Recieve error");
							}

							/* �α� Ȯ�� */
							if (printLog == TRUE)
								printf("�̻� ���� ���� ��: %d\n", cntPauseStartSensor);

							if (printLog == TRUE)
								puts("");

							for (int j = 0; j < cntPauseStartSensor; j++) {
								if (printLog == TRUE)
									printf("���� ��û ���� ��ȣ: %d\n", pauseBuf[j + 1]); // ���� ���� ���� ���� ��ȣ
							}
							if (printLog == TRUE)
								puts("");


							if (retVal == SOCKET_ERROR) { // ���� Ȯ��
								ErrorHandling("Send error\n");
							}
						}
					}
				}
			}
		}
	}

	closesocket(hServSock);
	WSACleanup();

	return 0;
}

void ErrorHandling(char* message) { // ���� �޽��� ��� �Լ�
	if (printUI == TRUE) {
		fputs(message, stderr);
		fputc('\n', stderr);
	}

	exit(1);
}

void alignCenter(int columns, char* text) { // �ؽ�Ʈ ��� ����
	int minWidth;
	minWidth = strlen(text) + (columns - strlen(text)) / 2;
	printf("%*s\n", minWidth, text); // �ּ� �ڸ� ����� ���ڿ� ä���
}


void gotoXY(int x, int y) { // UI ������ ���� Ŀ�� �̵� �Լ�
	COORD Pos;
	Pos.X = x;
	Pos.Y = y;
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), Pos);
}

void setColor(unsigned short text, unsigned short back) { // �ܼ� â �ؽ�Ʈ ���� ����
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), text | (back << 4));
}

void printBox() { // �α� �ڽ� UI ����
	if (printUI == TRUE) {
		printf("��");
		for (int i = 0; i < 100; i++) {
			printf("��");
		}
		printf("��");
		puts("");

		for (int i = 0; i < 20; i++) {
			printf("��");
			for (int j = 0; j < 100; j++) {
				printf(" ");
			}
			printf("��");
			puts("");
		}

		printf("��");
		for (int i = 0; i < 100; i++) {
			printf("��");
		}
		printf("��");
		puts("");

		for (int i = 0; i < 4; i++) {
			gotoXY(2 + i, 2);
			printf("��");
		}

		for (int i = 0; i < 25; i++) {
			gotoXY(9 + i, 2);
			printf("��");
		}

		for (int i = 0; i < 63; i++) {
			gotoXY(37 + i, 2);
			printf("��");
		}

		for (int i = 0; i < 20; i++) {
			gotoXY(7, i + 1);
			printf("��");
			gotoXY(35, i + 1);
			printf("��");
		}
	}
}

void printClientBox(int x, int y, int clientNum) { // Ŀ�� ��ǥ�� �������� Ŭ���̾�Ʈ �ڽ� UI ����
	if (printUI == TRUE) {
		gotoXY(x, y); // Ŀ�� �̵�
		/* ù ��° �� */
		printf("��");
		for (int i = 0; i < 5; i++)
			printf("��");
		printf("��\n");

		gotoXY(x, y + 1);
		/* �� ��° �� */
		printMiddleLine(clientNum);

		gotoXY(x, y + 2);
		/* �� ��° �� */
		printf("��");
		for (int i = 0; i < 5; i++)
			printf("��");
		printf("��\n");
	}
}

void printMiddleLine(int clientNum) { // Ŭ���̾�Ʈ �ڽ� ���ڿ� ��� UI ����
	if (printUI == TRUE)
		printf("��  %02d ��\n", clientNum);
}

void printGuide() { // ���̵� UI ����
	if (printUI == TRUE) {
		gotoXY(2, 1);
		printf(" Key ");

		gotoXY(2, 3);
		printf("  T  ");

		gotoXY(2, 5);
		printf("  F  ");

		gotoXY(2, 7);
		printf("  S  ");

		gotoXY(2, 9);
		printf("  R  ");

		gotoXY(2, 11);
		printf("  D  ");

		gotoXY(8, 1);
		alignCenter(28, "Command");

		gotoXY(8, 3);
		alignCenter(28, "����͸� UI");

		gotoXY(8, 5);
		alignCenter(28, "�α� �м�");

		gotoXY(8, 7);
		alignCenter(28, "���� ���� ���");

		gotoXY(8, 9);
		alignCenter(28, "���� ���� ���� ����");

		gotoXY(8, 11);
		alignCenter(28, "Ŭ���̾�Ʈ ���� ����");

		gotoXY(36, 1);
		alignCenter(65, "Client View");

		gotoXY(36, 14);
		setColor(3, 0);
		alignCenter(65, "�� -> ���� ���� Ŭ���̾�Ʈ");

		gotoXY(36, 16);
		setColor(4, 0);
		alignCenter(65, "�� -> �̻� ���� Ŭ���̾�Ʈ");

		gotoXY(36, 18);
		setColor(8, 0);
		alignCenter(65, "�� -> ���� ���� Ŭ���̾�Ʈ");

		setColor(7, 0);
	}
}


unsigned WINAPI ClientUI(void* clientParam) { // UI ���� ������
	Client* clientList = ((ClientParam*)clientParam)->clientList; // Ŭ���̾�Ʈ ���� ����Ʈ
	char* cntClient = ((ClientParam*)clientParam)->cntClient; // ���� Ŭ���̾�Ʈ�� �ο��� ���� ��ȣ�̰� �ϳ� ������ ���� �̿� �Ϸ�� Ŭ���̾�Ʈ ��ȣ

	char clientNum; // Ŭ���̾�Ʈ ���� ��ȣ
	char sensorNum; // ���� ���� ��ȣ

	double lowLimit; // ���� ���� ���� ����
	double highLimit; // �ְ� ���� ���� ����

	int baseX; // ���� X ��ǥ
	int baseY; // ���� Y ��ǥ

	printBox();

	printGuide();

	while (1) {
		baseX = 45; // Ŭ���̾�Ʈ �ڽ� UI ���� ��ǥ
		baseY = 5;

		for (int i = 0; i < 10; i++) { // Ŭ���̾�Ʈ �ڽ� UI �Ϻ� ����
			if (clientList[i].connection == TRUE) { // ���� ���¶�� ��� ����
				setColor(3, 0);
			}
			else { // ���� ���°� �ƴ϶�� �׷��� ����
				setColor(8, 0);
			}

			if (i < *cntClient) {
				for (int j = 0; j < clientList[i].cntSensor; j++) {
					if (clientList[i].sensor[j].pauseFlag == TRUE) { // �Ͻ� ���� ���¶�� ���� ����
						setColor(4, 0);
					}
				}
			}

			if (i % 5 == 0) {
				baseX = 45;
				baseY += 4 * (i / 5);
			}
			if (printUI == TRUE) {
				printClientBox(baseX, baseY, i + 1);
			}
			baseX += 10;
		}

		setColor(6, 0);
		Sleep(200); // 0.2��
	}

	return 0;
}

unsigned WINAPI ClientService(void* clientParam) { // Ŭ���̾�Ʈ ���� ���� ������
	Client* clientList = ((ClientParam*)clientParam)->clientList; // Ŭ���̾�Ʈ ���� ����Ʈ
	char* cntClient = ((ClientParam*)clientParam)->cntClient; // ���� Ŭ���̾�Ʈ�� �ο��� ���� ��ȣ�̰� �ϳ� ������ ���� �̿� �Ϸ�� Ŭ���̾�Ʈ ��ȣ

	char clientNum; // Ŭ���̾�Ʈ ���� ��ȣ
	char sensorNum; // ���� ���� ��ȣ

	double lowLimit; // ���� ���� ���� ����
	double highLimit; // �ְ� ���� ���� ����

	while (1) {
		char userKey = getch();

		switch (userKey) {
		case 'T':
		case 't':
			printLog = FALSE; // �α� �м� ����
			system("cls"); // �ܼ� ȭ�� �ʱ�ȭ
			printUI = TRUE; // ����͸� UI ����

			setColor(6, 0); // �ܼ� â �ؽ�Ʈ ���� ����
			printBox();
			printGuide();

			break;

		case 'F':
		case 'f':
			printUI = FALSE; // ����͸� UI ����
			system("cls");
			printLog = TRUE; // �α� �м� ����

			break;

		case 'S':
		case 's':
			printUI = FALSE;
			printLog = FALSE;
			system("cls");
			setColor(6, 0);
			printf("PRINT CLIENT INFO\n\n");

			printBox();

			for (int i = 0; i < *cntClient - 1; i++) {
				if (clientList[i].connection == TRUE) { // ������ ������� ���� Ŭ���̾�Ʈ ���� ���
					setColor(6, 0);
					printf("[Ŭ���̾�Ʈ %02d]\n\n", clientList[i].clientNum);

					setColor(15, 0);

					printf("��ġ�� ���� ��: %d\n\n", clientList[i].cntSensor);

					for (int j = 0; j < clientList[i].cntSensor; j++) {
						printf("����%02d�� ���� ���� ����: %f\n", j + 1, clientList[i].sensor[j].lowestLimit);
						printf("����%02d�� �ְ� ���� ����: %f\n\n", j + 1, clientList[i].sensor[j].highestLimit);
					}
				}
			}

			setColor(6, 0);
			printf("Press the T key");

			break;

		case 'R':
		case 'r':
			printUI = FALSE;
			printLog = FALSE;
			system("cls");
			setColor(6, 0);
			printf("SET CLIENT SAFETY LIMIT\n\n");

			printf("���� ������ �缳���� Ŭ���̾�Ʈ ��ȣ �Է�: ");
			scanf("%d", &clientNum);
			puts("");

			printf("��� ���� ��ȣ �Է�: ");
			scanf("%d", &sensorNum);
			puts("");

			for (int i = 0; i < *cntClient - 1; i++) {
				if (clientNum > *cntClient - 1) {
					printf("[Ŭ���̾�Ʈ %02d] �������� �ʴ� Ŭ���̾�Ʈ\n\n", clientNum);

					break;
				}


				if (clientList[i].clientNum == clientNum) { // Ŭ���̾�Ʈ ���� ��ȣ�� ������� ��� Ŭ���̾�Ʈ ����
					if (clientList[i].connection == FALSE) {
						printf("[Ŭ���̾�Ʈ %02d] ���� ���� ����\n\n", clientNum);

						break;
					}
				}

				if (clientList[i].clientNum == clientNum) { // Ŭ���̾�Ʈ ���� ��ȣ�� ������� ��� Ŭ���̾�Ʈ ����
					printf("����%02d�� ���� ���� ����: %f\n", sensorNum, clientList[i].sensor[sensorNum - 1].lowestLimit);
					printf("����%02d�� �ְ� ���� ����: %f\n\n", sensorNum, clientList[i].sensor[sensorNum - 1].highestLimit);

					printf("����%02d�� ���� ���� ���� �缳��: ", sensorNum);
					scanf("%lf", &lowLimit);
					printf("����%02d�� �ְ� ���� ���� �缳��: ", sensorNum);
					scanf("%lf", &highLimit);
					puts("");

					clientList[i].sensor[sensorNum - 1].lowestLimit = lowLimit;
					sensorNum, clientList[i].sensor[sensorNum - 1].highestLimit = highLimit;

					printf("����%02d�� �ְ� ���� ���� �缳�� �Ϸ�\n\n", sensorNum);
				}
			}

			printf("Press the T key");

			break;

		case 'D':
		case 'd':
			printUI = FALSE;
			printLog = FALSE;
			system("cls");
			printf("DISCONNECT CLIENT\n\n");

			printf("���� ������ Ŭ���̾�Ʈ ���� ��ȣ �Է�: ");
			scanf("%d", &clientNum);
			puts("");

			if (clientNum > *cntClient - 1)
				printf("[Ŭ���̾�Ʈ %02d] �������� �ʴ� Ŭ���̾�Ʈ\n\n", clientNum);

			for (int i = 0; i < *cntClient - 1; i++) {
				if (clientList[i].clientNum == clientNum) { // Ŭ���̾�Ʈ ���� ��ȣ�� ������� ��� Ŭ���̾�Ʈ ����
					if (clientList[i].connection == TRUE) {
						clientList[i].connection = FALSE;
						closesocket(clientList[i].sHandle);
						printf("[Ŭ���̾�Ʈ %02d] ���� ���� ����\n\n", clientNum);
					}
					else {
						printf("[Ŭ���̾�Ʈ %02d] ���� ���� ����\n\n", clientNum);
					}
				}
			}

			printf("Press the T key");

			break;
		}
	}
}
