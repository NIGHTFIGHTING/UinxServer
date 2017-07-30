#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <signal.h>



#define ERR_EXIT(m) \
    do \
    { \
    	perror(m); \
    	exit(EXIT_FAILURE); \
    }while(0)
    

void handler(int sig) {
	printf("recv a sig=%d\n", sig);
	exit(EXIT_SUCCESS);
}

int main () {
  
	int sock;
	if(( sock= socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
  		ERR_EXIT("socket");
  	
  	sock = socket(PF_INET, SOCK_STREAM, 0);
  	
  	struct sockaddr_in servaddr;
  	memset(&servaddr, 0, sizeof(servaddr));
  	servaddr.sin_family = AF_INET;
  	servaddr.sin_port = htons(5188);
  	servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
  	
  	if(connect(sock, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
  		ERR_EXIT("connect");
  	
	pid_t pid;
	pid = fork();
	
	if(-1 == pid) {
		ERR_EXIT("fork");
	}
	if(0 == pid) {
		char recvbuf[1024] = {0};
		while(1) {
			memset(recvbuf, 0, sizeof(recvbuf));
			int ret = read(sock, recvbuf, sizeof(recvbuf));
			if(-1 == ret) {
				ERR_EXIT("read");
			} else if(0 == ret) {
				printf("peer close\n");
				break;
			}
			fputs(recvbuf, stdout);
		}
		close(sock);
		kill(getppid(), SIGUSR1);
		exit(EXIT_SUCCESS);
	} else {
		signal(SIGUSR1, handler);
		char sendbuf[1024] = {0};	
		while(fgets(sendbuf, sizeof(sendbuf), stdin) != NULL) {
			write(sock, sendbuf, strlen(sendbuf));
			memset(sendbuf, 0, sizeof(sendbuf));
		}
		close(sock);
		exit(EXIT_SUCCESS);
	}
  	
  	
  	return 0;
}
