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
return:��ȡ�ɹ��ֽ���
ssize_t:�з���
size_t:�޷���
 */


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


/*
param1:fd, param2:buf, param3:count
return:�Ѿ������˶���
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



//���׽ӿڽ�������,���ǲ��ӻ��������Ƴ�MSG_PEEK
//ֻҪ��͵�������ݾͽ��գ�û��ͷ������������
//�Է��׽ӿڹرգ�����0
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
	int nleft = maxline;
	char* bufp = (char*)buf;
	while(1) {
		//�ź��ж�����recv_peek�д���
		ret = recv_peek(sockfd, bufp, nleft);
		if(ret < 0)  
			return ret;
		if(0 == ret)  //��ʾ�Է��ر��׽ӿ� 
			return ret;

		nread = ret;  //ʵ��͵�������ֽ���
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
		nleft -= nread;  //����ʣ����
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
	
	//1.���⽩ʬ����
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
		//accept����ͬʱΪNULL; ����ͬʱ��ΪNULL
		if((conn = accept(listenfd, (struct sockaddr*)&peeraddr, &peerlen)) < 0)
			ERR_EXIT("accept");

		//ͨ��accept�õ��Զ�addr,port
		printf("ip=%s port=%d\n", inet_ntoa(peeraddr.sin_addr), ntohs(peeraddr.sin_port));
		
		struct sockaddr_in connaddr;
		socklen_t connlen = sizeof(connaddr);
		//ͨ��getpeername�õ��Զ�addr,port[conn�����������ӵ��׽���]
		//conn��������ip,port; Ҳ�����Զ�ip,port
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
