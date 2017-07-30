#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "threadpool.h"


/*
 * 头文件包含次序
 * C库,C++库,其他库.h,项目中.h
 * */


void* mytask(void* arg) {
	printf("thread 0x%x is working on task %d\n", (int)pthread_self(), *(int*)arg);
	sleep(1);
	free(arg);
	return NULL;
}

int main(void) {
	threadpool_t pool;
	threadpool_init(&pool, 1000);
	
	int i;
	for(i=0; i<100; i++) {
		int* arg = (int*)malloc(sizeof(int));
		*arg = i;
		threadpool_add_task(&pool, mytask, arg);
	}
	
	//sleep(15);
	threadpool_destory(&pool);
	return 0;
}
