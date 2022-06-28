
/*
====================================================================================================
Name        : Monitoring
Author      : Kim Hwi So
Version     : v1.0
Description : Network Programming Project
			   서버와 클라이언트 코드의 송수신 코드(Send, Receive)에 명시된 1 - 9번 주석 순서에 따라 동작
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
#define MAX_CLIENTLIST_SIZE 100 // 최대 생성 가능한 클라이언트 수
#define FIRST_CLIENT_NUMBER 1 // 시작 클라이언트 가상 번호

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
	boolean pauseFlag; // 센서 일시 중단 여부
	char	sensorNum; // 센서 가상 번호
	double	lowestLimit; // 각 센서의 최저 안전 범위
	double	highestLimit; // 각 센서의 최고 안전 범위
} Sensor;

typedef struct ClientInfo {
	boolean connection; // 연결 여부
	char	clientNum; // 클라이언트 번호
	int		cntSensor; // 설치된 센서 수
	Sensor* sensor; // 각 센서 구조체 배열
	SOCKET	sHandle; // 소켓 파일 디스크립터
} Client;

typedef struct ClientParam { // 스레드에게 인자 전달 구조체
	Client* clientList; // 클라이언트 리스트
	char* cntClient; // 다음 클라이언트에 부여할 가상 번호이고 하나 작으면 현재 이용 완료된 클라이언트 번호
} ClientParam;

boolean			printLog = FALSE; // 로그 화면 출력 여부
boolean			printUI = TRUE; // 모니터링 UI 출력 여부

int main(void) {
	WSADATA			wsaData;
	SOCKET			hServSock, hClntSock;
	SOCKADDR_IN		servAdr, clntAdr, clientAddr;

	int				adrSize;
	int				fdNum, retVal, strLen;
	double			sensorVal; // 센서 값

	char            initBuf[BUF_SIZE]; // 센서 수와 초기 안전 범위를 저장 버퍼
	char			pauseBuf[BUF_SIZE]; // 차단된 센서 가상 번호를 저장 버퍼
	char			sensorValBuf[BUF_SIZE]; // 센서 수와 센서 값 저장 버퍼
	char			cntPauseStartSensor; // 일시 차단 요청 센서 수
	char			pauseStartCntSensor; // 차단 시작 센서 수

	int				cntClient = FIRST_CLIENT_NUMBER; // 최근 등록된 클라이언트 가상 번호이고 이용 완료된 마지막 클라이언트 가상 번호
	Client			clientList[MAX_CLIENTLIST_SIZE]; // 각 클라이언트 정보가 담긴 구조체 배열

	Client currentClient; // 현재 통신중인 클라이언트
	int currentClientNum; // 현재 통신중인 클라이언트 번호

	ClientParam clientParam;

	clientParam.clientList = clientList;
	clientParam.cntClient = &cntClient;


	// system("mode con:cols=102 lines=22"); // 콘솔 창 크기 지정
	setColor(6, 0); // 콘솔 창 텍스트 색상 지정

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) // 오류 확인
		ErrorHandling("WSAStartup error");

	hServSock = socket(PF_INET, SOCK_STREAM, 0); // 소켓 주소 구조체 초기화
	memset(&servAdr, 0, sizeof(servAdr));
	servAdr.sin_family = AF_INET;
	servAdr.sin_addr.s_addr = inet_addr("127.0.0.1");
	servAdr.sin_port = htons(9000);

	if (bind(hServSock, (SOCKADDR*)&servAdr, sizeof(servAdr)) == SOCKET_ERROR) // 오류 확인
		ErrorHandling("Bind error");

	if (listen(hServSock, 5) == SOCKET_ERROR) // 오류 확인
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

		fdNum = select(0, &cpyReads, 0, 0, &timeout); // 모든 Read Event Target

		if (fdNum == SOCKET_ERROR) { // 오류 확인
			ErrorHandling("Select Read error\n");
		}
		else if (fdNum == 0) { // 시간 할당 종료
			// continue;
		}

		for (int i = 0; i < reads.fd_count; i++) {
			if (FD_ISSET(reads.fd_array[i], &cpyReads)) {
				if (reads.fd_array[i] == hServSock) { // 서버 소켓에 연결 요청 이벤트 발생
					adrSize = sizeof(clntAdr);
					hClntSock = accept(hServSock, (SOCKADDR*)&clntAdr, &adrSize);
					clientList[cntClient - 1].sHandle = hClntSock; // 연결 결과로 생성될 클라이언트 정보에 파일 디스크립터 저장

					clientList[cntClient - 1].clientNum = cntClient; // 클라이언트 가상 번호 저장
					clientList[cntClient - 1].connection = TRUE;

					if (printLog == TRUE)
						printf("Connected Client: Port: %d, IP: %s\n\n",
							clntAdr.sin_port, inet_ntoa(clntAdr.sin_addr));

					FD_SET(hClntSock, &reads);

					while (1) { // 직전 생성된 소켓에 이벤트가 발생하지 않았다면 반복
						/* 2 */
						/* Receive 센서 수 수신 */
						cpyReadsSensorNum = reads;

						timeout.tv_sec = 5;
						timeout.tv_usec = 0;

						fdNum = select(0, &cpyReadsSensorNum, 0, 0, &timeout); // 센서 수 수신 Read Event Target

						if (fdNum == SOCKET_ERROR) { // 오류 확인
							ErrorHandling("Select Read error\n");
						}
						else if (fdNum == 0) { // 시간 할당 종료
							// continue;
						}

						if (FD_ISSET(reads.fd_array[reads.fd_count - 1], &cpyReadsSensorNum)) {  // 파일 디스크립터 집합의 가장 마지막 요소 선택하여 생성된 소켓에 이벤트 발생 여부 확인

							retVal = recv(reads.fd_array[reads.fd_count - 1], (char*)&clientList[cntClient - 1].cntSensor, sizeof(int), 0);  // 센서 수 수신

							if (retVal == SOCKET_ERROR) { // 오류 확인
								ErrorHandling("Recieve error");
							}

							clientList[cntClient - 1].sensor = malloc(sizeof(Sensor) * clientList[cntClient - 1].cntSensor); // 센서 구조체 배열 센서 수에 따라 동적 할당

							retVal = recv(reads.fd_array[reads.fd_count - 1], initBuf, sizeof(double) * (2 * clientList[cntClient - 1].cntSensor), 0); // 안전 범위 수신

							if (retVal == SOCKET_ERROR) { // 오류 확인
								ErrorHandling("Recieve error");
							}

							for (int j = 0; j < clientList[cntClient - 1].cntSensor; j++) {
								clientList[cntClient - 1].sensor[j].pauseFlag = FALSE; // 일시 차단하지 않도록 설정
								clientList[cntClient - 1].sensor[j].sensorNum = j + 1; // 센서 번호 설정
								clientList[cntClient - 1].sensor[j].lowestLimit = *(double*)(initBuf + sizeof(double) * (j * 2)); // 초기화 배열로부터 각 센서에 최저 안전 범위 저장
								if (printLog == TRUE)
									printf("센서%02d의 최저 안전 범위: %f\n", j + 1, clientList[cntClient - 1].sensor[j].lowestLimit);
								clientList[cntClient - 1].sensor[j].highestLimit = *(double*)(initBuf + sizeof(double) * (j * 2 + 1)); // 초기화 배열로부터 각 센서에 최고 안전 범위 저장
								if (printLog == TRUE)
									printf("센서%02d의 최고 안전 범위: %f\n", j + 1, clientList[cntClient - 1].sensor[j].highestLimit);
							}
							if (printLog == TRUE)
								puts("");

							break;
						}
					}

					/* 3 */
					/* Send 클라이언트 가상 번호 송신 */
					cpyClientNumWrites = reads;

					timeout.tv_sec = 5;
					timeout.tv_usec = 0;

					fdNum = select(0, 0, &cpyClientNumWrites, 0, &timeout); // 클라이언트 가상 번호 송신 Write Event Target

					if (fdNum == SOCKET_ERROR) {
						ErrorHandling("Select Write error\n");
					}
					else if (fdNum == 0) {
						continue;
					}

					if (FD_ISSET(reads.fd_array[reads.fd_count - 1], &cpyClientNumWrites)) { // 파일 디스크립터 집합의 가장 마지막 요소 선택하여 생성된 소켓에 이벤트 발생 여부 확인

						// printf("Send to Client: Client Number\n\n");

						retVal = send(reads.fd_array[reads.fd_count - 1], &clientList[cntClient - 1].clientNum, sizeof(char), 0);  // 클라이언트 가상 번호 송신

						if (retVal == SOCKET_ERROR) {
							ErrorHandling("Send error\n");
						}
					}

					cntClient++;
				}
				else {  // 연결 결과로 생성된 소켓 이벤트 발생
					/* 6 */
					/* Receive 각 센서 값 수신 */
					strLen = recv(reads.fd_array[i], sensorValBuf, BUF_SIZE, 0); // 센서 수와 센서 값 수신

					for (int j = 0; j < cntClient; j++) {
						if (reads.fd_array[i] == clientList[j].sHandle) { // 현재 이벤트 발생된 소켓의 파일 디스크립터와 클라이언트 리스트의 각 소켓 파일 디스크립터 비교
							currentClient = clientList[j]; // 현재 이벤트 발생된 소켓에 연결된 클라이언트 획득
							currentClientNum = clientList[j].clientNum; // 현재 이벤트 발생된 소켓에 연결된 클라이언트 가상 번호 획득
						}
					}

					if (strLen <= 0) { // 오류 확인
						for (int j = 0; j < cntClient; j++)
							if (reads.fd_array[i] == clientList[j].sHandle) // 현재 이벤트 발생된 소켓의 파일 디스크립터와 클라이언트 리스트의 각 소켓 파일 디스크립터 비교
								clientList[j].connection = FALSE; // 현재 연결 종료될 소켓의 연결 상태 비정상으로 변경

						currentClient.sensor->pauseFlag = FALSE;

						free(currentClient.sensor);

						closesocket(reads.fd_array[i]); // 소켓 연결 종료

						if (printLog == TRUE)
							printf("Closed Client: %d, StrLen: %d\n", reads.fd_array[i], strLen);

						FD_CLR(reads.fd_array[i], &reads);
					}
					else {
						if (printLog == TRUE) {
							setColor(6, 0);
							printf("[클라이언트 %02d]\n\n", currentClientNum);
							setColor(15, 0);
						}

						cntPauseStartSensor = 0; // 차단할 센서 수

						for (int j = 0; j < currentClient.cntSensor; j++) {
							sensorVal = *(double*)(sensorValBuf + j * sizeof(double));

							if (currentClient.sensor[j].lowestLimit <= sensorVal
								&& sensorVal <= currentClient.sensor[j].highestLimit) { // 안전 범위
								currentClient.sensor[j].pauseFlag = FALSE;
								if (printLog == TRUE)
									printf("센서 %02d: %f\n", j + 1, sensorVal);
							}
							else if (sensorVal == PAUSEVALUE) { // 이미 일시 차단된 상태
								currentClient.sensor[j].pauseFlag = TRUE;
								if (printLog == TRUE)
									printf("센서 %02d: 일시 차단\n", j + 1);
							}
							else { // 안전 범위 이탈
								currentClient.sensor[j].pauseFlag = TRUE; // 일시 차단
								cntPauseStartSensor++;
								pauseBuf[cntPauseStartSensor] = currentClient.sensor[j].sensorNum; // 차단 시작 센서 가상 번호 저장
								if (printLog == TRUE) {
									printf("센서 %02d: ", j + 1);
									setColor(4, 0);
									printf("이상 감지\n");
									setColor(15, 0);
								}
							}
						}
						if (printLog == TRUE)
							puts("");

						pauseBuf[0] = cntPauseStartSensor;

						/* 7 */
						/* Send 차단 시작 센서 수와 가상 번호 송신 */
						cpyPauseWrites = reads;

						timeout.tv_sec = 5;
						timeout.tv_usec = 0;

						fdNum = select(0, 0, &cpyPauseWrites, 0, &timeout); // 차단 시작 센서 Write Event Target

						if (fdNum == SOCKET_ERROR) {
							ErrorHandling("Select Write error\n");
						}
						else if (fdNum == 0) {
							// continue;
						}

						if (FD_ISSET(reads.fd_array[i], &cpyPauseWrites)) { // 현재 이벤트 발생된 소켓에 이벤트 발생 여부 확인
							// printf("Send to Client: Sensor Number\n\n");

							retVal = send(reads.fd_array[i], &pauseBuf, sizeof(char) * cntPauseStartSensor + 1, 0); // 차단 시작 센서 수와 센서 가상 번호가 저장된 버퍼 전송

							if (retVal == SOCKET_ERROR) { // 오류 확인
								ErrorHandling("Recieve error");
							}

							/* 로그 확인 */
							if (printLog == TRUE)
								printf("이상 감지 센서 수: %d\n", cntPauseStartSensor);

							if (printLog == TRUE)
								puts("");

							for (int j = 0; j < cntPauseStartSensor; j++) {
								if (printLog == TRUE)
									printf("차단 요청 센서 번호: %d\n", pauseBuf[j + 1]); // 차단 시작 센서 가상 번호
							}
							if (printLog == TRUE)
								puts("");


							if (retVal == SOCKET_ERROR) { // 오류 확인
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

void ErrorHandling(char* message) { // 에러 메시지 출력 함수
	if (printUI == TRUE) {
		fputs(message, stderr);
		fputc('\n', stderr);
	}

	exit(1);
}

void alignCenter(int columns, char* text) { // 텍스트 가운데 정렬
	int minWidth;
	minWidth = strlen(text) + (columns - strlen(text)) / 2;
	printf("%*s\n", minWidth, text); // 최소 자리 만들고 문자열 채우기
}


void gotoXY(int x, int y) { // UI 구성을 위한 커서 이동 함수
	COORD Pos;
	Pos.X = x;
	Pos.Y = y;
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), Pos);
}

void setColor(unsigned short text, unsigned short back) { // 콘솔 창 텍스트 색상 설정
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), text | (back << 4));
}

