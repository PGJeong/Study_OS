#include <stdio.h>
#include <string.h>
#include <unistd.h> // POSIX 운영체제 API. read(), write() 등
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h> // pid_t 타입(int형) 선언
#include <sys/wait.h> // wait()
#include <stdlib.h> // exit()

int status; // 자식 프로세스의 종료상태
int fdw;
int fdr;
pid_t cpid; // 자식 프로세스 PID
pid_t ppid; // 부모 프로세스 PID

void sigchld(int sig) { // SIGCHLD 핸들러

	printf("Parent process(PID: %d) End. Child Exit code : %d)\n", ppid, WEXITSTATUS(status));
	exit(0); // 프로세스 종료
}

void sigint(int sig) { // SIGINT 핸들러

	if (getpid() == ppid) { // 부모 프로세스인 경우 SIGINT 무시
		return;
	}
	printf("\nChild process(PID: %d) End\n", cpid);
	exit(0); // 프로세스 종료
}

int main()
{
	char user[80];
	char other[80];

	char* Tom2Jerry = "./Tom2Jerry"; // FIFO의 이름 경로명 (Tom → Jerry)
	char* Jerry2Tom = "./Jerry2Tom"; // FIFO의 이름 경로명 (Jerry → Tom)

	mkfifo(Tom2Jerry, 0666); // FIFO 생성
	mkfifo(Jerry2Tom, 0666); // FIFO 생성

	fdr = open(Tom2Jerry, O_RDONLY); // 쓰기모드(O_WRONLY)로 열기. fdw 변수는 File descriptor
	fdw = open(Jerry2Tom, O_WRONLY); // 읽기모드(O_RDONLY)로 열기. fdr 변수는 File descriptor

	pid_t pid = fork(); // 자식 프로세스 생성 (부모프로세스: pid 리턴, 자식 프로세스: 0리턴)

	if (pid > 0) {
		/* 부모 프로세스 코드 (메시지 수신/출력) */

		cpid = pid;
		ppid = getpid();
		printf("Parent process Start\n");
		signal(SIGINT, sigint); // SIGINT 신호 핸들러 등록
		signal(SIGCHLD, sigchld); // SIGCHLD 신호 핸들러 등록

		while (1) {
			int msglen = read(fdr, other, sizeof(other) / sizeof(char)); // fdr에서 문자열 읽어 other[]에 저장 (리턴값 : 읽은 문자 수)

			if (msglen > 0) {
				printf("\nother >> %s\n", other);
			}

			sleep(1);
		}

		wait(&status);
	}
	else if (pid == 0) {
		/* 자식 프로세스 코드 (메시지 입력/송신) */

		cpid = getpid();
		ppid = getppid();
		printf("Child process Start\n");
		signal(SIGINT, sigint); // SIGINT 신호 핸들러 등록

		while (1) {
			fgets(user, sizeof(user) / sizeof(char), stdin);
			write(fdw, user, strlen(user) + 1); // fdw에 user[]의 문자열 쓰기
		}
	}
	else if (pid == -1) {
		/* fork() 오류처리 */

		printf("fork() Error");
		return 0;
	}

	close(fdw); // FIFO의 한 쪽 끝단(쓰기모드) 닫기
	close(fdr); // FIFO의 한 쪽 끝단(읽기모드) 닫기
	return 0;
}
