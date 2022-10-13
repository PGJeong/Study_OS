#include <stdio.h>
#include <string.h>
#include <unistd.h> // POSIX �ü�� API. read(), write() ��
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h> // pid_t Ÿ��(int��) ����
#include <sys/wait.h> // wait()
#include <stdlib.h> // exit()

int status; // �ڽ� ���μ����� �������
int fdw;
int fdr;
pid_t cpid; // �ڽ� ���μ��� PID
pid_t ppid; // �θ� ���μ��� PID

void sigchld(int sig) { // SIGCHLD �ڵ鷯

	printf("Parent process(PID: %d) End. Child Exit code : %d)\n", ppid, WEXITSTATUS(status));
	exit(0); // ���μ��� ����
}

void sigint(int sig) { // SIGINT �ڵ鷯

	if (getpid() == ppid) { // �θ� ���μ����� ��� SIGINT ����
		return;
	}
	printf("\nChild process(PID: %d) End\n", cpid);
	exit(0); // ���μ��� ����
}

int main()
{
	char user[80];
	char other[80];

	char* Tom2Jerry = "./Tom2Jerry"; // FIFO�� �̸� ��θ� (Tom �� Jerry)
	char* Jerry2Tom = "./Jerry2Tom"; // FIFO�� �̸� ��θ� (Jerry �� Tom)

	mkfifo(Tom2Jerry, 0666); // FIFO ����
	mkfifo(Jerry2Tom, 0666); // FIFO ����

	fdr = open(Tom2Jerry, O_RDONLY); // ������(O_WRONLY)�� ����. fdw ������ File descriptor
	fdw = open(Jerry2Tom, O_WRONLY); // �б���(O_RDONLY)�� ����. fdr ������ File descriptor

	pid_t pid = fork(); // �ڽ� ���μ��� ���� (�θ����μ���: pid ����, �ڽ� ���μ���: 0����)

	if (pid > 0) {
		/* �θ� ���μ��� �ڵ� (�޽��� ����/���) */

		cpid = pid;
		ppid = getpid();
		printf("Parent process Start\n");
		signal(SIGINT, sigint); // SIGINT ��ȣ �ڵ鷯 ���
		signal(SIGCHLD, sigchld); // SIGCHLD ��ȣ �ڵ鷯 ���

		while (1) {
			int msglen = read(fdr, other, sizeof(other) / sizeof(char)); // fdr���� ���ڿ� �о� other[]�� ���� (���ϰ� : ���� ���� ��)

			if (msglen > 0) {
				printf("\nother >> %s\n", other);
			}

			sleep(1);
		}

		wait(&status);
	}
	else if (pid == 0) {
		/* �ڽ� ���μ��� �ڵ� (�޽��� �Է�/�۽�) */

		cpid = getpid();
		ppid = getppid();
		printf("Child process Start\n");
		signal(SIGINT, sigint); // SIGINT ��ȣ �ڵ鷯 ���

		while (1) {
			fgets(user, sizeof(user) / sizeof(char), stdin);
			write(fdw, user, strlen(user) + 1); // fdw�� user[]�� ���ڿ� ����
		}
	}
	else if (pid == -1) {
		/* fork() ����ó�� */

		printf("fork() Error");
		return 0;
	}

	close(fdw); // FIFO�� �� �� ����(������) �ݱ�
	close(fdr); // FIFO�� �� �� ����(�б���) �ݱ�
	return 0;
}
