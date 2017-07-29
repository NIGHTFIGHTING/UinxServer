#include "common.h"
#include "session.h"
#include "privateparent.h"
#include "ftpproto.h"
#include "privsock.h"


void begin_session(session_t *sess)
{

    /*int sockfds[2];
    if(socketpair(PF_UNIX, SOCK_STREAM, 0, sockfds) < 0)
    {
        ERR_EXIT("socketpair");
    }*/


    priv_sock_init(sess);

    //父子进程间 通信 采用 socketpair
    pid_t pid;
    pid = fork();
    if(pid < 0) 
        ERR_EXIT("fork");
    if(pid == 0)
    {
        //子进程 ftp服务进程
        /*
        close(sockfds[0]);
        sess->child_fd = sockfds[1];
        */
        priv_sock_set_child_context(sess);
        handle_child(sess);
    }
    else
    {
        //nobody 进程 父进程
        //close(sockfds[1]);
        //sess->parent_fd = sockfds[0];
        priv_sock_set_parent_context(sess);
        handle_parent(sess);
    }
}
