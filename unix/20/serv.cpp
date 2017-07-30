#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "pub.h"


#define ERR_EXIT(m) \
    do \
    { \
    	perror(m); \
    	exit(EXIT_FAILURE); \
    }while(0)


//�����ҳ�Ա�б�
USER_LIST client_list;

void do_login(MESSAGE& msg, int sock, struct sockaddr_in* cliaddr);
void do_logout(MESSAGE& msg, int sock, struct sockaddr_in* cliaddr);
void do_sendlist(int sock, struct sockaddr_in* cliaddr); 



void chat_srv(int sock) {
	struct sockaddr_in cliaddr;
	socklen_t clilen;
	int n;
	MESSAGE msg;
	while(1) {
		memset(&msg, 0, sizeof(msg));
		clilen = sizeof(cliaddr);
		n = recvfrom(sock, &msg, sizeof(msg), 0, (struct sockaddr*)&cliaddr, &clilen);
		if(n < 0) {
			if(errno == EINTR) 
				continue;
			ERR_EXIT("recvfrom");
		} 
		
		int cmd = ntohl(msg.cmd);
		switch(cmd) {
		case C2S_LOGIN:
			do_login(msg, sock, &cliaddr);
			break;
		case C2S_LOGOUT:
			do_logout(msg, sock, &cliaddr);
			break;
		case C2S_ONLINE_USER:
			do_sendlist(sock, &cliaddr);
			break;
		default:
			break;
		}
	}
}


void do_login(MESSAGE& msg, int sock, struct sockaddr_in* cliaddr) {
	USER_INFO user;
	strcpy(user.username, msg.body);
	user.ip = cliaddr->sin_addr.s_addr;
	user.port = cliaddr->sin_port;
	
	/*�����û�*/	
	USER_LIST::iterator it;
	for(it = client_list.begin(); it != client_list.end(); ++it) {
		if(strcmp(it->username, msg.body) == 0) {
			break;
		}
	}

	/*û�ҵ��û�*/
	if(it == client_list.end()) {
		printf("has a user login : %s <-> %s:%d\n", msg.body, inet_ntoa(cliaddr.sin_addr.s_addr);
		client_list.push_back(user);
		
		//��¼�ɹ�Ӧ��
		MESSAGE reply_msg;
		memset(&reply_msg, 0, sizeof(reply_msg);
		reply_msg.cmd = htonl(S2C_LOGIN_OK);
		sendto(sock, &reply_msg, sizeof(msg), 0, (struct sockaddr*)&cliaddr, sizeof(cliaddr));
		
		int count = htonl((int)client_list.size());
		//������������
		sendto(sock, &count, sizeof(int), 0, (struct sockaddr*)&cliaddr, sizeof(cliaddr));
		printf("sending user list information to: %s <-> %s:%d\n", msg.body, inet_ntoa(cliaddt.sin_addr.s_addr));
		//���������б�
		for(it = client_list.begin(); it != client_list.end(); ++it) {
		sendto(sock, &*it, sizeof(USER_INFO), 0, (struct sockaddr*)&cliaddr, sizeof(cliaddr));
		}
		//�������û�֪ͨ���û���¼
		for(it = client_list.begin(); it != client_list.end(); ++it) {
			if(strcmp(it->username, msg.body) == 0)
				continue;
			struct sockaddr_in peeraddr;
			memset(&peeraddr, 0, sizeof(peeraddr));
			peeraddr.sin_family = AF_INET;
			peeraddr.sin_port = it->ip;
			
			msg.cmd = htonl(S2C_SOMEONE_LOGIN);
			memcpy(msg.body, &user, sizeof(user));

			if(sendto(sock, &msg, sizeof(msg), 0, (Struct sockaddr*)&peeraddr, sizeof(peeraddr));
				ERR_EXIT("sendto");	
		}
	} else /*�ҵ��û�*/
	{
		printf("user %s has already logined\n", msg.body);
		
		MESSAGE reply_msg;
		memset(&reply_msg, 0, sizeof(reply_msg);
		reply_msg.cmd = htonl(S2C_ALREADY_LOGINED);
		sendto(sock, &reply_msg, sizeof(reply_msg), 0, (struct sockaddr*)&peeraddr, peerlen);
	}
}
void do_logout(MESSAGE& msg, int sock, struct sockaddr_in* cliaddr) {}
void do_sendlist(int sock, struct sockaddr_in* cliaddr) {} 


int main() {
	int sock;
	if((sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
		ERR_EXIT("socket");
	
	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(5188);
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	if((bind(sock, (struct sockaddr*)&servaddr, sizeof(servaddr))) < 0)
		ERR_EXIT("bind");
	
	chat_srv(sock);
	return 0;
}
