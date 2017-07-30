#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
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
    


void do_service(int conn) {
  	char recvbuf[1024];
  	while(1) {
  		memset(recvbuf, 0, sizeof(recvbuf));
  		int ret = read(conn, recvbuf, sizeof(recvbuf));
		if(0 == ret) {  //server端知道client关闭
			printf("client close\n");
			break;
		} else if(-1 == ret) {
			ERR_EXIT("read");
		}
  		fputs(recvbuf, stdout);
  		write(conn, recvbuf, ret);
  	}
}

void* thread_routine(void* arg) {
	//将线程设置分离状态，避免出现僵尸线程
	pthread_detach(pthread_self());
	int conn = *(int*)arg;
	free(arg);
	do_service(conn);
	printf("exiting thread ...\n");
	return NULL;
}

int main () {
  
	int listenfd;
	if(( listenfd= socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
  	//if((listenfd= socket(PF_INET, SOCK_STREAM, 0)) <0) 
  		ERR_EXIT("socket");
  	
  	
  	struct sockaddr_in servaddr;
  	memset(&servaddr, 0, sizeof(servaddr));
  	servaddr.sin_family = AF_INET;
  	servaddr.sin_port = htons(5188);
  	//servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  	servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
  	//inet_aton("127.0.0.1", &servaddr.sin_addr);
  
	int on = 1;
	if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
		ERR_EXIT("setsockopt");
	
  	if(bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr))<0)
  		ERR_EXIT("bind");
  		
  	if(listen(listenfd, SOMAXCONN) < 0)
  		ERR_EXIT("listen");
  	
  	struct sockaddr_in peeraddr;
  	socklen_t peerlen = sizeof(peeraddr);
  	int conn;

	while(1) {
		if((conn = accept(listenfd, (struct sockaddr*)&peeraddr, &peerlen)) < 0)
			ERR_EXIT("accept");

		printf("ip=%s port=%d\n", inet_ntoa(peeraddr.sin_addr), ntohs(peeraddr.sin_port));
		
		pthread_t tid;
		int ret;
		/*
 		1.if((ret = pthread_create(&tid, NULL, thread_routine, (void*)&conn)) != 0) {
		竞速问题,当一个连接,accept时候,上一个线程创建conn的值可能会被改变，地址改变
 		2.if((ret = pthread_create(&tid, NULL, thread_routine, (void*)conn)) != 0) {
		值传递,conn是4个字节,void*指针也是4个字节,平台移植不好
 		*/
		int* p = (int*)malloc(sizeof(int));
		*p = conn;
		if((ret = pthread_create(&tid, NULL, thread_routine, (void*)p)) != 0) {
			fprintf(stderr, "pthread_create:%s\n", strerror(ret));
			exit(EXIT_FAILURE);
		}

  	}


	
	close(listenfd);
	close(conn);
  return 0;
}
