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
	boolean	pauseFlag; // 센서 일시 중단 여부
	char	pauseSeconds; // 남은 중단 시간
	char	sensorNum; // 센서 가상 번호
} Sensor;

typedef struct ClientInfo {
	char	clientNum; // 클라이언트 가상 번호
	int		cntSensor; // 설치된 센서 수
	Sensor* sensor; // 각 센서 구조체 배열
} Client;

void setColor(unsigned short text, unsigned short back);

int main(void) {
	setColor(15, 0);

	WSADATA	wsaData;
	SOCKET	clientSocket;
	SOCKADDR_IN	serverAddr;

	int	retVal;
	int strLen;

	char	initBuf[BUF_SIZE]; // 센서 수와 초기 안전 범위를 저장하는 버퍼
	char	pauseBuf[BUF_SIZE]; // 차단된 센서 가상 번호를 저장하는 버퍼
	char	pauseCntSensor; // 일시 차단 요청 센서 수

	double	sensorVal; // 센서 값
	double* sensorValArr; // 주기적으로 갱신되는 각 센서 값 임시 저장

	Client client;

	srand(time(NULL)); // 랜덤 시드 초기화

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) // 오류 확인
		ErrorHandling("WSAStartup error");

	clientSocket = socket(AF_INET, SOCK_STREAM, 0); // 소켓 생성

	if (clientSocket == INVALID_SOCKET) { // 오류 확인
		ErrorHandling("Socket error");
	}

	ZeroMemory(&serverAddr, sizeof(serverAddr)); // 소켓 주소 구조체 초기화
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(9000);
	serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	printf("센서 수: ");
	scanf("%d", &client.cntSensor); // 클라이언트 센서 수 설정
	puts("");

	*(int*)initBuf = client.cntSensor; // 초기화 배열에 센서 수 저장

	for (int i = 0; i < client.cntSensor; i++) {
		printf("센서 %02d의 최저 안전 범위: ", i + 1);
		scanf("%lf", (double*)&initBuf[(sizeof(int) + sizeof(double) * (i * 2))]); // 초기화 배열에 최저 안전 범위 저장
		printf("센서 %02d의 최고 안전 범위: ", i + 1);
		scanf("%lf", (double*)&initBuf[(sizeof(int) + sizeof(double) * (i * 2 + 1))]); // 초기화 배열에 최고 안전 범위 저장
		puts("");
	}
	puts("");

	system("cls"); // 콘솔 초기화

	retVal = connect(clientSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)); // 서버 소켓에 연결 요청

	if (retVal == SOCKET_ERROR) { // 오류 확인
		ErrorHandling("Connect error");
	}

	client.sensor = malloc(sizeof(Sensor) * client.cntSensor); // 센서 구조체 배열 센서 수에 따라 동적 할당
	/* 1 */
	/* Send */
	retVal = send(clientSocket, initBuf, sizeof(int) + sizeof(double) * (2 * client.cntSensor), 0); // 센서 수와 안전 범위 서버에 송신

	if (retVal == SOCKET_ERROR) // 오류 확인
		ErrorHandling("Send error");


	sensorValArr = malloc(sizeof(double) * client.cntSensor); // 랜덤 센서 값 저장 배열 센서 수에 따라 동적 할당

	/* 4 */
	/* Receive */
	strLen = recv(clientSocket, &client.clientNum, sizeof(client.clientNum), 0); // 클라이언트 번호 서버로부터 수신

	if (strLen <= 0) // 오류 확인
		ErrorHandling("Receive error");


	for (int i = 0; i < client.cntSensor; i++) { // 차단 여부 초기 설정
		client.sensor[i].pauseFlag = FALSE;
	}

	while (1) { // 주기적으로 랜덤 센서 값 발생 후 서버로 전달
		setColor(6, 0);
		printf("[클라이언트 %02d]\n\n", client.clientNum);
		setColor(15, 0);

		for (int i = 0; i < client.cntSensor; i++) {
			client.sensor[i].pauseSeconds -= 2; // 

			if (client.sensor[i].pauseFlag == FALSE) { // 차단되지 않은 센서
				sensorValArr[i] = (double)rand() / RAND_MAX;
				printf("센서 %02d: %f\n", i + 1, sensorValArr[i]);
			}
			else {  // 차단된 센서
				sensorValArr[i] = (double)PAUSEVALUE;

				if (client.sensor[i].pauseSeconds == 0) { // 차단 시간 다 소모한 센서
					client.sensor[i].pauseFlag = FALSE;

					sensorValArr[i] = (double)rand() / RAND_MAX;
					printf("센서 %02d: %f\n", i + 1, sensorValArr[i]);
				}
				else { // 잔여 차단 시간 존재 센서
					printf("센서 %02d: 일시 차단으로 인한 관리자 모드 - 남은 대기 시간 %d초\n", i + 1, client.sensor[i].pauseSeconds);
				}
			}
		}
		puts("");

		/* 5 */
		/* Send */
		retVal = send(clientSocket, (char*)sensorValArr, sizeof(double) * client.cntSensor, 0); // 랜덤 센서 값 배열 서버에 송신

		if (retVal == SOCKET_ERROR) // 오류 확인
			ErrorHandling("Send error");

		/* 8 */
		/* Receive */
		strLen = recv(clientSocket, &pauseBuf, sizeof(char), 0); // 일시 차단 요청 센서 수 서버로부터 수신

		if (strLen <= 0) // 오류 확인
			ErrorHandling("Receive error");

		pauseCntSensor = pauseBuf[0];

		if (pauseCntSensor > 0) { // 차단 요청 존재
			/* 9 */
			/* Receive */
			strLen = recv(clientSocket, &pauseBuf[1], sizeof(char) * pauseCntSensor, 0); // 일시 차단 요청 센서 가상 번호 서버로부터 수신

			if (strLen <= 0) // 오류 확인
				ErrorHandling("Receive error");

			for (int i = 0; i < pauseCntSensor; i++) { // 차단 요청 수만큼 반복하고 대상 센서 차단
				client.sensor[pauseBuf[i + 1] - 1].pauseFlag = TRUE;
				client.sensor[pauseBuf[i + 1] - 1].pauseSeconds = 10;

				setColor(4, 0);
				printf("센서 %02d 이상 감지\n", pauseBuf[i + 1]);
				setColor(15, 0);
			}
			puts("");
		}

		Sleep(2000); // 2초
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

void setColor(unsigned short text, unsigned short back) { // 콘솔 창 텍스트 색상 설정
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), text | (back << 4));
}

void alignCenter(int columns, char* text) { // 텍스트 가운데 정렬
	int minWidth;
	minWidth = strlen(text) + (columns - strlen(text)) / 2;
	printf("%*s\n", minWidth, text);
}