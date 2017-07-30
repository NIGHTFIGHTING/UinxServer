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
    



int main () {
	int count = 0;
	while(1) {	
		
		//signal(SIGPIPE, handle_sigpipe);
		signal(SIGPIPE, SIG_IGN);  
		int sock;
		if(( sock= socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
			ERR_EXIT("socket");
			sleep(4);
		}
		
		sock = socket(PF_INET, SOCK_STREAM, 0);
		
		struct sockaddr_in servaddr;
		memset(&servaddr, 0, sizeof(servaddr));
		servaddr.sin_family = AF_INET;
		servaddr.sin_port = htons(5188);
		servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
		
		if(connect(sock, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
			ERR_EXIT("sockect");
		
		
		//获得本端ip,port
		struct sockaddr_in localaddr;
		socklen_t addrlen= sizeof(localaddr);
		if(getsockname(sock,(struct sockaddr*)&localaddr, &addrlen) < 0)
			ERR_EXIT("getsockname");
		printf("ip=%s port=%d\n", inet_ntoa(localaddr.sin_addr), ntohs(localaddr.sin_port));

		
		struct sockaddr_in connaddr;
		socklen_t connlen = sizeof(connaddr);
		//通过getpeername得到对端addr,port[conn必须是已连接的套接字]
		getpeername(sock, (struct sockaddr*)&connaddr, &connlen);
		printf("ip=%s port=%d\n", inet_ntoa(connaddr.sin_addr), ntohs(connaddr.sin_port));
		printf("count = %d\n", ++count);
	}


	
  	return 0;
}
