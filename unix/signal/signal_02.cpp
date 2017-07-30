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
    signal(SIGINT, handler);
    if(signal(SIGUSR1, handler) == SIG_ERR)
        ERR_EXIT("signal error");
    pid_t pid = fork();
    if(-1 == pid) 
        ERR_EXIT("fork error");

    if(0 == pid) {
        pid = getpgrp();
        //kill(-pid, SIGUSR1);  //
        killpg(getpgrp(), SIGUSR1);
        //kill(getppid(), SIGUSR1);
        exit(EXIT_SUCCESS);
    }

    int n = 15;
    do {
        n = sleep(n);
    }while(n>0);
  	return 0;
}

void handler(int sig) {
    printf("recv a sig=%d\n", sig);
}