void printBox() { // 로그 박스 UI 구현
	if (printUI == TRUE) {
		printf("┏");
		for (int i = 0; i < 100; i++) {
			printf("━");
		}
		printf("┓");
		puts("");

		for (int i = 0; i < 20; i++) {
			printf("┃");
			for (int j = 0; j < 100; j++) {
				printf(" ");
			}
			printf("┃");
			puts("");
		}

		printf("┗");
		for (int i = 0; i < 100; i++) {
			printf("━");
		}
		printf("┛");
		puts("");

		for (int i = 0; i < 4; i++) {
			gotoXY(2 + i, 2);
			printf("━");
		}

		for (int i = 0; i < 25; i++) {
			gotoXY(9 + i, 2);
			printf("━");
		}

		for (int i = 0; i < 63; i++) {
			gotoXY(37 + i, 2);
			printf("━");
		}

		for (int i = 0; i < 20; i++) {
			gotoXY(7, i + 1);
			printf("┃");
			gotoXY(35, i + 1);
			printf("┃");
		}
	}
}

void printClientBox(int x, int y, int clientNum) { // 커서 좌표를 기준으로 클라이언트 박스 UI 구현
	if (printUI == TRUE) {
		gotoXY(x, y); // 커서 이동
		/* 첫 번째 줄 */
		printf("┌");
		for (int i = 0; i < 5; i++)
			printf("─");
		printf("┐\n");

		gotoXY(x, y + 1);
		/* 두 번째 줄 */
		printMiddleLine(clientNum);

		gotoXY(x, y + 2);
		/* 세 번째 줄 */
		printf("└");
		for (int i = 0; i < 5; i++)
			printf("─");
		printf("┘\n");
	}
}

