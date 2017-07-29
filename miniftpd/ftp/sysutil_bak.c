#include "sysutil.h"

int getlocalip(char *ip)
{
	char host[100] = {0};
	if(gethostname(host, sizeof(host)) < 0)
		return -1;
	struct hostent *hp;
	if((hp = gethostbyname(host)) == NULL)
		return -1;
	strcpy(ip, inet_ntoa(*(struct in_addr*)hp->h_addr));
	return 0;
}

int tcp_client(unsigned short port)
{
    int sock;
    if((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
        ERR_EXIT("tcp_client");
    if(port >0)
    {
        int on = 1;
        if((setsockopt(sock , SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on))) < 0)
            ERR_EXIT("setsockopt");

        char ip[16] = {0};
        getlocalip(ip);
        struct sockaddr_in localaddr;
        memset(&localaddr, 0, sizeof(localaddr));
        localaddr.sin_family = AF_INET;
        localaddr.sin_port = htons(port);
        localaddr.sin_addr.s_addr = inet_addr(ip);

        if(bind(sock, (struct sockaddr*)&localaddr, sizeof(localaddr)) < 0)
            ERR_EXIT("bind");
    }
    return sock;
}



/*
*@tcp_server - ����tcp������
*@host:������IP��ַ���߷�����������
*@port���������˿�
*�ɹ����ؼ����׽���
*/

int tcp_server(const char *host,unsigned short port)
{
	int listenfd;
	if((listenfd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{
		ERR_EXIT("tcp_server");
	}
	struct sockaddr_in servaddr;
	memset(&servaddr, 0,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	if(host != NULL)
	{
		if(inet_aton(host, &servaddr.sin_addr) == 0)
		{
			struct hostent* hp;
			hp = gethostbyname(host);
			if(hp == NULL)
				ERR_EXIT("gethostbyname");
			servaddr.sin_addr = *(struct in_addr*)hp->h_addr;
		}
	}
	else
		servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(port);

	int on =1;
	if((setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,(const char *)&on,sizeof(on))) < 0)
	{
		ERR_EXIT("setsockopt");
	}
	if(bind(listenfd,(struct sockaddr*)&servaddr,sizeof(servaddr)) < 0)
	{
		ERR_EXIT("bind");
	}
	
	if(listen(listenfd, SOMAXCONN) < 0)
	{
		ERR_EXIT("listenfd");
	}
	return listenfd;
}


/**
*activate_nonblock - ����I/OΪ������ģʽ
* @fd���ļ�������
*/
void activate_nonblock(int fd)
{
    int ret ;
    int flags = fcntl(fd,F_GETFL);
    if(flags == -1)
        ERR_EXIT("fcntl");
    flags |= O_NONBLOCK;
    ret = fcntl(fd, F_SETFL, flags);
    if(ret == -1)
        ERR_EXIT("fcntl");
}

/**
 * deactivate_nonbloc - ���� I/OΪ����ģʽ
 * @fd���ļ�������
 */
void deactivate_nonblock(int fd)
{
    int ret;
    int flags = fcntl(fd, F_GETFL);
    if(flags == -1)
        ERR_EXIT("fcntl");
    flags &= ~O_NONBLOCK;
    ret = fcntl(fd, F_SETFL, flags);
    if(ret == -1)
        ERR_EXIT("fcntl");
}


/**
 * read_timeout - ����ʱ��⺯��������������
 * @fd���ļ�������
 * @wait_seconds���ȴ���ʱ���������Ϊ0��ʾ����ⳬʱ
 * �ɹ���δ��ʱ������0��ʧ�ܷ���-1����ʱ����-1����errno = ETIMEOUT
 */


int read_timeout(int fd, unsigned int wait_seconds)
{
    int ret = 0;
    if(wait_seconds > 0)
    {
        fd_set read_fdset;
        struct timeval timeout;

        FD_ZERO(&read_fdset);
        FD_SET(fd, &read_fdset);

        timeout.tv_sec = wait_seconds;
        timeout.tv_usec = 0;

        do
        {
            ret = select(fd+1, &read_fdset, NULL, NULL, &timeout);
        }while(ret < 0 && errno == EINTR/*�ź��ж�*/);

        if(ret == 0)
        {
            ret = -1;
            errno = ETIME;
        }
        else if(ret == 1)
            ret = 0;
    }
    return ret;
}

/**
 * write_timeout - д��ʱ��⺯��������д����
 *  @fd���ļ�������
 *  @wait_seconds���ȴ���ʱ���������Ϊ0��ʾ����ⳬʱ
 *  �ɹ���δ��ʱ������0��ʧ�ܷ���-1����ʱ����-1����errno = ETIMEOUT
 */
int write_timeout(int fd,unsigned int wait_seconds)
{
    int ret = 0;
    if(wait_seconds > 0)
    {
        fd_set write_fdset;
        struct timeval timeout;

        FD_ZERO(&write_fdset);
        FD_SET(fd,&write_fdset);

        timeout.tv_sec = wait_seconds;
        timeout.tv_usec= 0;
        do
        {
            ret = select(fd+1, NULL, NULL, &write_fdset, &timeout);
        }while(ret < 0 && errno == EINTR);

        if(ret == 0)
        {
            ret = -1;
            errno = ETIME;
        }
        else if(ret == 1)
            ret = 0;
    }
    return ret;
}


/**
 * accept_timeout - ����ʱ��accept
 * @fd�׽���
 * @addr��������������ضѷŵ�ַ
 * @wait_seconds���ȴ���ʱ���� �����Ϊ0��ʾ����ģʽ
 * �ɹ���δ��ʱ�������������׽��֣���ʱ����-1����errno = ETIMEOUT
 */
int accept_timeout(int fd, struct sockaddr_in *addr,unsigned int wait_seconds)
{
	int ret;
	socklen_t addrlen = sizeof(struct sockaddr_in);
	if(wait_seconds > 0)
	{
		fd_set accept_fdset;
		struct timeval timeout;
		FD_ZERO(&accept_fdset);
		FD_SET(fd,&accept_fdset);
		timeout.tv_sec = wait_seconds;
		timeout.tv_usec = 0;
		do
		{
			ret = select(fd+1, &accept_fdset, NULL, NULL, &timeout);
		}while (ret < 0 && errno == EINTR);
		if(ret == -1)
			return -1;
		else if(ret == 0)
		{
			errno = ETIMEDOUT;
			return -1;
		}
	}
	if(addr != NULL)
		ret = accept(fd,(struct sockaddr*)addr, &addrlen);
	else
		ret = accept(fd, NULL, NULL);
	return ret;
}

/*
int connect_timeout(int fd,struct sockaddr_in *addr,unsigned int wait_seconds)
{
    int ret ;
    socklen_t addrlen = sizeof(struct sockaddr_in);

    if(wait_seconds > 0)
        activate_nonblock(fd);
    ret = connect(fd, (struct sockaddr*)addr, addrlen);
    if(ret < 0 && errno == EINPROGRESS)
    {
        fd_set connect_fdset;
        struct timeval timeout;

        FD_ZERO(&connect_fdset);
        FD_SET(fd, &connect_fdset);

        timeout.tv_sec = wait_seconds;
        timeout.tv_usec = 0;

        do
        {
            //һ�����ӽ������׽��־Ϳ�д
            ret = select(fd+1, NULL, &connect_fdset, NULL,&timeout);
        }while(ret < 0 && errno == EINTR);
        if(ret == 0)
        {
            ret = -1;
            errno = ETIME;
        }
        else if(ret < 0)
            return -1;
        else if(ret == 1)
        {
            //ret����Ϊ1�����������������һ�������ӽ����ɹ���һ�����׽��ֲ�������
            //��ʱ������Ϣ���ᱣ����errno�����У���ˣ���Ҫ����getsockopt����ȡ
            int err;
            socklen_t socklen = sizeof(err);
            int sockoptret = getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &socklen);
            if(sockoptret == -1)
            {
                return -1;
            }
            if(err == 0)
                ret = 0;
            else
            {
                errno = err;
                ret = -1;
            }
        }
    }
    if(wait_seconds > 0)
    {
        deactivate_nonblock(fd);
    }
    return ret;
}*/
/**
 * connect_timeout - connect
 * @fd: �׽���
 * @addr: Ҫ���ӵĶԷ���ַ
 * @wait_seconds: �ȴ���ʱ���������Ϊ0��ʾ����ģʽ
 * �ɹ���δ��ʱ������0��ʧ�ܷ���-1����ʱ����-1����errno = ETIMEDOUT
 */
int connect_timeout(int fd, struct sockaddr_in *addr, unsigned int wait_seconds)
{
    int ret;
    socklen_t addrlen = sizeof(struct sockaddr_in);

    if (wait_seconds > 0)
        activate_nonblock(fd);

    ret = connect(fd, (struct sockaddr*)addr, addrlen);
    if (ret < 0 && errno == EINPROGRESS)
    {
        printf("AAAAA\n");
        fd_set connect_fdset;
        struct timeval timeout;
        FD_ZERO(&connect_fdset);
        FD_SET(fd, &connect_fdset);
        timeout.tv_sec = wait_seconds;
        timeout.tv_usec = 0;
        do
        {
            /* һ�����ӽ������׽��־Ϳ�д */
            ret = select(fd + 1, NULL, &connect_fdset, NULL, &timeout);
        } while (ret < 0 && errno == EINTR);
        if (ret == 0)
        {
            ret = -1;
            errno = ETIMEDOUT;
        }
        else if (ret < 0)
            return -1;
        else if (ret == 1)
        {
            printf("BBBBB\n");
            /* ret����Ϊ1�����������������һ�������ӽ����ɹ���һ�����׽��ֲ�������*/
            /* ��ʱ������Ϣ���ᱣ����errno�����У���ˣ���Ҫ����getsockopt����ȡ�� */
            int err;
            socklen_t socklen = sizeof(err);
            int sockoptret = getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &socklen);
            if (sockoptret == -1)
            {
                return -1;
            }
            if (err == 0)
            {
                printf("DDDDDDD\n");
                ret = 0;
            }
            else
            {
                printf("CCCCCC\n");
                errno = err;
                ret = -1;
            }
        }
    }
    if (wait_seconds > 0)
    {
        deactivate_nonblock(fd);
    }
    return ret;
}



ssize_t readn(int fd, void *buf, size_t count)
{
    size_t nleft = count;/*ʣ��Ľ��յ�*/
    ssize_t nread;/*�ѽ��յ�*/
    char *bufp = (char*)buf;
    while(nleft > 0)
    {
        if((nread = read(fd, bufp,nleft))<0)
        {
            if(errno == EINTR)/*�ź��ж�*/
            {
                continue;
            }
            else
            {
                return -1;
            }
        }
        else if(nread == 0)
        {
            return count - nleft;
        }
        else /*nread > 0*/
        {
            bufp += nread;
            nleft -= nread;
        }
    }

    return count;
}


ssize_t writen(int fd, const void *buf, size_t count)
{
    size_t nleft = count;/*ʣ�෢�͵�*/
    size_t nwriten;
    char *bufp = (char*)buf;
    while(nleft > 0)
    {
        if((nwriten = write(fd,bufp,nleft)) < 0)
        {
            if(errno == EINTR)
            {
                continue;
            }
            else
            {
                return -1;
            }
        }
        else if(nwriten == 0)
        {
            continue;
        }
        else
        {
            bufp += nwriten;
            nleft -= nwriten;
        }
    }
    return count;
}
ssize_t recv_peek(int sockfd,void *buf, size_t count)
{
    while(1)
    {
        //ֻ�Ǵ��׽ӿڻ����� �������ݵ� buf �� ����û���Ƴ�
        int ret = recv(sockfd,buf,count,MSG_PEEK);
        if(ret == -1 && errno == EINTR)
        {
            continue;
        }
        else
        {
            return ret;
        }
    }
}

/*һ��һ���ַ��Ķ�ȡ���ݣ�����Ч�ʱȽϵͣ���Ϊ ��ε��� recv /read����
 ��̬����Ϊ���յ����ݽ��л��棬��һ�ν��յ�ʱ�򣬴ӻ����п������Ƿ���\r\n,��������ĺ���*/
ssize_t readline(int sockfd,void *buf, size_t maxline)
{
    int ret;
    int nread;
    char *bufp = (char*)buf;
    int nleft = maxline;
    while(1)
    {
        ret = recv_peek(sockfd, bufp,nleft);
        if(ret < 0)
        {
            return ret;
        }
        else if(ret == 0)
        {
            return ret;
        }
        else
        {
            nread = ret;
            int i;
            for(i=0; i<nread; i++)
            {
                if(bufp[i] == '\n')
                {
                    ret = readn(sockfd,bufp,i+1);
                    if(ret != i+1)
                    {
                        exit(EXIT_FAILURE);
                    }
                    else
                    {
                        return ret;
                    }
                }
            }
            if(nread > nleft)
                exit(EXIT_FAILURE);
            nleft -= nread;
            ret = readn(sockfd, buf, nread);
            if(ret != nread)
                exit(EXIT_FAILURE);
            bufp += nread;

        }
    }
    return -1;
}

void send_fd(int sock_fd, int fd)
{
}
int recv_fd(const int sock_fd)
{
    return 0;
}


