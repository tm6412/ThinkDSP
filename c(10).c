#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <signal.h>

#define PORTNUM 9006
#define BUF_SIZE 1024
#define MSG_SIZE 1024

//지뢰판을 출력하는 함수
void printMine(char* mine);
//랜덤으로 설정된 지뢰가 존재하는 지뢰판을 생성하는 함수
char* creatMine(char* game, int sd, int num);
//사용자로부터 숫자를 입력 받는 함수
int enterClientNum(int sd, int arr[]);
//지뢰 게임 한 ROUND를 실행하는 함수
void playMine(char* arrMine, char* game, int sd, int num, int arr[], int c, int s);
//입력한 번호가 중복됐는지 확인 후 저장하는 함수
int saveNum(int arr[], int num);
//게임 결과를 저장하는 함수
void result(int c, int s);
//서버, 클라이언트 점수를 저장하는 정수
int c, s;

void handler(int signo) {
	psignal(signo, "\n비정상종료");
	exit(0);
}

int main(void) {
	char buf[BUF_SIZE];
	char getM[10];
	char game[25];
	char* arrMine;
	struct sockaddr_in sin;
	int sd;

	//인터럽트 시그널을 받았을 경우 시그널 핸들러 호출
	void (*hand)(int);
	hand = signal(SIGINT, handler);
	if (hand == SIG_ERR) {
		perror("signal");
		exit(1);
	}

	int arr[25];		//입력한 번호를 저장하기 위한 배열
	memset(arr, -1, sizeof(arr));	//arr[25]를 -1로 초기화

	memset((char*)&sin, '\0', sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(PORTNUM);
	sin.sin_addr.s_addr = inet_addr("127.0.0.1");

	//소켓 생성
	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}

	//서버에 접속 요청 및 연결
	if (connect(sd, (struct sockaddr*)&sin, sizeof(sin))) {
		perror("connect");
		exit(1);
	}

	//난이도 해당하는 숫자 (1 or 2 or 3) 입력 후 전송
	while(1) {
		printf("LEVEL(1 or 2 or 3) 입력: ");
		fgets(getM, sizeof(getM), stdin);
		if (!strcmp(getM, "1\n") || !strcmp(getM, "2\n") || !strcmp(getM, "3\n"))
			break;
		printf("다시 입력하세요!\n");
	}
	
	strcpy(buf, getM);
	if (send(sd, buf, sizeof(buf), 0) == -1) {
		perror("send");
		exit(1);
	}

	//난이도에 해당하는 지뢰 개수 받음
	if (recv(sd, buf, sizeof(buf), 0) == -1) {
		perror("recv");
		exit(1);
	}
	
	//받은 지뢰 개수를 정수형으로 변환
	printf("지뢰 개수: %s\n", buf);
	int num = atoi(buf);

	//지뢰게임을 위한 지뢰판 생성
	for (int i = 0;i < 25;i++) {
		game[i] = i+1;
	}

	printf("\nCreating Mine ...\n");

	//지뢰 개수에 따라 지뢰에 해당하는 인덱스를 받음
	switch(num) {			
	case 5 :
		arrMine = creatMine(game, sd, num);
		break;
	case 11:
		arrMine = creatMine(game, sd, num);
		break;
	case 17:
		arrMine = creatMine(game, sd, num);
		break;
	}

	printf("--------------- Game Start ---------------\n\n");

	//지뢰찾기 게임을 위한 숫자판 출력
	printMine(game);
	
	//지뢰찾기 게임 실행
	playMine(arrMine, game, sd, num, arr, c, s);

	free(arrMine);
	close(sd);
	return 0;
}

void printMine(char* mine) {
	int count = 0;		
	
	for (int i = 0;i < 25;i++) {
		if (mine[i] == 'X') 
			printf("%c\t", mine[i]);
		else
			printf("%d\t", mine[i]);
		count++;
		if (count == 5) {
			printf("\n");
			count = 0;
		}
	}
	printf("\n");
}