void printMiddleLine(int clientNum) { // 클라이언트 박스 문자열 출력 UI 구현
	if (printUI == TRUE)
		printf("│  %02d │\n", clientNum);
}

void printGuide() { // 가이드 UI 구현
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
		alignCenter(28, "모니터링 UI");

		gotoXY(8, 5);
		alignCenter(28, "로그 분석");

		gotoXY(8, 7);
		alignCenter(28, "센서 정보 출력");

		gotoXY(8, 9);
		alignCenter(28, "센서 안전 범위 설정");

		gotoXY(8, 11);
		alignCenter(28, "클라이언트 연결 종료");

		gotoXY(36, 1);
		alignCenter(65, "Client View");

		gotoXY(36, 14);
		setColor(3, 0);
		alignCenter(65, "■ -> 정상 연결 클라이언트");

		gotoXY(36, 16);
		setColor(4, 0);
		alignCenter(65, "■ -> 이상 감지 클라이언트");

		gotoXY(36, 18);
		setColor(8, 0);
		alignCenter(65, "■ -> 연결 해제 클라이언트");

		setColor(7, 0);
	}
}


unsigned WINAPI ClientUI(void* clientParam) { // UI 구현 스레드
	Client* clientList = ((ClientParam*)clientParam)->clientList; // 클라이언트 정보 리스트
	char* cntClient = ((ClientParam*)clientParam)->cntClient; // 다음 클라이언트에 부여할 가상 번호이고 하나 작으면 현재 이용 완료된 클라이언트 번호

	char clientNum; // 클라이언트 가상 번호
	char sensorNum; // 센서 가상 번호

	double lowLimit; // 최저 안전 범위 저장
	double highLimit; // 최고 안전 범위 저장

	int baseX; // 기준 X 좌표
	int baseY; // 기준 Y 좌표

	printBox();

	printGuide();

	while (1) {
		baseX = 45; // 클라이언트 박스 UI 기준 좌표
		baseY = 5;

		for (int i = 0; i < 10; i++) { // 클라이언트 박스 UI 일부 구현
			if (clientList[i].connection == TRUE) { // 연결 상태라면 블루 색상
				setColor(3, 0);
			}
			else { // 연결 상태가 아니라면 그레이 색상
				setColor(8, 0);
			}

			if (i < *cntClient) {
				for (int j = 0; j < clientList[i].cntSensor; j++) {
					if (clientList[i].sensor[j].pauseFlag == TRUE) { // 일시 차단 상태라면 레드 색상
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
		Sleep(200); // 0.2초
	}

	return 0;
}

unsigned WINAPI ClientService(void* clientParam) { // 클라이언트 서비스 구현 스레드
	Client* clientList = ((ClientParam*)clientParam)->clientList; // 클라이언트 정보 리스트
	char* cntClient = ((ClientParam*)clientParam)->cntClient; // 다음 클라이언트에 부여할 가상 번호이고 하나 작으면 현재 이용 완료된 클라이언트 번호

	char clientNum; // 클라이언트 가상 번호
	char sensorNum; // 센서 가상 번호

	double lowLimit; // 최저 안전 범위 저장
	double highLimit; // 최고 안전 범위 저장

	while (1) {
		char userKey = getch();

		switch (userKey) {
		case 'T':
		case 't':
			printLog = FALSE; // 로그 분석 중지
			system("cls"); // 콘솔 화면 초기화
			printUI = TRUE; // 모니터링 UI 시작

			setColor(6, 0); // 콘솔 창 텍스트 색상 지정
			printBox();
			printGuide();

			break;

		case 'F':
		case 'f':
			printUI = FALSE; // 모니터링 UI 중지
			system("cls");
			printLog = TRUE; // 로그 분석 시작

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
				if (clientList[i].connection == TRUE) { // 연결이 종료되지 않은 클라이언트 정보 출력
					setColor(6, 0);
					printf("[클라이언트 %02d]\n\n", clientList[i].clientNum);

					setColor(15, 0);

					printf("설치된 센서 수: %d\n\n", clientList[i].cntSensor);

					for (int j = 0; j < clientList[i].cntSensor; j++) {
						printf("센서%02d의 최저 안전 범위: %f\n", j + 1, clientList[i].sensor[j].lowestLimit);
						printf("센서%02d의 최고 안전 범위: %f\n\n", j + 1, clientList[i].sensor[j].highestLimit);
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

			printf("안전 범위를 재설정할 클라이언트 번호 입력: ");
			scanf("%d", &clientNum);
			puts("");

			printf("대상 센서 번호 입력: ");
			scanf("%d", &sensorNum);
			puts("");

			for (int i = 0; i < *cntClient - 1; i++) {
				if (clientNum > *cntClient - 1) {
					printf("[클라이언트 %02d] 존재하지 않는 클라이언트\n\n", clientNum);

					break;
				}


				if (clientList[i].clientNum == clientNum) { // 클라이언트 가상 번호를 기반으로 대상 클라이언트 선택
					if (clientList[i].connection == FALSE) {
						printf("[클라이언트 %02d] 연결 해제 상태\n\n", clientNum);

						break;
					}
				}

				if (clientList[i].clientNum == clientNum) { // 클라이언트 가상 번호를 기반으로 대상 클라이언트 선택
					printf("센서%02d의 최저 안전 범위: %f\n", sensorNum, clientList[i].sensor[sensorNum - 1].lowestLimit);
					printf("센서%02d의 최고 안전 범위: %f\n\n", sensorNum, clientList[i].sensor[sensorNum - 1].highestLimit);

					printf("센서%02d의 최저 안전 범위 재설정: ", sensorNum);
					scanf("%lf", &lowLimit);
					printf("센서%02d의 최고 안전 범위 재설정: ", sensorNum);
					scanf("%lf", &highLimit);
					puts("");

					clientList[i].sensor[sensorNum - 1].lowestLimit = lowLimit;
					sensorNum, clientList[i].sensor[sensorNum - 1].highestLimit = highLimit;

					printf("센서%02d의 최고 안전 범위 재설정 완료\n\n", sensorNum);
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

			printf("연결 종료할 클라이언트 가상 번호 입력: ");
			scanf("%d", &clientNum);
			puts("");

			if (clientNum > *cntClient - 1)
				printf("[클라이언트 %02d] 존재하지 않는 클라이언트\n\n", clientNum);

			for (int i = 0; i < *cntClient - 1; i++) {
				if (clientList[i].clientNum == clientNum) { // 클라이언트 가상 번호를 기반으로 대상 클라이언트 선택
					if (clientList[i].connection == TRUE) {
						clientList[i].connection = FALSE;
						closesocket(clientList[i].sHandle);
						printf("[클라이언트 %02d] 연결 정상 종료\n\n", clientNum);
					}
					else {
						printf("[클라이언트 %02d] 연결 해제 상태\n\n", clientNum);
					}
				}
			}

			printf("Press the T key");

			break;
		}
	}
}
