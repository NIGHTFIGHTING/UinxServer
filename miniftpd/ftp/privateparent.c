#include "privateparent.h"
#include "privsock.h"
#include "sysutil.h"
#include "tunable.h"


static void privop_pasv_get_data_sock(session_t *sess);
static void privop_pasv_active(session_t *sess);
static void privop_pasv_listen(session_t *sess);
static void privop_pasv_accept(session_t *sess);

int capset(cap_user_header_t hdrp, const cap_user_data_t datap)
{
    return syscall(__NR_capset, hdrp, datap);
}


//给nobody进程bind特权
void minimize_privilege(void )
{
	//struct passwd *pw = getpwnam("nobody");
    struct passwd *pw = getpwnam("liuqi");
    if(pw == NULL)
       return ;
    //将父进程 改为 nobody 进程 没有改之前 gid和 uid都是 0 （以root权限启动的）
    if(setgid(pw->pw_gid) < 0)  //一定先改 gid  ,如果先改 uid 可能 就没有权限 改 gid了
    {   
        ERR_EXIT("setgid");     
    }
    if(setuid(pw->pw_uid) < 0)  
    {
        ERR_EXIT("setuid");
    }

    struct __user_cap_header_struct cap_header;
    struct __user_cap_data_struct cap_data;

    memset(&cap_header, 0, sizeof(cap_header));
    memset(&cap_data, 0, sizeof(cap_data));

    cap_header.version =  _LINUX_CAPABILITY_VERSION_2;//版本 32系统
    cap_header.pid = 0;

    __u32 cap_mask = 0;
    cap_mask |= (1 << CAP_NET_BIND_SERVICE);

    //应该具有的capability
    cap_data.effective = cap_data.permitted = cap_mask;
    cap_data.inheritable = 0; //不允许被继承

    capset(&cap_header, &cap_data);
}


void handle_parent(session_t *sess)
{
   
    minimize_privilege();

    char cmd;
    while(1)
    {
        //read(sess->parent_fd, &cmd, 1);
        //nobody进程接收 ftp服务进程的命令
        cmd = priv_sock_get_cmd(sess->parent_fd);
        //解析内部命令
        //处理内部命令
        switch(cmd)
        {
            case PRIV_SOCK_GET_DATA_SOCK:
                privop_pasv_get_data_sock(sess);
                break;
            case PRIV_SOCK_PASV_ACTIVE:
                privop_pasv_active(sess);
                break;
            case PRIV_SOCK_PASV_LISTEN:
                privop_pasv_listen(sess);
                break;
            case PRIV_SOCK_PASV_ACCEPT:
                privop_pasv_accept(sess);
                break;
        }
    }
}



static void privop_pasv_get_data_sock(session_t *sess)
{
    /*nobody 进程接收 PRIV_SOCK_GET_DATA_SOCK命令
    进一步 接收 一个整数 ，也就是 port
    接收一个 字符串， 也就是 ip
    socket
    bind(20)
    connect(ip,port);*/
    unsigned short port = (unsigned short)priv_sock_get_int(sess->parent_fd);
    char ip[16] = {0};
    priv_sock_recv_buf(sess->parent_fd, ip, sizeof(ip));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr= inet_addr(ip);

    int fd = tcp_client(20);
    if(fd == -1)
    {
        priv_sock_send_result(sess->parent_fd, PRIV_SOCK_RESULT_BAD);  
        return ;
    }
    if(connect_timeout(fd, &addr, tunable_connect_timeout) < 0)
    {
        close(fd);
        priv_sock_send_result(sess->parent_fd, PRIV_SOCK_RESULT_BAD);
    }
    priv_sock_send_result(sess->parent_fd, PRIV_SOCK_RESULT_OK);
    priv_sock_send_fd(sess->parent_fd, fd);
    close(fd);

}


//通过 判断 当前的 listen 套接字 是否 为 -1 ，--->  判断 是否处于 PASV被动模式
static void privop_pasv_active(session_t *sess)
{
    int active = 1;
    if(sess->pasv_listen_fd != -1)
    {
        active = 1;
    }
    else
    {
        active = 0;
    }
    priv_sock_send_int(sess->parent_fd, active);
}


//被动模式PASV ： ftp服务进程 请求得到 一个listen 套接字
static void privop_pasv_listen(session_t *sess)
{
    char ip[16] = {0};
    getlocalip(ip);

    sess->pasv_listen_fd = tcp_server(ip,0);
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    if(getsockname(sess->pasv_listen_fd, (struct sockaddr*)&addr, &addrlen) < 0)
    {
        ERR_EXIT("getsockname");
    }
    unsigned short port = ntohs(addr.sin_port);

    //把listen套接字的 端口号 发送过去
    priv_sock_send_int(sess->parent_fd, (int)port);

}

//PRIV_SOCK_PASV_ACCEPT
static void privop_pasv_accept(session_t *sess)
{
    int fd = accept_timeout(sess->pasv_listen_fd, NULL, tunable_accept_timeout);
    close(sess->pasv_listen_fd);//关闭 监听套接字
    sess->pasv_listen_fd = -1;


    if(fd == -1)
    {
        priv_sock_send_result(sess->child_fd, PRIV_SOCK_RESULT_BAD);
        return ;
    }
    priv_sock_send_result(sess->child_fd, PRIV_SOCK_RESULT_OK);
    priv_sock_send_fd(sess->parent_fd, fd);
    close(fd);//这个套接字在nobody进程中没用了
}
