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
  		ERR_EXIT("sockect");
  	
  	
	struct packet sendbuf;
	struct packet recvbuf;
	memset(&sendbuf, 0, sizeof(sendbuf));
	memset(&recvbuf, 0, sizeof(recvbuf));
  	//char sendbuf[1024] = {0};	
  	//char recvbuf[1024] = {0};
  	int n;
  	while(fgets(sendbuf.buf, sizeof(sendbuf.buf), stdin) != NULL) {
		n = strlen(sendbuf.buf);
		sendbuf.len = htonl(n);  //注意主机字节序-->网络字节序 
  		writen(sock, &sendbuf, 4+n);  //定制协议
  		//writen(sock, sendbuf, sizeof(sendbuf));  //发送定长包
  		//writen(sock, sendbuf, strlen(sendbuf));
  		
			
  		int ret = readn(sock, &recvbuf.len, 4);
		if(-1 == ret) {
			ERR_EXIT("read");
		} else if(4 > ret) {
			printf("client close\n");
			break;
		}
		n = ntohl(recvbuf.len);
		ret = readn(sock, recvbuf.buf, n);
		if(ret < n) {  //server端知道client关闭
			printf("client close\n");
			break;
		} else if(-1 == ret) {
			ERR_EXIT("read");
		}
	
		//readn(sock, recvbuf, sizeof(recvbuf));
  		fputs(recvbuf.buf, stdout);
  		memset(&sendbuf, 0, sizeof(sendbuf));
  		memset(&recvbuf, 0, sizeof(recvbuf));
  	}
  	
  	close(sock);
  	return 0;
}
