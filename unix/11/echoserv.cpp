#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>

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
    

/*
 param1:fd, param2:buf, param3:count
return:读取成功字节数
ssize_t:有符号
size_t:无符号
 */


ssize_t readn(int fd, void* buf, size_t count) {
	size_t nleft = count;
	ssize_t nread;  //已经读了
	char* bufp = (char*)buf;
	while(nleft > 0) {
		if((nread = read(fd, bufp, nleft)) < 0) {
			if(errno == EINTR) 
				continue;
			return -1;
		} else if (0 == nread) {
			return count - nleft;
		}
		bufp += nread;
		nleft -= nread;
		
	}
	return count;
}


/*
param1:fd, param2:buf, param3:count
return:已经发送了多少
*/
ssize_t writen(int fd, void* buf, size_t count) {
	size_t nleft = count;
	ssize_t nwrite;
	char* bufp = (char*)buf;
	while(nleft > 0) {
		if((nwrite = write(fd, bufp, nleft)) < 0) {	
			if(errno == EINTR)
				continue;
			return -1;
		} else if(0 == nwrite) {
			continue;
		}
		bufp += nwrite;
		nleft -= nwrite;
	}
	return count;
}



//从套接口接收数据,但是不从缓冲区中移除MSG_PEEK
//只要有偷看到数据就接收，没有头看到就是阻塞
//对方套接口关闭，返回0
ssize_t recv_peek(int sockfd, void* buf, size_t len) {
	while(1) {
		int ret = recv(sockfd, buf, len, MSG_PEEK);
		if(-1 == ret && errno == EINTR) 
			continue;
		return ret;
	}
}

//读取遇到\r\n截止，最大不能超过maxline
ssize_t readline(int sockfd, void* buf, size_t maxline) {
	int ret;
	int nread;
	int nleft = maxline;
	char* bufp = (char*)buf;
	while(1) {
		//信号中断已在recv_peek中处理
		ret = recv_peek(sockfd, bufp, nleft);
		if(ret < 0)  
			return ret;
		if(0 == ret)  //表示对方关闭套接口 
			return ret;

		nread = ret;  //实际偷看到的字节数
		int i;
		
		//该缓冲区中有\n，read读走
		for(i=0; i<nread; i++) {
			if(bufp[i] == '\n') {
				ret = readn(sockfd, bufp, i+1);  //包括\n都读走
				if(ret != i+1) 
					exit(EXIT_FAILURE);
				return ret;
			}
		}
	
		//没有\n，read先读走这部分，然后bufp偏移
		if(nread > nleft) 
			exit(EXIT_FAILURE);
		nleft -= nread;  //更新剩余量
		ret = readn(sockfd, bufp, nread); 
		if(ret != nread) 
			exit(EXIT_FAILURE);
		bufp += nread;	
	}
	return -1;
}

void echo_srv(int conn) {
  	char recvbuf[1024];
  	//struct packet recvbuf;
	int n;
  	while(1) {
  		memset(&recvbuf, 0, sizeof(recvbuf));
  		int ret = readline(conn, recvbuf, 1024);
		if(-1 == ret) {
			ERR_EXIT("readline");
		} else if(0 == ret) {
			printf("client close\n");
			break;
		}
  		fputs(recvbuf, stdout);
  		writen(conn, recvbuf, strlen(recvbuf));
  	}
}

void handle_sigchild(int sig) {
	//wait(NULL);
	//waitpid(-1, NULL, WNOHANG);
	while(waitpid(-1, NULL, WNOHANG) >0)
		;
}


int main () {
	
	//1.避免僵尸进程
	//signal(SIGCHLD, SIG_IGN);
	
//	signal(SIGCHLD, handle_sigchild);

  
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

	pid_t pid;
	while(1) {
		//accept必须同时为NULL; 或者同时不为NULL
		if((conn = accept(listenfd, (struct sockaddr*)&peeraddr, &peerlen)) < 0)
			ERR_EXIT("accept");

		//通过accept得到对端addr,port
		printf("ip=%s port=%d\n", inet_ntoa(peeraddr.sin_addr), ntohs(peeraddr.sin_port));
		
		struct sockaddr_in connaddr;
		socklen_t connlen = sizeof(connaddr);
		//通过getpeername得到对端addr,port[conn必须是已连接的套接字]
		//conn包含本段ip,port; 也包含对端ip,port
		getpeername(conn, (struct sockaddr*)&connaddr, &connlen);
		printf("ip=%s port=%d\n", inet_ntoa(connaddr.sin_addr), ntohs(connaddr.sin_port));
		
		
		pid = fork();
		if(-1 == pid) {
			ERR_EXIT("fork");
		}
		if(pid == 0) {  //child 
			close(listenfd);
			echo_srv(conn);
			exit(EXIT_SUCCESS);
		} else {	//parent
			close(conn);
		}
  	}


	
	close(listenfd);
	close(conn);
  return 0;
}
