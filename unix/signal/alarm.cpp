#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

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
    

void handler(int sig);

int main () {
    signal(SIGALRM, handler);
    alarm(1);
    for(;;) {
        pause();
        printf("pause return\n");
    }
  	return 0;
}

void handler(int sig) {
    printf("recv a sig=%d\n", sig);
    alarm(1);
    //sleep(1);
}
