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
void printsigset(sigset_t* set) {
    int i;
    for(i=1; i<NSIG; ++i) {
        if(sigismember(set, i)) {
	    putchar('1');
	} else {
	    putchar('0');
	}
    }
    printf("\n");
}


int main () {
    sigset_t pset;
    sigset_t bset;
    //×èÈûSIGINTÐÅºÅ
    sigemptyset(&bset);
    sigaddset(&bset, SIGINT);
    //sigaddset(&bset, SIGRTMIN);

    signal(SIGINT, handler);
    //signal(SIGRTMIN, handler);
    signal(SIGQUIT, handler);

    sigprocmask(SIG_BLOCK, &bset, NULL);

    for(;;) {
        sigpending(&pset);
	printsigset(&pset);
	sleep(2);
    }
  	return 0;
}

void handler(int sig) {
    if(sig == SIGINT || SIGRTMIN) {
	    printf("recv a sig=%d\n", sig);
    } else if(sig == SIGQUIT) {
    	sigset_t uset;
	sigemptyset(&uset);
	sigaddset(&uset, SIGINT);
	//sigaddset(&uset, SIGRTMIN);
	sigprocmask(SIG_UNBLOCK, &uset, NULL);
    }
}
