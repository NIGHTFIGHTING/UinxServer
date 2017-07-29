#include "common.h"
#include "sysutil.h"
#include "session.h"
#include "tunable.h"
#include "parseconf.h"
#include "ftpproto.h"

int main(void )
{
	if(getuid() != 0) //判断是否由root启动
	{
		fprintf(stderr,"miniftpd: must be started as root\n");
		exit(EXIT_FAILURE);
	}


    //list_common();

    parseconf_load_file(MINIFTP_CONF);
    printf("tunable_pasv_enable=%d\n", tunable_pasv_enable);
    printf("tunable_port_enable=%d\n", tunable_port_enable);
    printf("tunable_listen_port=%u\n", tunable_listen_port);
    printf("tunable_max_clients=%u\n", tunable_max_clients);
    printf("tunable_max_per_ip=%u\n", tunable_max_per_ip);
    printf("tunable_accept_timeout=%u\n", tunable_accept_timeout);
    printf("tunable_connect_tineout=%u\n", tunable_connect_timeout);
    printf("tunable_idle_session_timeout=%u\n", tunable_idle_session_timeout);
    printf("tunable_data_connection_timeout=%u\n", tunable_data_connection_timeout);
    printf("tunable_local_umask=0%o\n", tunable_local_umask);
    printf("tunable_upload_max_rate=%u\n", tunable_upload_max_rate);
    printf("tunable_dowload_max_rate=%u\n", tunable_download_max_rate);

    if( tunable_listen_address != NULL)
    {
        printf("tunalbe_listen_address=%s\n", tunable_listen_address);
    }
    else
    {
        printf("tunalbe_listen_address=NULL\n");
    }




    //创建会话
    session_t sess = 
    {
        /*控制连接*/
        0,-1,"","","",
        /*数据连接*/
        NULL,-1,-1,
        /*父子进程间通道*/
        -1,-1,
        /*FTP协议状态*/
        0
    };

    signal(SIGCHLD, SIG_IGN);
    int listenfd = tcp_server(tunable_listen_address/*NULL*/, 5188);
    int conn;
    pid_t pid;

    while(1)
    {
        //采用select模型进行超时检测
        conn = accept_timeout(listenfd, NULL, 0 );  //接收客户算连接
        if(conn == -1)
        {
            ERR_EXIT("accept_timeout");
        }
        pid = fork();
        if(pid == -1)
            ERR_EXIT("fork");
        if(pid == 0)   //子进程  开启 一个 会话 
        {
            close(listenfd);
            sess.ctrl_fd = conn;  
            begin_session(&sess);   
            //开启一个会话 ，两个 进程 父进程 是 nobody 进程 ，子进程 是 服务进程

        }
        else
            close(conn);
    }
	return 0;
}
