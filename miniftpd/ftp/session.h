#ifndef __SESSION_H__
#define __SESSION_H__

#include "common.h"


typedef struct session
{
    //控制连接
    uid_t uid;
    int ctrl_fd;
    char cmdline[MAX_COMMAND_LINE];
    char cmd[MAX_COMMAND];
    char arg[MAX_ARG];

    //数据连接
    struct sockaddr_in *port_addr;    //保存PORT模式，主动模式下  客户端发送过来的 IP和 端口号
    int pasv_listen_fd; //PASV模式 ，保存 tcp_server创建的套接字
    int data_fd;  //PORT模式 ，服务器端 作为 客户端tcp_client创建sock-> connect 的绑定的 socket 
                  //PASV模式， 服务器端 tcp_server创建的 pasv_listen_fd ->accept之后的socket

    //父子进程通道
    int parent_fd;
    int child_fd;
    
    //FTP协议状态
    int is_ascii;
}session_t;


void begin_session(session_t * sess);


#endif //__SESSION_H__
