#include <stdio.h>
#include <string.h>
#include <unistd.h> // POSIX �ü�� API. read(), write() ��
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h> // pid_t Ÿ��(int��) ����
#include <sys/wait.h> // wait()
#include <stdlib.h> // exit()
#include <pthread.h>

void* receiver_return_val;
void* sender_return_val;

int fdw;
int fdr;
pthread_t receiverTID, senderTID; // Thread ID(TID)�� �����ϴ� ����
pthread_attr_t receiverAttr, senderAttr; // Thread �Ӽ�(attribute)�� �����ϴ� ��ü

void sigint(int sig) { // SIGINT �ڵ鷯

	if (pthread_equal(receiverTID, pthread_self()) != 0) { // Receiver Thread�� ���
		pthread_exit((void*)21);
		return;
	}
	if (pthread_equal(senderTID, pthread_self()) != 0) { // Sender Thread�� ���
		pthread_exit((void*)22);
		return;
	}
	
	// pthread_equal(pthread_t t1, pthread_t t2) : �� �������� ID�� �����ϸ� 0�� �ƴ� ���� ��ȯ (����Ÿ�� : int)
	// pthread_self() : �ڽ��� TID ��ȯ
	// pthread_exit (void *return_value) : �ش� �����常 ���� (�ٸ� �����忡 ���� X)
	
	// exit(0); // ���μ��� ����. �ش� �ý��� ȣ���� ���μ��� ������ ó���ǹǷ� ��� �����尡 ����.
}

void* receiver() {
	/* Receiver Thread (�޽��� ����/���) */

	printf("Receiver Thread Start\n");
	signal(SIGINT, sigint); // SIGINT ��ȣ �ڵ鷯 ���

	char other[80];

	while (1) {
		int msglen = read(fdr, other, sizeof(other) / sizeof(char)); // fdr���� ���ڿ� �о� other[]�� ���� (���ϰ� : ���� ���� ��)

		if (msglen > 0) {
			printf("\nother : %s\n", other);
		}
	}
}

void* sender() {
	/* Sender Thread (�޽��� �Է�/�۽�) */

	printf("Sender Thread Start\n");
	signal(SIGINT, sigint); // SIGINT ��ȣ �ڵ鷯 ���

	char user[80];

	while (1) {
		fgets(user, sizeof(user) / sizeof(char), stdin);
		write(fdw, user, strlen(user) + 1); // fdw�� user[]�� ���ڿ� ����
	}
}

int main()
{
	/* Main Thread */

	printf("Main Thread Start\n");
	signal(SIGINT, sigint); // SIGINT ��ȣ �ڵ鷯 ���

	// Signal�� �ش� ���μ����� ��� �����忡 �����ȴ�

	char* Tom2Jerry = "./Tom2Jerry"; // FIFO�� �̸� ��θ� (Tom �� Jerry)
	char* Jerry2Tom = "./Jerry2Tom"; // FIFO�� �̸� ��θ� (Jerry �� Tom)

	mkfifo(Tom2Jerry, 0666); // FIFO ����
	mkfifo(Jerry2Tom, 0666); // FIFO ����

	fdw = open(Tom2Jerry, O_WRONLY); // ������(O_WRONLY)�� ����. fdw ������ File descriptor
	fdr = open(Jerry2Tom, O_RDONLY); // �б���(O_RDONLY)�� ����. fdr ������ File descriptor

	/* Thread ���� */
	
	pthread_attr_init(&receiverAttr); // Thread �Ӽ� ��ü�� �⺻ �Ӽ� ����� �ʱ�ȭ
	pthread_attr_init(&senderAttr);

	pthread_create(&receiverTID, &receiverAttr, receiver, NULL); // Receiver Thread ����. �Űܺ����� �����Ƿ� ������ �Ű����� NULL
	pthread_create(&senderTID, &senderAttr, sender, NULL); // Sender Thread ����

	pthread_join(receiverTID, &receiver_return_val); // Thread Join. Receiver Thread�� ������ ������ ���
	printf("Receiver Thread End (return value : %d)\n", *(int*)receiver_return_val);

	pthread_join(senderTID, &sender_return_val); // Thread Join. Sender Thread�� ������ ������ ��� (return_val : ������ ����� ��ȯ�� ������ ��)	
	printf("Sender Thread End (return value : %d)\n", *(int*)sender_return_val);

	close(fdw); // FIFO�� �� �� ����(������) �ݱ�
	close(fdr); // FIFO�� �� �� ����(�б���) �ݱ�

	printf("Main Thread End\n");

	return 0;
}
