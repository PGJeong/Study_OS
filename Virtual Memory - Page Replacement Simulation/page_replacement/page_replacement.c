#define _CRT_SECURE_NO_WARNINGS
// C program for FIFO page replacement algorithm
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <climits>

#define MAX_PROCESS 	8
#define DEFAULT_PAGES 	3 	// the number of pages to be allocated when process loading
#define MAX_PAGE 	8
#define MAX_FRAME 	8

// PAGE_TABLE index information
#define P_VALID 	0
#define P_DIRTY 	1
#define P_FRAME_NO 	2

// FRAME index information
#define F_PID 		0
#define F_PAGE_NO 	1
#define F_TIME 		2

time_t sys_start_time;

typedef struct {
	int pid;
	int allocated_pages;	//the number of allocated pages
	//i-th page_table entry 
	//page_table[i][P_VALID]: presence bit, page_table[i][P_DIRTY]: modified bit, 
	//page_table[i][P_FRAME_NO]: allocated frame no.
	int page_table[MAX_PAGE][3]; // ★ 페이지 테이블 // (순서대로) Presence, Modified, FrameNo (Presence가 0인 경우 DiskBlockNo)
} process;

// i-th frame entry
// frame[i][F_PID]: allocated process id, frame[i][F_PAGE_NO]: mapped page no(page_table index). 
// frame[i][F_TIME]: last access time
int frames[MAX_FRAME][3];	// ★ 물리 메모리(프레임) // (순서대로) PID, PageNo, AccessTime
int allocated_frames = 0;	// the number of allocated frames
int page_counter = 0;		// victim index using FIFO page replacement algorithm

process* proc[MAX_PROCESS];	// process information(PCB)
int no_of_proc = 0;		// the number of created processes

/*
int find_victim() { // page_replacement algorithm (FIFO 알고리즘)
	return (page_counter++) % MAX_FRAME;
}
*/

int find_victim() { // page_replacement algorithm (★ LRU 알고리즘)
	int index = 0;

	for (int i = 0; i < MAX_FRAME; i++) {
		if (frames[index][F_TIME] > frames[i][F_TIME]) // AccessTime 비교 (값이 작을수록 참조된 시간이 오래된 것)
			index = i;
	}

	return index; // 가장 오래 전에 참조된 페이지의 프레임No 리턴 (희생 페이지로 선택)
}

int swap_area[100][2]; // ★ 스왑 영역 // (순서대로) PID, PageNo
int sa_index = 0;

void swap_out(int frameNo) { // ★ Swap Out
	/* 스왑 영역으로 이동 */
	swap_area[sa_index][F_PID] = frames[frameNo][F_PID];
	swap_area[sa_index][F_PAGE_NO] = frames[frameNo][F_PAGE_NO];

	/* 페이지 테이블 수정 */
	int pid = frames[frameNo][F_PID];
	int pageNo = frames[frameNo][F_PAGE_NO];

	// 실제 운영체제에서는 Modified 비트가 1인 경우에만 Swap Out
	proc[pid]->page_table[pageNo][P_VALID] = 0; // Presence 비트를 0으로 수정
	proc[pid]->page_table[pageNo][P_FRAME_NO] = sa_index; // Swap Out 되었으므로 프레임번호 대신 디스크블록번호 대입

	sa_index++;
}

void swap_in(int pid, int pageNo) { // ★ Swap In
	int diskBlockNo = 9999;

	for (int i = 0; i < 100; i++) {
		if (swap_area[i][F_PID] == pid && swap_area[i][F_PAGE_NO] == pageNo) // 스왑 영역에 찾고자 하는 특정 프로세스의 특정 페이지가 있으면
			diskBlockNo = i;
	}

	// 미완성
}