char* creatMine(char* game, int sd, int num) {
	//char형 포인터 배열 arrMine 생성 후 메모리 할당
	char* arrMine = (char*)malloc(sizeof(char)*25);
	//game을 arrMine에 배열 복사
	memcpy(arrMine, game, sizeof(char)*25);
	char msg[MSG_SIZE];

	for (int i = 0;i < num;i++) {
		if ((read(sd, msg, sizeof(msg))) == -1) {
			printf("read");
			exit(1);
		}
		int r = atoi(msg);
		arrMine[r] = 0;
	}
	return arrMine;	
}

int enterClientNum(int sd, int arr[]) {
	char msg[MSG_SIZE];
	int clientNum;
	
	while(1) {
		printf(">> (1~25) 숫자 입력: ");
		scanf("%d", &clientNum);

		//범위 밖의 숫자를 입력 받았거나 중복된 숫자를 입력했을 경우
		if (clientNum < 1 || clientNum > 25 || saveNum(arr, clientNum) == 1) {
			printf("다시 입력하세요!\n");
			continue;
		}
		break;
	}

	//입력받은 숫자를 인덱스화 한 후 문자열로 전환
	sprintf(msg, "%d", clientNum-1);
	if((write(sd, msg, strlen(msg)+1)) == -1) {	
		printf("write");
		exit(1);
	}

	return clientNum;
}

void playMine(char* arrMine, char* game, int sd, int num, int arr[], int c, int s) {
	char msg[MSG_SIZE];
	int clientNum;		//사용자로부터 입력 받을 숫자
	c = 0;
	s = 0;

	while(1) {
		//사용자로부터 숫자를 입력 받는 함수
		clientNum = enterClientNum(sd, arr);
	
		//클라이언트가 지뢰에 해당하는 숫자를 입력했을 경우
		if (arrMine[clientNum-1] == 0) {
			num--;
			printf("\nClient 지뢰 발견!\n\n");
			c++;
			game[clientNum-1] = 'X';
			arrMine[clientNum-1] = 99;

			//지뢰를 모두 발견했을 경우
			if (num == 0) {
				printf("--------------- FINISH ---------------\n");
				break;
			}
		}
		//지뢰를 제외한 나머지 숫자를 입력했을 경우
		else {
			game[clientNum-1] = 'X';
			arrMine[clientNum-1] = 99;
		}
		
		//클라이언트가 입력한 후 바뀐 지뢰판 출력
		printMine(game);
		
		//서버가 랜덤으로 입력한 숫자를 받음
		if ((read(sd, msg, sizeof(msg))) == -1) {
			printf("read");
			exit(1);
		}

		int serverNum = atoi(msg);
		printf(">> Server: %d\n", serverNum+1);

		//서버로부터 받은 숫자를 저장
		if (saveNum(arr, serverNum+1) == 0);

		//서버가 지뢰에 해당하는 숫자를 입력했을 경우
		if (arrMine[serverNum] == 0) {
			num--;
			printf("\nServer 지뢰 발견!\n\n");
			s++;
			game[serverNum] = 'X';
			arrMine[serverNum] = 99;

			//지뢰를 모두 발견했을 경우
			if (num == 0) {
				printf("--------------- FINISH ---------------\n");
				break;
			}
		}
		//지뢰를 제외한 나머지 숫자를 입력 받았을 경우
		else {
			game[serverNum] = 'X';
			arrMine[serverNum] = 99;
		}
		//서버가 입력한 후 바뀐 지뢰판 출력
		printMine(game);
	}

	//게임 결과 저장
	result(c, s);
}

int saveNum(int arr[], int num) {
	for (int i = 0;i < 25;i++) {
		//중복된 값을 입력했을 경우 true 반환
		if (arr[i] == num)
			return 1;
		//배열의 값이 비워져 있는 경우 입력한 번호 채워 넣은 후 false 반환
		else {
			if (arr[i] == -1) {
				arr[i] = num;
				return 0;
			}
		}	
	}
}

void result(int c, int s) {
	if (c > s)
		printf("\n--------------- WIN ---------------\n");
	else if (c < s)
		printf("\n--------------- LOSE ---------------\n");
	//지뢰 개수는 홀수이기 때문에 사실상 나올 수 없는 else
	else				
		printf("\n--------------- DRAW ---------------\n");	
}

