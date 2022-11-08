#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <pthread.h>

int repeat = 1;

int fdw;
int fdr;
pthread_t receiverTID, senderTID; // Thread ID(TID)를 저장하는 변수
pthread_attr_t receiverAttr, senderAttr; // Thread 속성(attribute)을 저장하는 객체

void sigint(int sig) { // SIGINT 핸들러
	
	repeat = 0;
	close(fdw); // FIFO의 한 쪽 끝단(쓰기모드) 닫기
	close(fdr); // FIFO의 한 쪽 끝단(읽기모드) 닫기
	
	// FIFO가 열려있는 경우, Receiver의 read()와 Sender의 write()가 정상 대기하고 있기 때문에 스레드가 죽지 않으므로 SIGINT 핸들러에서 닫아준다
	// pthread_equal(pthread_t t1, pthread_t t2) : 두 스레드의 ID가 동일하면 0이 아닌 값을 반환 (리턴타입 : int)
}

void* receiver() {
	/* Receiver Thread (메시지 수신/출력) */

	printf("Receiver Thread Start\n");

	char other[80];

	while (repeat) {
		int msglen = read(fdr, other, sizeof(other) / sizeof(char)); // fdr에서 문자열 읽어 other[]에 저장 (리턴값 : 읽은 문자 수)

		if (msglen > 0) {
			printf("\nother : %s\n", other);
		}
	}

	printf("\nReceiver Thread End (TID : %ld)", pthread_self());
	pthread_exit((void*)1);
	// pthread_exit (void *return_value) : 해당 스레드만 종료 (다른 스레드에 영향 X)
}

void* sender() {
	/* Sender Thread (메시지 입력/송신) */

	printf("Sender Thread Start\n");

	char user[80];

	while (repeat) {
		fgets(user, sizeof(user) / sizeof(char), stdin);
		write(fdw, user, strlen(user) + 1); // fdw에 user[]의 문자열 쓰기
	}

	printf("\nSender Thread End (TID : %ld)", pthread_self());
	pthread_exit((void*)2);
	// pthread_exit (void *return_value) : 해당 스레드만 종료 (다른 스레드에 영향 X)
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

	int receiver_return_val;
	int sender_return_val;

	pthread_join(receiverTID, (void*)&receiver_return_val); // Receiver Thread가 종료할 때까지 대기
	printf("\nReceiver Thread (TID : %ld) Return Value : %d", receiverTID, receiver_return_val);

	pthread_join(senderTID, (void*)&sender_return_val); // Sender Thread가 종료할 때까지 대기 (return_val : 스레드 종료시 반환값 저장할 곳)
	printf("\nSender Thread  (TID : %ld) Return Value : %d", senderTID, sender_return_val);

	printf("\nMain Thread End (TID : %lu)\n", pthread_self());

	return 0;
}