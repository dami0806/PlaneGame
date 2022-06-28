#define _CRT_SECURE_NO_WARNINGS
#include <winsock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <time.h>

#define	BUF_SIZE 1024

#define FALSE 0
#define TRUE 1

#define PAUSEVALUE 100;

void ErrorHandling(char* message);

typedef struct ClientSensor {
	boolean	pauseFlag; // ���� �Ͻ� �ߴ� ����
	char	pauseSeconds; // ���� �ߴ� �ð�
	char	sensorNum; // ���� ���� ��ȣ
} Sensor;

typedef struct ClientInfo {
	char	clientNum; // Ŭ���̾�Ʈ ���� ��ȣ
	int		cntSensor; // ��ġ�� ���� ��
	Sensor* sensor; // �� ���� ����ü �迭
} Client;

void setColor(unsigned short text, unsigned short back);

int main(void) {
	setColor(15, 0);

	WSADATA	wsaData;
	SOCKET	clientSocket;
	SOCKADDR_IN	serverAddr;

	int	retVal;
	int strLen;

	char	initBuf[BUF_SIZE]; // ���� ���� �ʱ� ���� ������ �����ϴ� ����
	char	pauseBuf[BUF_SIZE]; // ���ܵ� ���� ���� ��ȣ�� �����ϴ� ����
	char	pauseCntSensor; // �Ͻ� ���� ��û ���� ��

	double	sensorVal; // ���� ��
	double* sensorValArr; // �ֱ������� ���ŵǴ� �� ���� �� �ӽ� ����

	Client client;

	srand(time(NULL)); // ���� �õ� �ʱ�ȭ

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) // ���� Ȯ��
		ErrorHandling("WSAStartup error");

	clientSocket = socket(AF_INET, SOCK_STREAM, 0); // ���� ����

	if (clientSocket == INVALID_SOCKET) { // ���� Ȯ��
		ErrorHandling("Socket error");
	}

	ZeroMemory(&serverAddr, sizeof(serverAddr)); // ���� �ּ� ����ü �ʱ�ȭ
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(9000);
	serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	printf("���� ��: ");
	scanf("%d", &client.cntSensor); // Ŭ���̾�Ʈ ���� �� ����
	puts("");

	*(int*)initBuf = client.cntSensor; // �ʱ�ȭ �迭�� ���� �� ����

	for (int i = 0; i < client.cntSensor; i++) {
		printf("���� %02d�� ���� ���� ����: ", i + 1);
		scanf("%lf", (double*)&initBuf[(sizeof(int) + sizeof(double) * (i * 2))]); // �ʱ�ȭ �迭�� ���� ���� ���� ����
		printf("���� %02d�� �ְ� ���� ����: ", i + 1);
		scanf("%lf", (double*)&initBuf[(sizeof(int) + sizeof(double) * (i * 2 + 1))]); // �ʱ�ȭ �迭�� �ְ� ���� ���� ����
		puts("");
	}
	puts("");

	system("cls"); // �ܼ� �ʱ�ȭ

	retVal = connect(clientSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)); // ���� ���Ͽ� ���� ��û

	if (retVal == SOCKET_ERROR) { // ���� Ȯ��
		ErrorHandling("Connect error");
	}

	client.sensor = malloc(sizeof(Sensor) * client.cntSensor); // ���� ����ü �迭 ���� ���� ���� ���� �Ҵ�
	/* 1 */
	/* Send */
	retVal = send(clientSocket, initBuf, sizeof(int) + sizeof(double) * (2 * client.cntSensor), 0); // ���� ���� ���� ���� ������ �۽�

	if (retVal == SOCKET_ERROR) // ���� Ȯ��
		ErrorHandling("Send error");


	sensorValArr = malloc(sizeof(double) * client.cntSensor); // ���� ���� �� ���� �迭 ���� ���� ���� ���� �Ҵ�

	/* 4 */
	/* Receive */
	strLen = recv(clientSocket, &client.clientNum, sizeof(client.clientNum), 0); // Ŭ���̾�Ʈ ��ȣ �����κ��� ����

	if (strLen <= 0) // ���� Ȯ��
		ErrorHandling("Receive error");


	for (int i = 0; i < client.cntSensor; i++) { // ���� ���� �ʱ� ����
		client.sensor[i].pauseFlag = FALSE;
	}

	while (1) { // �ֱ������� ���� ���� �� �߻� �� ������ ����
		setColor(6, 0);
		printf("[Ŭ���̾�Ʈ %02d]\n\n", client.clientNum);
		setColor(15, 0);

		for (int i = 0; i < client.cntSensor; i++) {
			client.sensor[i].pauseSeconds -= 2; // 

			if (client.sensor[i].pauseFlag == FALSE) { // ���ܵ��� ���� ����
				sensorValArr[i] = (double)rand() / RAND_MAX;
				printf("���� %02d: %f\n", i + 1, sensorValArr[i]);
			}
			else {  // ���ܵ� ����
				sensorValArr[i] = (double)PAUSEVALUE;

				if (client.sensor[i].pauseSeconds == 0) { // ���� �ð� �� �Ҹ��� ����
					client.sensor[i].pauseFlag = FALSE;

					sensorValArr[i] = (double)rand() / RAND_MAX;
					printf("���� %02d: %f\n", i + 1, sensorValArr[i]);
				}
				else { // �ܿ� ���� �ð� ���� ����
					printf("���� %02d: �Ͻ� �������� ���� ������ ��� - ���� ��� �ð� %d��\n", i + 1, client.sensor[i].pauseSeconds);
				}
			}
		}
		puts("");

		/* 5 */
		/* Send */
		retVal = send(clientSocket, (char*)sensorValArr, sizeof(double) * client.cntSensor, 0); // ���� ���� �� �迭 ������ �۽�

		if (retVal == SOCKET_ERROR) // ���� Ȯ��
			ErrorHandling("Send error");

		/* 8 */
		/* Receive */
		strLen = recv(clientSocket, &pauseBuf, sizeof(char), 0); // �Ͻ� ���� ��û ���� �� �����κ��� ����

		if (strLen <= 0) // ���� Ȯ��
			ErrorHandling("Receive error");

		pauseCntSensor = pauseBuf[0];

		if (pauseCntSensor > 0) { // ���� ��û ����
			/* 9 */
			/* Receive */
			strLen = recv(clientSocket, &pauseBuf[1], sizeof(char) * pauseCntSensor, 0); // �Ͻ� ���� ��û ���� ���� ��ȣ �����κ��� ����

			if (strLen <= 0) // ���� Ȯ��
				ErrorHandling("Receive error");

			for (int i = 0; i < pauseCntSensor; i++) { // ���� ��û ����ŭ �ݺ��ϰ� ��� ���� ����
				client.sensor[pauseBuf[i + 1] - 1].pauseFlag = TRUE;
				client.sensor[pauseBuf[i + 1] - 1].pauseSeconds = 10;

				setColor(4, 0);
				printf("���� %02d �̻� ����\n", pauseBuf[i + 1]);
				setColor(15, 0);
			}
			puts("");
		}

		Sleep(2000); // 2��
	}

	closesocket(clientSocket);
	WSACleanup();

	free(client.sensor);
	free(sensorValArr);

	return 0;
}

void ErrorHandling(char* message)
{
	fputs(message, stderr);
	fputc('\n', stderr);

	exit(1);
}

void setColor(unsigned short text, unsigned short back) { // �ܼ� â �ؽ�Ʈ ���� ����
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), text | (back << 4));
}

void alignCenter(int columns, char* text) { // �ؽ�Ʈ ��� ����
	int minWidth;
	minWidth = strlen(text) + (columns - strlen(text)) / 2;
	printf("%*s\n", minWidth, text);
}