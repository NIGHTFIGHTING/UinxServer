
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

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



/**
 * read timeout - ����ʱ��⺯��������������
 * @fd:�ļ�������
 * @wait_seconds:�ȴ���ʱ����,���Ϊ0����ⳬʱ
 * �ɹ�(δ��ʱ) ����0,ʧ�ܷ���-1,��ʱ����-1����errno = ETIMEDOUT
 */

int read_timeout(int fd, unsigned int wait_seconds) {
	int ret = 0;
	if(wait_seconds > 0) {
		fd_set read_fdset;
		struct timeval timeout;
		
		FD_ZERO(&read_fdset);
		FD_SET(fd, &read_fdset);
		
		timeout.tv_sec = wait_seconds;
		timeout.tv_usec = 0;

		do {
			ret = select(fd+1, &read_fdset, NULL, NULL, &timeout);
		} while(ret < 0 && errno == EINTR);

		if(0 == ret) {
			ret = -1;
			errno = ETIMEDOUT;
		} else if(1 == ret) {
			ret = 0;
		}
	}
	return ret;
}



/**
 * write timeout - д��ʱ��⺯��,����д����
 * @fd:�ļ�������
 * @wait_seconds:�ȴ���ʱ����,���Ϊ0����ⳬʱ
 * �ɹ�(δ��ʱ) ����0,ʧ�ܷ���-1,��ʱ����-1����errno = ETIMEDOUT
 */
int write_timeout(int fd, unsigned int wait_seconds) {
	int ret;
	if(wait_seconds > 0) {
		fd_set write_fdset;
		struct timeval timeout;
		
		FD_ZERO(&write_fdset);
		FD_SET(fd, &write_fdset);
		
		timeout.tv_sec = wait_seconds;
		timeout.tv_usec = 0;

		do {
			ret = select(fd+1, NULL, &write_fdset, NULL, &timeout);
		} while(ret < 0 && errno == EINTR);

		if(0 == ret) {
			ret = -1;
			errno = ETIMEDOUT;
		} else if(1 == ret) {
			ret = 0;
		}
	}
	return ret;
}


/**
 * accept_timeout - ����ʱ��accept
 * @fd:�׽���
 * @addr:������������ضԷ���ַ
 * @wait_seconds:�ȴ���ʱ���������Ϊ0��ʾ����ģʽ
 * �ɹ�(δ��ʱ)�����������׽��֣���ʱ����-1����errno = ETIMEDOUT
 */


int accept_timeout(int fd, struct sockaddr_in* addr, unsigned int wait_seconds) {
	int ret;
	socklen_t addrlen = sizeof(struct sockaddr_in);
	if(wait_seconds > 0) {
		fd_set accept_fdset;
		struct timeval timeout;
		
		FD_ZERO(&accept_fdset);
		FD_SET(fd, &accept_fdset);
		
		timeout.tv_sec = wait_seconds;
		timeout.tv_usec = 0;

		do {
			ret = select(fd+1, &accept_fdset, NULL, NULL, &timeout);
		} while(ret < 0 && errno == EINTR);
		
		if(-1 == ret) 
			return -1;
		else if(0 == ret) {
			return -1;
			errno = ETIMEDOUT;
		}
	}
	
	if(NULL != addr)
		ret = accept(fd, (struct sockaddr*)addr, &addrlen);
	else 
		ret = accept(fd, NULL, NULL);

	if(-1 == ret) 
		ERR_EXIT("accept");

	return ret;
}


/**
 * active_nonblock - ����I/OΪ������ģʽ
 * @fd:�ļ�������
 */
void active_nonblock(int fd) {
	int ret;
	int flags = fcntl(fd, F_GETFL);
	if(-1 == flags) 
		ERR_EXIT("fcntl");

	flags |= O_NONBLOCK;
	ret = fcntl(fd, F_SETFL, flags);
	if(-1 == ret)
		ERR_EXIT("fcntl");
}


/**
 * deactive_nonblock - ����I/OΪ����ģʽ
 * @fd:�ļ�������
 */
void deactive_nonblock(int fd) {
	int ret;
	int flags = fcntl(fd, F_GETFL);
	if(-1 == flags) 
		ERR_EXIT("fcntl");
	
	flags &= ~O_NONBLOCK;
	ret = fcntl(fd, F_SETFL, flags);
	if(ret == -1) 
		ERR_EXIT("fcntl");
}

/**
 * connect timeout - connect
 * @fd:�׽���
 * @addr:Ҫ���ӵĵ�ַ
 * @wait_seconds:�ȴ���ʱ���������Ϊ0��ʾ����ģʽ
 * �ɹ�(δ��ʱ)����0,ʧ�ܷ���-1,��ʱ����-1����errno = ETIMEDOUT
 */

int connect_timeout(int fd, struct sockaddr_in* addr, unsigned int wait_seconds) {
	int ret;
	socklen_t addrlen = sizeof(struct sockaddr_in);

	if(wait_seconds > 0) 
		active_nonblock(fd);

	ret = connect(fd, (struct sockaddr*)addr, addrlen);
	if(ret < 0 && errno == EINPROGRESS) {
		fd_set connect_fdset;
		struct timeval timeout;
		
		FD_ZERO(&connect_fdset);
		FD_SET(fd, &connect_fdset);
		
		timeout.tv_sec = wait_seconds;
		timeout.tv_usec = 0;

		do {
			/*һ�����ӽ������׽��־Ϳ�д*/
			ret = select(fd+1, NULL, &connect_fdset, NULL, &timeout);
		} while(ret < 0 && errno == EINTR);
		
		if(0 == ret) {
			ret = -1;
			errno = ETIMEDOUT;
		} else if (ret <0) 
			return -1;
		else if (ret == 1) {
			/*ret����Ϊ1,�������������,һ�������ӽ����ɹ�,һ�����׽��ֲ�������*/
			/*���´�����Ϣ���ᱣ����errno�����У���ˣ���Ҫ����getsockopt����ȡ*/
			int err;
			socklen_t socklen = sizeof(err);
			int sockoptret = getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &socklen);
			if(-1 == sockoptret) {
				return -1;
			}
			if(0 == err) 
				ret = 0;
			else {
				errno = err;	
				ret = -1;
			}
		}
	}
	
	if(wait_seconds > 0) {
		deactive_nonblock(fd);

	return ret;
	}
}
	

