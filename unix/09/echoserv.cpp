#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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

struct packet {
	int len;
	char buf[1024];
};


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

void do_service(int conn) {
  	//char recvbuf[1024];
  	struct packet recvbuf;
	int n;
  	while(1) {
  		memset(&recvbuf, 0, sizeof(recvbuf));
  		int ret = readn(conn, &recvbuf.len, 4);
		if(-1 == ret) {
			ERR_EXIT("read");
		} else if(4 > ret) {
			printf("client close\n");
			break;
		}
		n = ntohl(recvbuf.len);
		ret = readn(conn, recvbuf.buf, n);
		if(ret < n) {  //server端知道client关闭
			printf("client close\n");
			break;
		} else if(-1 == ret) {
			ERR_EXIT("read");
		}
  		fputs(recvbuf.buf, stdout);
  		writen(conn, &recvbuf, 4+n);
  	}
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

	pid_t pid;
	while(1) {
		if((conn = accept(listenfd, (struct sockaddr*)&peeraddr, &peerlen)) < 0)
			ERR_EXIT("accept");

		printf("ip=%s port=%d\n", inet_ntoa(peeraddr.sin_addr), ntohs(peeraddr.sin_port));
		
		pid = fork();
		if(-1 == pid) {
			ERR_EXIT("fork");
		}
		if(pid == 0) {  //child 
			close(listenfd);
			do_service(conn);
			exit(EXIT_SUCCESS);
		} else {	//parent
			close(conn);
		}
  	}


	
	close(listenfd);
	close(conn);
  return 0;
}