void insert_page(int pid) {
	printf("insert%d\n", pid);
	if (pid < 0 || pid >= no_of_proc) {
		printf("Invalid pid\n");
		return;
	}

	int page_no = proc[pid]->allocated_pages;
	if (page_no == MAX_PAGE) {
		printf("No more page can be allocated\n");
		return;
	}
	else proc[pid]->allocated_pages++;

	int target = allocated_frames;

	if (allocated_frames == MAX_FRAME) {
		target = find_victim(); // page_replacement algorithm 
		swap_out(target);

		// For using virtual memory, you need to implement the followings:
		// swap_out | target:victim -> hdd; 

		int t_pid = frames[target][F_PID];
		int t_page_no = frames[target][F_PAGE_NO];

		proc[t_pid]->page_table[t_page_no][P_VALID] = 0;
		proc[t_pid]->page_table[t_page_no][P_DIRTY] = 0;
		// proc[pid]->page_table[t_page_no][P_FRAME_NO] = block_no;

	}
	else allocated_frames++;

	proc[pid]->page_table[page_no][P_VALID] = 1;
	// we suppose that the page should be modified, always.
	proc[pid]->page_table[page_no][P_DIRTY] = 1;
	proc[pid]->page_table[page_no][P_FRAME_NO] = target;

	frames[target][F_PID] = pid;
	frames[target][F_PAGE_NO] = page_no;
	frames[target][F_TIME] = (int)(difftime(time(NULL), sys_start_time));
}

void access_page(int pid, int page_no) {
	if (pid < 0 || pid >= no_of_proc) {
		printf("Invalid pid\n");
		return;
	}

	if (proc[pid]->page_table[page_no][P_FRAME_NO] < 0) {
		printf("Invalid page_no\n");
		return;
	}

	if (!proc[pid]->page_table[page_no][P_VALID]) {
		printf(" - Page fault occurs - \n");
		// For using virtual memory, you need to implement the followings:
	// swap_in |  target:ram <- hdd; (also victim -> hdd)

		return;
	}

	// we suppose that the page should be modified, always.
	proc[pid]->page_table[page_no][P_DIRTY] = 1;
	int target = proc[pid]->page_table[page_no][P_FRAME_NO];

	frames[target][F_TIME] = (int)difftime(time(NULL), sys_start_time);
}

void create_process() {
	if (no_of_proc == MAX_PROCESS) {
		printf("No more process can be created\n");
		return;
	}

	proc[no_of_proc] = (process*)malloc(sizeof(process));
	proc[no_of_proc]->pid = no_of_proc;
	proc[no_of_proc]->allocated_pages = 0;

	for (int i = 0; i < MAX_PAGE; i++)
		proc[no_of_proc]->page_table[i][P_FRAME_NO] = -1;

	no_of_proc++;

	// default memory allocation though loading : DEFAULT_PAGES
	for (int i = 0; i < DEFAULT_PAGES; i++)
		insert_page(proc[no_of_proc - 1]->pid);

}

void show_frames() {
	printf("\n");
	for (int i = 0; i < MAX_FRAME; i++) {
		printf(" Frame %d", i);

		if (frames[i][F_PID] < 0) printf(" - \n");
		else printf(" %d:%d(%d)\n", frames[i][F_PID], frames[i][F_PAGE_NO], frames[i][F_TIME]);
	}
}

void show_pages(int pid) {
	if (pid < 0 || pid >= no_of_proc) {
		printf("Invalid pid\n");
		return;
	}

	printf("\n");
	for (int i = 0; i < MAX_PAGE; i++) {
		printf(" Page %d", i);

		if (proc[pid]->page_table[i][P_FRAME_NO] < 0) printf(" - \n");
		else if (proc[pid]->page_table[i][P_VALID])
			printf(" %d:%d(RAM)\n", proc[pid]->page_table[i][P_DIRTY], proc[pid]->page_table[i][P_FRAME_NO]);
		else printf(" %d:%d(SWAPPED)\n", proc[pid]->page_table[i][P_DIRTY], proc[pid]->page_table[i][P_FRAME_NO]);
	}
}

int main() {
	sys_start_time = time(NULL);

	// Physical memory initializing
	for (int i = 0; i < MAX_FRAME; i++)
		frames[i][0] = -1;

	while (1) {
		int choice = 0, pid = -1, page = -1;
		printf("\n\n1. Create Process\n2. Insert Page\n3. Access Page");
		printf("\n4. Show Frames\n5. Show Pages\n6. Exit\nEnter Your Choice:");
		scanf("%d", &choice);
		switch (choice) {
		case 1: create_process(); break;
		case 2:
			printf(" PID :");
			scanf("%d", &pid);
			insert_page(pid);
			break;
		case 3:
			printf(" PID :");
			scanf("%d", &pid);
			printf(" PAGE_NO :");
			scanf("%d", &page);
			access_page(pid, page);
			break;
		case 4:
			show_frames();
			break;
		case 5:
			printf(" PID :");
			scanf("%d", &pid);
			show_pages(pid);
			break;
		case 6: exit(1);
		}
	}

	return 0;
}