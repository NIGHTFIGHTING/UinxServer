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
	msgid = msgget(1234, 0666 | IPC_CREAT);
	//msgid = msgget(1234, 0666 | IPC_CREAT | IPC_EXCL); //�ڶ��δ�����ʧ��
	//msgid = msgget(IPC_PRIVATE, 0666 | IPC_CREAT | IPC_EXCL); //ÿһ�ζ��ᴴ��һ���µ�
	//msgid = msgget(IPC_PRIVATE, 0666);
	//msgid = msgget(1234, 0666 | IPC_CREAT);
	msgid = msgget(1234, 0); //����ԭ�������Ȩ�޴�
	
	if(msgid == -1) 
		ERR_EXIT("msgget");
	printf("msgget succ\n");
	printf("msgid=%d\n", msgid);
	return 0;
}
