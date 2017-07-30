#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>


#define ERR_EXIT(m) \
    do \
    { \
    	perror(m); \
    	exit(EXIT_FAILURE); \
    }while(0)

int main(void) {
	int msgid;
	msgid = msgget(1234, 0); //按照原来创造的权限打开
	
	if(msgid == -1) 
		ERR_EXIT("msgget");
	printf("msgget succ\n");
	printf("msgid=%d\n", msgid);

	msgctl(msgid, IPC_RMID, NULL);  
	return 0;
}
