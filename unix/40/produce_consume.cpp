#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <pthread.h>
#include <semaphore.h>

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>


#define ERR_EXIT(m) \
    do \
    { \
    	perror(m); \
    	exit(EXIT_FAILURE); \
    }while(0)


#define CONSUMERS_COUNT 2
#define PRODUCERS_COUNT 5

pthread_mutex_t g_mutex;
pthread_cond_t g_cond;

pthread_t g_thread[CONSUMERS_COUNT + PRODUCERS_COUNT];

//无界缓冲区
int nready = 0;//表示当前缓冲区产品个数
void* consume(void* arg) {
	int i;
	int num = *(int*)arg;
	free(arg);
	while(1) {
		pthread_mutex_lock(&g_mutex);
		while(nready == 0) {
			printf("%d begin wait a condition ...\n", num);
			pthread_cond_wait(&g_cond, &g_mutex);
		}

		printf("%d end wait a condition ...\n", num);
		printf("%d begin consume product ...\n", num);
		--nready; // 消费产品
		printf("%d end consume product ...\n", num);
		pthread_mutex_unlock(&g_mutex);
		sleep(5);
	}
	return NULL;
}

void* produce(void* arg) {
	int i;
	int num = *(int*)arg;
	free(arg);
	while(1) {
		pthread_mutex_lock(&g_mutex);
		printf("%d begin produce product ...\n", num);
		++nready;
		printf("%d end produce product ...\n", num);
		pthread_cond_signal(&g_cond);
		printf("%d signal ...\n", num);
		pthread_mutex_unlock(&g_mutex);
		sleep(5);
	}
	return NULL;
}
int main(int argc, char* argv[]) {
	int i;
	pthread_mutex_init(&g_mutex, NULL); 
	pthread_cond_init(&g_cond, NULL);
	
	for(i=0; i<CONSUMERS_COUNT; i++) {
		int* p = (int*)malloc(sizeof(int));
		*p = i;
		pthread_create(&g_thread[i], NULL, consume, (void*)p); 
	}
	sleep(1);

	for(i=0; i<PRODUCERS_COUNT; i++) {
		int* p = (int*)malloc(sizeof(int));
		*p = i;
		pthread_create(&g_thread[CONSUMERS_COUNT+i], NULL, produce, (void*)p); 
	}
	for(i=0; i<CONSUMERS_COUNT+PRODUCERS_COUNT; i++) {
		pthread_join(g_thread[i], NULL);
	}
//	pthread_mutex_destory(&g_mutex);
//	pthread_cond_destory(&g_cond);
	return 0;
}
