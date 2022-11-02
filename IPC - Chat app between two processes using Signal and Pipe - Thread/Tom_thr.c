#include <stdio.h>
#include <string.h>
#include <unistd.h> // POSIX 운영체제 API. read(), write() 등
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h> // pid_t 타입(int형) 선언
#include <sys/wait.h> // wait()
#include <stdlib.h> // exit()
#include <pthread.h>

void* receiver_return_val;
void* sender_return_val;

int fdw;
int fdr;
pthread_t receiverTID, senderTID; // Thread ID(TID)를 저장하는 변수
pthread_attr_t receiverAttr, senderAttr; // Thread 속성(attribute)을 저장하는 객체

void sigint(int sig) { // SIGINT 핸들러

	if (pthread_equal(receiverTID, pthread_self()) != 0) { // Receiver Thread인 경우
		pthread_exit((void*)21);
		return;
	}
	if (pthread_equal(senderTID, pthread_self()) != 0) { // Sender Thread인 경우
		pthread_exit((void*)22);
		return;
	}
	
	// pthread_equal(pthread_t t1, pthread_t t2) : 두 스레드의 ID가 동일하면 0이 아닌 값을 반환 (리턴타입 : int)
	// pthread_self() : 자신의 TID 반환
	// pthread_exit (void *return_value) : 해당 스레드만 종료 (다른 스레드에 영향 X)
	
	// exit(0); // 프로세스 종료. 해당 시스템 호출은 프로세스 단위로 처리되므로 모든 스레드가 종료.
}

void* receiver() {
	/* Receiver Thread (메시지 수신/출력) */

	printf("Receiver Thread Start\n");
	signal(SIGINT, sigint); // SIGINT 신호 핸들러 등록

	char other[80];

	while (1) {
		int msglen = read(fdr, other, sizeof(other) / sizeof(char)); // fdr에서 문자열 읽어 other[]에 저장 (리턴값 : 읽은 문자 수)

		if (msglen > 0) {
			printf("\nother : %s\n", other);
		}
	}
}

void* sender() {
	/* Sender Thread (메시지 입력/송신) */

	printf("Sender Thread Start\n");
	signal(SIGINT, sigint); // SIGINT 신호 핸들러 등록

	char user[80];

	while (1) {
		fgets(user, sizeof(user) / sizeof(char), stdin);
		write(fdw, user, strlen(user) + 1); // fdw에 user[]의 문자열 쓰기
	}
}

int main()
{
	/* Main Thread */

	printf("Main Thread Start\n");
	signal(SIGINT, sigint); // SIGINT 신호 핸들러 등록

	// Signal은 해당 프로세스의 모든 스레드에 공유된다

	char* Tom2Jerry = "./Tom2Jerry"; // FIFO의 이름 경로명 (Tom → Jerry)
	char* Jerry2Tom = "./Jerry2Tom"; // FIFO의 이름 경로명 (Jerry → Tom)

	mkfifo(Tom2Jerry, 0666); // FIFO 생성
	mkfifo(Jerry2Tom, 0666); // FIFO 생성

	fdw = open(Tom2Jerry, O_WRONLY); // 쓰기모드(O_WRONLY)로 열기. fdw 변수는 File descriptor
	fdr = open(Jerry2Tom, O_RDONLY); // 읽기모드(O_RDONLY)로 열기. fdr 변수는 File descriptor

	/* Thread 생성 */
	
	pthread_attr_init(&receiverAttr); // Thread 속성 객체를 기본 속성 값들로 초기화
	pthread_attr_init(&senderAttr);

	pthread_create(&receiverTID, &receiverAttr, receiver, NULL); // Receiver Thread 생성. 매겨변수가 없으므로 마지막 매개변수 NULL
	pthread_create(&senderTID, &senderAttr, sender, NULL); // Sender Thread 생성

	pthread_join(receiverTID, &receiver_return_val); // Thread Join. Receiver Thread가 종료할 때까지 대기
	printf("Receiver Thread End (return value : %d)\n", *(int*)receiver_return_val);

	pthread_join(senderTID, &sender_return_val); // Thread Join. Sender Thread가 종료할 때까지 대기 (return_val : 스레드 종료시 반환값 저장할 곳)	
	printf("Sender Thread End (return value : %d)\n", *(int*)sender_return_val);

	close(fdw); // FIFO의 한 쪽 끝단(쓰기모드) 닫기
	close(fdr); // FIFO의 한 쪽 끝단(읽기모드) 닫기

	printf("Main Thread End\n");

	return 0;
}
