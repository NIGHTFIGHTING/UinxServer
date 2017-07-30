#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <pthread.h>

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


void* thread_routine(void* arg) {
	int i;
	for(i=0; i<20; i++) {
		printf("B");
		fflush(stdout);
		usleep(20);
		//if(3==i)
			//pthread_exit(const_cast<char*>("ABC"));
	}
	
	sleep(3);
	return const_cast<char*>("DEF");
}


int main(int argc, char* argv[]) {
	pthread_t tid;
	int ret;
	if((ret = pthread_create(&tid, NULL, thread_routine, NULL)) != 0) {
		fprintf(stderr, "pthead_create:%s\n", strerror(ret));
		exit(EXIT_FAILURE);
	}
	
	int i;
	for(i=0; i<20; i++) {
		printf("A");
		fflush(stdout);
		usleep(20);
	}

	//sleep(1);
	void* value;
	if((ret = pthread_join(tid, &value)) != 0) {
		fprintf(stderr, "pthead_join:%s\n", strerror(ret));
		exit(EXIT_FAILURE);
	}
	printf("\n");
	printf("return msg=%s\n", (char*)value); 
	return 0;
}
