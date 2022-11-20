#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdlib.h>

#define N_COUNTER 4 // 공유 버퍼의 크기. 정수 4개 저장
#define MILLI 1000

void mywrite(int n);
int myread();

pthread_mutex_t critical_section; // 뮤텍스
sem_t semWrite; // 세마포 : 쓰기 가능한 버퍼의 개수 (counter가 0이면 버퍼 full)
sem_t semRead;  // 세마포 : 읽기 가능한 버퍼의 개수 (counter가 0이면 버퍼 empty)

int queue[N_COUNTER]; // 정수 4개를 저장하는 공유 버퍼(원형 큐)
int wptr; // queue[]에 저장할 다음 인덱스
int rptr; // queue[]에서 읽을 다음 인덱스

static __thread int no; // Thread Local Storage(TLS), 스레드 번호를 표시하기 위해 사용

int pthread_sleep(int seconds) {
	pthread_mutex_t mutex; // 조건변수는 race condition(경쟁상태)을 피해야하므로 반드시 뮤텍스(mutex)와 함께 사용
	pthread_cond_t conditionvar; // 조건변수 (스레드간 동기화를 위해 사용, 조건변수에 Signal이 전달 될 때 까지 대기)
	struct timespec timetoexpire;

	if (pthread_mutex_init(&mutex, NULL)) {
		return -1;
	}
	if (pthread_cond_init(&conditionvar, NULL)) {
		return -1;
	}

	//When to expire is an absolute time, so get the current time and add it to our delay time
	timetoexpire.tv_sec = (unsigned int)time(NULL) + seconds;
	timetoexpire.tv_nsec = 0;

	// pthread_cond_wait() : 조건변수로 Signal이 전달되기를 기다린다 (Signal이 도착할 때까지 무한시간 대기)
	// pthread_cond_timedwait(cond, mutex, abstime) : pthread_cond_wait()의 시간제한 버전 (abstime 시간 이후 ETIMEOUT를 반환)
	return pthread_cond_timedwait(&conditionvar, &mutex, &timetoexpire);
}

void* producer(void* arg) { // 생산자 스레드 함수
	no = *(int*)arg;

	// m 밀리초 동안 잠을 잔다.
	int m = rand() % 10; // 0~9 사이의 랜덤한 정수
	// usleep(MILLI * m * 10); // m*10 밀리초동안 잠자기
	pthread_sleep(m * 10 / MILLI);

	mywrite(no); // 생상자 스레드 번호(no)를 공유버퍼에 저장

	int w; // semWrite의 현재 counter 값
 	int r; // semRead의 현재 counter 값
	sem_getvalue(&semWrite, &w); // 세마포의 현재 counter값을 두번째 매개변수에 반환
	sem_getvalue(&semRead, &r);

	printf("Producer%d : wrote %d (W : %d/%d, R : %d/%d)\n", no, no, w, N_COUNTER, r, N_COUNTER);

	return NULL;
}

void* consumer(void* arg) { // 소비자 스레드 함수
	no = *(int*)arg;

	// m 밀리초 동안 잠을 잔다.
	int m = rand() % 10; // 0~9 사이의 랜덤한 정수
	// usleep(MILLI * m * 10); // m*10 밀리초동안 잠자기
	pthread_sleep(m * 10 / MILLI);

	int n = myread(); // 공유버퍼의 맨 앞에 있는 정수 읽어 리턴

	int w; // semWrite의 현재 counter 값
	int r; // semRead의 현재 counter 값
	sem_getvalue(&semWrite, &w); // counter값 반환 (두 번째 매개변수 : 값이 저장되는 위치)
	sem_getvalue(&semRead, &r);

	printf("\t\t\t\t\tConsumer%d : read %d (W : %d/%d, R : %d/%d)\n", no, n, w, N_COUNTER, r, N_COUNTER);

	return NULL;
}

void mywrite(int n) { // 정수 n을 queue[]에 삽입
	sem_wait(&semWrite); // semWrite P연산 (counter 1 감소)

	pthread_mutex_lock(&critical_section); // 뮤텍스 락 잠그기
	queue[wptr] = n; // 버퍼에 정수 n을 삽입한다.
	wptr++;
	wptr %= N_COUNTER;
	pthread_mutex_unlock(&critical_section); // 뮤텍스 락 열기

	sem_post(&semRead); // semRead V연산 (counter 1 증가)
}

int myread() { // queue[]의 맨 앞에 있는 정수를 읽어 리턴
	sem_wait(&semRead); // semRead P연산 (counter 1 감소)

	pthread_mutex_lock(&critical_section); // 뮤텍스 락 잠그기
	int n = queue[rptr]; // 버퍼에서 정수를 읽는다.
	rptr++;
	rptr %= N_COUNTER;
	pthread_mutex_unlock(&critical_section); // 뮤텍스 락 열기

	sem_post(&semWrite); // semWrite V연산 (counter 1 증가)

	return n;
}

int main() {
	pthread_t pro_thr[10]; // 생산자 스래드 10개
	pthread_t con_thr[10]; // 소비자 스래드 10개

	srand(time(NULL)); // 난수 발생 초기화(seed 랜덤 지정)
	pthread_mutex_init(&critical_section, NULL); // 뮤텍스 락 초기화

	// 세마포 초기화 : N_COUNTER 개의 자원으로 초기화
	sem_init(&semWrite, 0, N_COUNTER); // counter를 N_COUNTER로 초기화
	sem_init(&semRead, 0, 0); // counter를 0으로 초기화

	// producer 스레드 생성
	for (int i = 0; i < 10; i++) {
		int tmp = i;
		pthread_create(&pro_thr[i], NULL, producer, (void*)&tmp); // i+1은 매개변수로, 스레드 번호를 매기기 위해 사용
		pthread_sleep(1);
	}

	// consumer 스레드 생성
	for (int i = 0; i < 10; i++) {
		int tmp = i;
		pthread_create(&con_thr[i], NULL, consumer, (void*)&tmp); // i+1은 매개변수로, 스레드 번호를 매기기 위해 사용
		pthread_sleep(1);
	}

	// 모든 스레드가 소멸할 때까지 대기
	for (int i = 0; i < 10; i++)
		pthread_join(pro_thr[i], NULL);
	for (int i = 0; i < 10; i++)
		pthread_join(con_thr[i], NULL);

	sem_destroy(&semRead); // 세마포 기능 소멸
	sem_destroy(&semWrite); // 세마포 기능 소멸

	pthread_mutex_destroy(&critical_section); // 뮤텍스 락 소멸

	return 0;
}
