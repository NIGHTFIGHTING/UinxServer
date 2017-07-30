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
    




ssize_t readn(int fd, void* buf, size_t count) {
	size_t nleft = count;
	ssize_t nread;  //�Ѿ�����
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


ssize_t recv_peek(int sockfd, void* buf, size_t len) {
	while(1) {
		int ret = recv(sockfd, buf, len, MSG_PEEK);
		if(-1 == ret && errno == EINTR) 
			continue;
		return ret;
	}
}

//��ȡ����\r\n��ֹ������ܳ���maxline
ssize_t readline(int sockfd, void* buf, size_t maxline) {
	int ret;
	int nread;
	char* bufp = (char*)buf;
	int nleft = maxline;
	while(1) {
		ret = recv_peek(sockfd, bufp, nleft);
		if(ret < 0) //�ź��ж� 
			return ret;
		if(0 == ret)  //��ʾ�Է��ر��׽ӿ� 
			return ret;
		nread = ret;
		int i;
		
		//�û���������\n��read����
		for(i=0; i<nread; i++) {
			if(bufp[i] == '\n') {
				ret = readn(sockfd, bufp, i+1);  //����\n������
				if(ret != i+1) 
					exit(EXIT_FAILURE);
				return ret;
			}
		}
	
		//û��\n��read�ȶ����ⲿ�֣�Ȼ��bufpƫ��
		if(nread > nleft) 
			exit(EXIT_FAILURE);
		nleft -= nread;
		ret = readn(sockfd, bufp, nread); 
		if(ret != nread) 
			exit(EXIT_FAILURE);
		bufp += nread;	
	}
	return -1;
}

void echo_cli(int sock) {
  	char sendbuf[1024] = {0};	
  	char recvbuf[1024] = {0};
  	int n;
  	while(fgets(sendbuf, sizeof(sendbuf), stdin) != NULL) {
		//������\n��buf���͹�ȥ
  		//writen(sock, sendbuf, strlen(sendbuf));
  	
		//TCPЭ��ջ�յ�FIN����	
		writen(sock, sendbuf, 1);
		//TCPЭ��ջ�յ�RST����
		writen(sock, sendbuf+1, strlen(sendbuf)-1);
		//�ڶ��η��ͺ󣬻����SIGPIPE�ź�
			
  		int ret = readline(sock, recvbuf, sizeof(recvbuf));
		if(-1 == ret) {
			ERR_EXIT("readline");
		} else if(0 == ret) {
			printf("client close\n");
			break;
		}
	
  		fputs(recvbuf, stdout);
  		memset(sendbuf, 0, sizeof(sendbuf));
  		memset(recvbuf, 0, sizeof(recvbuf));
  	}
  	close(sock);
}


void handle_sigpipe(int sig) {
	printf("recv a sig=%d\n", sig); 
}

int main () {
		
	//signal(SIGPIPE, handle_sigpipe);
	signal(SIGPIPE, SIG_IGN);  
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
  	
  	
	//��ñ���ip,port
	struct sockaddr_in localaddr;
	socklen_t addrlen= sizeof(localaddr);
	if(getsockname(sock,(struct sockaddr*)&localaddr, &addrlen) < 0)
		ERR_EXIT("getsockname");
	printf("ip=%s port=%d\n", inet_ntoa(localaddr.sin_addr), ntohs(localaddr.sin_port));

	
	struct sockaddr_in connaddr;
	socklen_t connlen = sizeof(connaddr);
	//ͨ��getpeername�õ��Զ�addr,port[conn�����������ӵ��׽���]
	getpeername(sock, (struct sockaddr*)&connaddr, &connlen);
	printf("ip=%s port=%d\n", inet_ntoa(connaddr.sin_addr), ntohs(connaddr.sin_port));


  	echo_cli(sock);
	
  	return 0;
}
