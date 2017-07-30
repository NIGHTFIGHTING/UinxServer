
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
 * read timeout - 读超时检测函数，不含读操作
 * @fd:文件描述符
 * @wait_seconds:等待超时秒数,如果为0不检测超时
 * 成功(未超时) 返回0,失败返回-1,超时返回-1并且errno = ETIMEDOUT
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
 * write timeout - 写超时检测函数,不含写函数
 * @fd:文件描述符
 * @wait_seconds:等待超时秒数,如果为0不检测超时
 * 成功(未超时) 返回0,失败返回-1,超时返回-1并且errno = ETIMEDOUT
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
 * accept_timeout - 带超时的accept
 * @fd:套接字
 * @addr:输出参数，返回对方地址
 * @wait_seconds:等待超时秒数，如果为0表示正常模式
 * 成功(未超时)返回已连接套接字，超时返回-1并且errno = ETIMEDOUT
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
 * active_nonblock - 设置I/O为非阻塞模式
 * @fd:文件描述符
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
 * deactive_nonblock - 设置I/O为阻塞模式
 * @fd:文件描述符
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
 * @fd:套接字
 * @addr:要连接的地址
 * @wait_seconds:等待超时秒数，如果为0表示正常模式
 * 成功(未超时)返回0,失败返回-1,超时返回-1并且errno = ETIMEDOUT
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
			/*一旦连接建立，套接字就可写*/
			ret = select(fd+1, NULL, &connect_fdset, NULL, &timeout);
		} while(ret < 0 && errno == EINTR);
		
		if(0 == ret) {
			ret = -1;
			errno = ETIMEDOUT;
		} else if (ret <0) 
			return -1;
		else if (ret == 1) {
			/*ret返回为1,可能有两种情况,一种是连接建立成功,一种是套接字产生错误*/
			/*此事错误信息不会保存至errno变量中，因此，需要调用getsockopt来获取*/
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
	

