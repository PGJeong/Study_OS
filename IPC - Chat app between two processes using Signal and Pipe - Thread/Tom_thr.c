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
pthread_t receiverTID, senderTID; // Thread ID(TID)�� �����ϴ� ����
pthread_attr_t receiverAttr, senderAttr; // Thread �Ӽ�(attribute)�� �����ϴ� ��ü

void sigint(int sig) { // SIGINT �ڵ鷯
	
	repeat = 0;
	close(fdw); // FIFO�� �� �� ����(������) �ݱ�
	close(fdr); // FIFO�� �� �� ����(�б���) �ݱ�
	
	// FIFO�� �����ִ� ���, Receiver�� read()�� Sender�� write()�� ���� ����ϰ� �ֱ� ������ �����尡 ���� �����Ƿ� SIGINT �ڵ鷯���� �ݾ��ش�
	// pthread_equal(pthread_t t1, pthread_t t2) : �� �������� ID�� �����ϸ� 0�� �ƴ� ���� ��ȯ (����Ÿ�� : int)
}

void* receiver() {
	/* Receiver Thread (�޽��� ����/���) */

	printf("Receiver Thread Start\n");

	char other[80];

	while (repeat) {
		int msglen = read(fdr, other, sizeof(other) / sizeof(char)); // fdr���� ���ڿ� �о� other[]�� ���� (���ϰ� : ���� ���� ��)

		if (msglen > 0) {
			printf("\nother : %s\n", other);
		}
	}

	printf("\nReceiver Thread End (TID : %ld)", pthread_self());
	pthread_exit((void*)1);
	// pthread_exit (void *return_value) : �ش� �����常 ���� (�ٸ� �����忡 ���� X)
}

void* sender() {
	/* Sender Thread (�޽��� �Է�/�۽�) */

	printf("Sender Thread Start\n");

	char user[80];

	while (repeat) {
		fgets(user, sizeof(user) / sizeof(char), stdin);
		write(fdw, user, strlen(user) + 1); // fdw�� user[]�� ���ڿ� ����
	}

	printf("\nSender Thread End (TID : %ld)", pthread_self());
	pthread_exit((void*)2);
	// pthread_exit (void *return_value) : �ش� �����常 ���� (�ٸ� �����忡 ���� X)
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

	int receiver_return_val;
	int sender_return_val;

	pthread_join(receiverTID, (void*)&receiver_return_val); // Receiver Thread�� ������ ������ ���
	printf("\nReceiver Thread (TID : %ld) Return Value : %d", receiverTID, receiver_return_val);

	pthread_join(senderTID, (void*)&sender_return_val); // Sender Thread�� ������ ������ ��� (return_val : ������ ����� ��ȯ�� ������ ��)
	printf("\nSender Thread  (TID : %ld) Return Value : %d", senderTID, sender_return_val);

	printf("\nMain Thread End (TID : %lu)\n", pthread_self());

	return 0;
}