#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <time.h>
#include <fcntl.h>

#define PORTNUM 9006
#define BUF_SIZE 1024
#define MSG_SIZE 1024

//지뢰판을 출력하는 함수
void printMine(char* mine);
//랜덤으로 설정된 지뢰가 존재하는 지뢰판을 생성하는 함수
char* creatMine(char* game, int num);		
//지뢰에 해당하는 인덱스 전송하는 함수
void sndMineNum(char* arrMine, int ns);
//지뢰 게임 한 ROUND를 실행하는 함수
void playMine(char* arrMine, char* game, int ns, int num, int arr[], int c, int s, FILE* wfp);
//입력한 번호가 중복됐는지 확인 후 저장하는 함수
int saveNum(int arr[], int num);
//게임 결과를 저장하는 함수
void result(int c, int s, FILE* wfp);
//서버, 클라이언트 점수를 저장하는 정수
int c, s;

int main(void) {
	char buf[BUF_SIZE];
	char game[25];
	char* arrMine;
	struct sockaddr_in sin, cli;
	int sd, ns, clientlen = sizeof(cli);
	FILE* wfp;

	//결과를 저장할 파일 result.txt 열기
	if ((wfp = fopen("result.txt", "w+")) == NULL) {
		perror("fopen: result.txt");
		exit(1);
	}

	int arr[25];			//입력한 번호를 저장하기 위한 배열
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

	//소켓 기술자, 주소 구조체 연결
	if (bind(sd, (struct sockaddr*)&sin, sizeof(sin))) {
		perror("bind");
		exit(1);
	}

	printf("Waiting ...\n");
	//클라이언트 접속요청 대기
	if (listen(sd, 5)) {				
		perror("listen");
		exit(1);
	}

	//클라이언트와 연결
	if ((ns = accept(sd, (struct sockaddr*)&cli, &clientlen)) == -1) {
		perror("accept");
		exit(1);
	}

	printf("OK\n");
	
	//난이도 해당하는 숫자를 먼저 받음 (1 or 2 or 3)
	if (recv(ns, buf, sizeof(buf), 0) == -1) {
		perror("recv");
		exit(1);
	}
	printf("LEVEL: %s", buf);	

	//난이도에 해당하는 지뢰 개수 전송
	switch (buf[0]) {
	case '1':
		strcpy(buf, "5");
		if (send(ns, buf, strlen(buf) + 1, 0) == -1) {
			perror("send");
			exit(1);
		}
		break;
	case '2':
		strcpy(buf, "11");
		if (send(ns, buf, strlen(buf) + 1, 0) == -1) {
			perror("send");
			exit(1);
		}
		break;
	case '3':
		strcpy(buf, "17");
		if (send(ns, buf, strlen(buf) + 1, 0) == -1) {
			perror("send");
			exit(1);
		}
		break;
	}

	printf("지뢰 개수: %s\n", buf);
	int num = atoi(buf);
	srand(time(NULL));		//실행할 때마다 다른 난수 발생시키기 위해

	//지뢰게임을 위한 지뢰판 생성
	for (int i = 0;i < 25;i++) {
		game[i] = i+1;
	}

	printf("\nCreating Mine ...\n");

	//지뢰 개수에 따른 지뢰 생성
	switch(num) {			
	case 5 :
		arrMine = creatMine(game, num);
		sndMineNum(arrMine, ns);

		break;
	case 11:
		arrMine = creatMine(game, num);
		sndMineNum(arrMine, ns);
		break;
	case 17:
		arrMine = creatMine(game, num);
		sndMineNum(arrMine, ns);
		break;
	}

	printf("--------------- Game Start ---------------\n\n");
	
	//지뢰찾기 게임을 위한 숫자판 출력
	printMine(game);

	//지뢰찾기 게임 실행
	playMine(arrMine, game, ns, num, arr, c, s, wfp);	

	free(arrMine);
	close(ns);
	close(sd);
	fclose(wfp);
	
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

char* creatMine(char* game, int num) {
	//char형 포인터 배열 arrMine 생성 후 메모리 할당
	char* arrMine = (char*)malloc(sizeof(char)*25);
	//game을 arrMine에 배열 복사
	memcpy(arrMine, game, sizeof(char)*25);
	while (num >0) {
		num--;
		int r = rand() %25;
		// 만약 이미 지뢰로 설정된 값이면 다시 생성
		if (arrMine[r] == 0){
			num++;
			continue;
	   	}
	   	arrMine[r] = 0;
	}
	return arrMine;
}

void sndMineNum(char* arrMine, int ns) {
	char msg[MSG_SIZE];
	for (int i = 0;i < 25;i++) {
		if (arrMine[i] == 0) {
			sprintf(msg, "%d", i);
			sleep(1);
			write(ns, msg, strlen(msg));
		}
	}
}

void playMine(char* arrMine, char* game, int ns, int num, int arr[], int c, int s, FILE* wfp) {
	char msg[MSG_SIZE];
	int serverNum;		//서버가 생성할 숫자
	c = 0;
	s = 0;

	while(1) {

		sleep(1);
		//클라이언트가 입력한 숫자를 받음
		if ((read(ns, msg, sizeof(msg))) == -1) {
			printf("read");
			exit(1);
		}		

		int clientNum = atoi(msg);
		printf(">> Client: %d\n", clientNum+1);

		// 사용자로부터 받은 숫자 저장
		if(saveNum(arr, clientNum+1) == 0);	

		//클라이언트가 지뢰에 해당하는 숫자를 입력했을 경우
		if (arrMine[clientNum] == 0) {
			num--;
			printf("\nClient 지뢰 발견!\n\n");
			c++;
			game[clientNum] = 'X';
			arrMine[clientNum] = 99;

			//지뢰를 모두 발견했을 경우
			if (num == 0) {
				printf("--------------- FINISH ---------------\n");
				break;
			}
		}
		//지뢰를 제외한 나머지 숫자를 입력 받았을 경우
		else {
			game[clientNum] = 'X';
			arrMine[clientNum] = 99;
		}
		
		//클라이언트가 입력한 후 바뀐 지뢰판 출력
		printMine(game);

		while(1) {
			//0~24 범위 안에서 난수 발생
			serverNum = rand() % 25;
			/* 중복된 숫자인지 검사 */
			if (saveNum(arr, serverNum+1) == 1)
				continue;
			else
				break;
		}

		printf(">> Server: %d\n", serverNum+1);
		sleep(1);
		/* 입력받은 숫자를 인덱스화 한 후 문자열로 전환 */
		sprintf(msg, "%d", serverNum);

		if((write(ns, msg, strlen(msg)+1)) == -1) {	
			printf("write");
			exit(1);
		}

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
		//지뢰를 제외한 나머지 숫자를 입력 했을 경우
		else {
			game[serverNum] = 'X';
			arrMine[serverNum] = 99;
		}
		
		//서버가 입력한 후 바뀐 지뢰판 출력
		printMine(game);
		
	}
	//게임 결과 저장
	result(c, s, wfp);
}

int saveNum(int arr[], int num) {
	for (int i = 0;i < 25;i++) {
		// 중복된 값을 입력했을 경우 true 반환
		if (arr[i] == num)
			return 1;
		// 배열의 값이 비워져 있는 경우 입력한 번호 채워 넣은 후 false 반환
		else {
			if (arr[i] == -1) {
				arr[i] = num;
				return 0;
			}
		}	
	}
}

void result(int c, int s, FILE *wfp) {
	char buf[BUF_SIZE];
	strcpy(buf, "CLIENT: ");
	fputs(buf, wfp);		// 입력된 문자열을 파일에 쓰기 
	sprintf(buf, "%d, ", c);	// 입력받은 숫자를 문자열로 전환 
	fputs(buf, wfp);		// 입력된 문자열을 파일에 쓰기 
	strcpy(buf,"SERVER: ");
	fputs(buf, wfp);		// 입력된 문자열을 파일에 쓰기 
	sprintf(buf, "%d ", s);		// 입력받은 숫자를 문자열로 전환 
	fputs(buf, wfp);		// 입력된 문자열을 파일에 쓰기 

	if (c > s){
		strcpy(buf, "(CLIENT WIN)");
		printf("\n--------------- LOSE ---------------\n");	
	}
	else if(c < s) {
		strcpy(buf, "(SERVER WIN)");
		printf("\n--------------- WIN ---------------\n");
	}
	//지뢰 개수는 홀수이기 때문에 사실상 나올 수 없는 else
	else{
		strcpy(buf, "DRAW");
		printf("\n--------------- DRAW ---------------\n");	
	}

	fputs(buf, wfp);
	fputs("\n", wfp);
}


