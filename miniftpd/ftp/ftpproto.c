#include "tunable.h"
#include "ftpcodes.h"
#include "ftpproto.h"
#include "sysutil.h"
#include "str.h"
#include "privsock.h"


void ftp_reply(session_t *sess, int status, const char *text);
void ftp_lreply(session_t *sess, int status, const char *text);

int port_active(session_t *sess);
int pasv_active(session_t *sess); 

int list_common(session_t *sess, int detail);
int get_transfer_fd(session_t *sess);
int get_port_fd(session_t *sess);
int get_pasv_fd(session_t *sess);


static void do_user(session_t *sess);
static void do_pass(session_t *sess);
static void do_cwd(session_t *sess);
static void do_cdup(session_t *sess);
static void do_quit(session_t *sess);
static void do_port(session_t *sess);
static void do_pasv(session_t *sess);
static void do_type(session_t *sess);
static void do_stru(session_t *sess);
static void do_mode(session_t *sess);
static void do_retr(session_t *sess);
static void do_stor(session_t *sess);
static void do_appe(session_t *sess);
static void do_list(session_t *sess);
static void do_nlst(session_t *sess);
static void do_rest(session_t *sess);
static void do_abor(session_t *sess);
static void do_pwd(session_t *sess);
static void do_mkd(session_t *sess);
static void do_rmd(session_t *sess);
static void do_dele(session_t *sess);
static void do_rnfr(session_t *sess);
static void do_rnto(session_t *sess);
static void do_site(session_t *sess);
static void do_syst(session_t *sess);
static void do_feat(session_t *sess);
static void do_size(session_t *sess);
static void do_stat(session_t *sess);
static void do_noop(session_t *sess);
static void do_help(session_t *sess);


typedef struct ftpcmd

{

       const char *cmd;//命令字符串

       void (*cmd_handler)(session_t *sess);//函数指针

} ftpcmd_t;

 
static ftpcmd_t ctrl_cmds[] = {
	/*控制命令*/
	{"USER",  do_user},
	{"CWD",   do_cwd},
    {"PASS", do_pass},
	{"XCWD",  do_cwd},
	{"CDUP",  do_cdup},
	{"QUIT",  do_quit},
	{"ACCT",  NULL},
	{"SMMT",  NULL},
	{"REIN",  NULL},
	/*传输参数命令*/
	{"STRU", do_stru},
    {"PORT", do_port},
    {"MODE", do_mode},
    {"PASV", do_pasv},
    {"TYPE", do_type},
	/*服务命令*/
    {"REST", do_rest},
	{"RETR", do_retr},
	{"STOR", do_stor},
	{"APPE", do_appe},
	{"LIST", do_list},
	{"NLST", do_nlst},
	{"ABOR", do_abor},
	{"\377\364\377\362ABOR", do_abor},
	{"PWD", do_pwd},
	{"XPWD", do_pwd},
	{"MKD", do_mkd},
	{"XMKD", do_mkd},
	{"RMD", do_rmd},
	{"XRMD", do_rmd},
	{"DELE", do_dele},
	{"RNFR", do_rnfr},
	{"RNTO", do_rnto},
	{"site", do_site},
	{"SYST", do_syst},
	{"SIZE" , do_size},
	{"STAT", do_stat},
	{"NOOP", do_noop},
    {"FEAT", do_feat},
	{"HELP", do_help},
	{"STOU", NULL},
	{"ALLO", NULL},
};

void handle_child(session_t *sess)
{
    //writen(sess->ctrl_fd,"220 (miniftpd 0.1)\r\n",strlen("220 (miniftpd 0.1)\r\n"));
    ftp_reply(sess, FTP_GREET, "(miniftpd 0.1)");
    int ret;
    int i;
    int size = sizeof(ctrl_cmds) / sizeof(ctrl_cmds[0]) ; //数组整个空间的大小 / 一个的大小 = 数组的长度
    for(i=0; i<size;i++)
    {
        printf("%s\n",ctrl_cmds[i].cmd);
    }
    while(1)
    {
        memset(sess->cmdline, 0, sizeof(sess->cmdline));
        memset(sess->cmd, 0, sizeof(sess->cmd));
        memset(sess->arg, 0, sizeof(sess->arg));
        ret = readline(sess->ctrl_fd, sess->cmdline, MAX_COMMAND_LINE); //收到USER liuqi\r\n
        if(ret == -1)
            ERR_EXIT("readline");
        else if(ret == 0)   //客户端 关闭 ，这两个应该同时关闭 nobody进程
            exit(EXIT_SUCCESS);

        //printf("cmdline=[%s]\n",sess->cmdline);
        //去除\r\n
        str_trim_crlf(sess->cmdline);
        printf("cmdline=[%s]\n",sess->cmdline);
        //解析FTD命令与参数
        str_split(sess->cmdline, sess->cmd, sess->arg, ' ');
        printf("cmd=[%s] arg=[%s]\n",sess->cmd, sess->arg);
        //将命令转换为大写
        str_upper(sess->cmd);
        //处理FTD命令
        /*if(strcmp("USER", sess->cmd) == 0)
        {
            do_user(sess);
        }
        else if(strcmp("PASS", sess->cmd) == 0)
        {
            do_pass(sess);
        }*/


        int i = 0;
        int size = sizeof(ctrl_cmds) / sizeof(ctrl_cmds[0]) ; //数组整个空间的大小 / 一个的大小 = 数组的长度
       
        /*for(i = 0;i <size;i++)
        {
             printf("%s\n",ctrl_cmds[i].cmd);
        }*/
        for(i = 0; i<size;i++)
        {
            if(strcmp(ctrl_cmds[i].cmd, sess->cmd) == 0)
            {
                if(ctrl_cmds[i].cmd_handler != NULL)
                {
                    ctrl_cmds[i].cmd_handler(sess);
                }
                else
                {
                    ftp_reply(sess, FTP_COMMANDNOTIMPL, "Unimplement command.");
                }
                break;
            }
            i++;
        }
        if(i == size)
        {
            ftp_reply(sess, FTP_BADCMD,"Unknown comnand.");
        }
        memset(sess->cmd, 0,sizeof(sess->cmd));
    }
}

void ftp_reply(session_t *sess, int status, const char *text)
{
    char buf[1024] = {0};
    sprintf(buf,"%d %s\r\n",status,text);
    writen(sess->ctrl_fd, buf, strlen(buf));
}

void ftp_lreply(session_t *sess, int status, const char *text)
{
    char buf[1024] = {0};
    sprintf(buf,"%d-%s\r\n",status,text);                     
    writen(sess->ctrl_fd, buf, strlen(buf));
}


int list_common(session_t *sess, int detail)
{
    DIR *dir = opendir(".");
    if(dir == NULL)
    {
        return 0;
    }

    struct dirent *dt;
    struct stat sbuf;
    while((dt = readdir(dir)) != NULL)
    {
        if(lstat(dt->d_name, &sbuf) < 0)
        {
            continue;
        }
        if(dt->d_name[0] == '.')
        {
            continue;
        }
        
		const char *perms = statbuf_get_perms(&sbuf);

        char buf[1024] = {0};
        int off = 0;
        off += sprintf(buf, "%s", perms);
        off += sprintf(buf + off, " %3d %-8d %-8d ", (int)sbuf.st_nlink, (int)sbuf.st_uid, (int)sbuf.st_gid);
        
        off += sprintf(buf + off, "%8lu ", (unsigned long)sbuf.st_size);

        
        
		const char *databuf = statbuf_get_data(&sbuf);
		off += sprintf(buf + off, "%s ",databuf);
        sprintf(buf + off, "%s ",dt->d_name);
        if(S_ISLNK(sbuf.st_mode))
        {
            char tmp[1024] = {0};
            readlink(dt->d_name, tmp, sizeof(tmp));
            off += sprintf(buf + off, "%s -> %s\r\n", dt->d_name, tmp);
        }
        else
        {
            off += sprintf(buf + off, "%s\r\n", dt->d_name);
        }
        //printf("%s", buf);
        writen(sess->data_fd, buf, strlen(buf));
    }
    closedir(dir);
    return 1;
}


//是否激活PORT
int port_active(session_t *sess)
{
    if(sess->port_addr)     //sess->port_addr 保存客户端的地址信息
    {
        if(pasv_active(sess))
        {
            fprintf(stderr, "botn port an pasv are active");
            exit(EXIT_FAILURE);
        }
        return 1;
    }
    return 0;
}

//是否为PASV被动模式
int pasv_active(session_t *sess)
{
   /* if(sess->pasv_listen_fd != -1)
    {
        if(port_active(sess))
        {
            fprintf(stderr, "botn port an pasv are active");
            exit(EXIT_FAILURE);
        }
        return 1;
    }*/

    /*监听套接字是由nobody进程创建的，ftp服务进程的sess->pasv_listen_fd 一直为-1
     * 向nobody进程命令请求看监听套接字是否处于-1*/
    priv_sock_send_cmd(sess->child_fd, PRIV_SOCK_PASV_ACTIVE);
    int active = priv_sock_get_int(sess->child_fd);
    if(active)
    {
        if(port_active(sess)) 
        {
            fprintf(stderr, "botn port an pasv are active");
            exit(EXIT_FAILURE);
        }
        return 1;
    }
    return 0;
}

int get_port_fd(session_t *sess)
{
	  /*
         * 向nobody进程  PRIV_SOCK_GET_DATA_SOCK命令   1个字节
         * 向nobody进程  一个整数   port               4个字节
         * 向nobody进程 一个字符串  ip                 不定长
         */
        priv_sock_send_cmd(sess->child_fd, PRIV_SOCK_GET_DATA_SOCK);
        unsigned short port = ntohs(sess->port_addr->sin_port);
        char *ip = inet_ntoa(sess->port_addr->sin_addr);
        priv_sock_send_int(sess->child_fd, (int)port);
        priv_sock_send_buf(sess->child_fd, ip, strlen(ip));

        char res = priv_sock_get_result(sess->child_fd);
        if(res == PRIV_SOCK_RESULT_BAD)
        {
            return 0;
        }
        else if(res == PRIV_SOCK_RESULT_OK)
        {
            sess->data_fd = priv_sock_recv_fd(sess->child_fd);
        }
        return 1;
}

int get_pasv_fd(session_t *sess)
{
    priv_sock_send_cmd(sess->child_fd, PRIV_SOCK_PASV_ACCEPT);
    char res = priv_sock_get_result(sess->child_fd);
    if(res == PRIV_SOCK_RESULT_BAD)
    {
        return 0;
    }
    else if(res == PRIV_SOCK_RESULT_OK)
    {
        sess->data_fd = priv_sock_recv_fd(sess->child_fd);
    }
    return 1;
}




int get_transfer_fd(session_t *sess)
{
    //检测是否收到PORT或者PASV命令
    if(!port_active(sess) && !pasv_active(sess))
    {
        ftp_reply(sess, FTP_BADSENDCONN, "Use PORT or PASV　first.");
        return 0;
    }
    

    int ret = 1;

    //如果是主动模式 
    if(port_active(sess))
    {
        /*
         socket
         bind(20)
         connect
         */

        //int fd = tcp_client(20);//ftp 服务进程 没有权限 bing 20端口号
                                //1.提升ftp 服务进程 权限 ，导致外界 能获取 更多 的ftp 控制权 ，导致 ftp处于不安全状态
                                //2.创建一个内部进程 ，它不与外界进行通信，只协助  ftp服务进程 数据 连接的创建
                                // nobody进程 ，赋予 他 一些 特殊 的 权限
        //printf("%d\n",fd);
        //printf("ip=%s port=%d\n",inet_ntoa(sess->port_addr->sin_addr),ntohs(sess->port_addr->sin_port));
        //if(connect_timeout(fd, sess->port_addr,0/*tunable_connect_timeout*/) < 0)
        /*{
            printf("------------------------------\n");
            close(fd);
            return 0;
        }
        sess->data_fd = fd;*/

        if(get_port_fd(sess) == 0)
        {
            ret = 0;
        }
    }


    //如果是被动模式 PASV
    if(pasv_active(sess))
    {
        int fd = accept_timeout(sess->pasv_listen_fd, NULL, tunable_accept_timeout) <0;
        close(sess->pasv_listen_fd);
        if(fd == -1)
        {
            return 0;
        }
        sess->data_fd = fd; 
    }
    printf("pppppppppppppppppppppppppppppppp\n");
    if(sess->port_addr)
    {
        free(sess->port_addr);
        sess->port_addr = NULL;
    }
    return ret;
}


//static 函数 只能在当前文件使用
static void do_user(session_t *sess)
{
    //USER liuqi  
    struct passwd *pw = getpwnam(sess->arg);
    if (pw == NULL)   //校验 用户是否存在
    {
        // 用户不存在
        ftp_reply(sess, FTP_LOGINERR, "1Login incorrect.");
        return;
    }
    sess->uid = pw->pw_uid;   //保存当前的用户id 保存在 会话中
    ftp_reply(sess, FTP_GIVEPWORD, "Please specify the password.");
}

static void do_pass(session_t *sess)
{
    // PASS liuqi
    struct passwd *pw = getpwuid(sess->uid);   //根据 保存的 uid ，再一次对用户名进行验证
    if (pw == NULL)
    {
        // 用户不存在
        ftp_reply(sess, FTP_LOGINERR, "2Login incorrect.");
        return;
    }

    printf("name=[%s]\n", pw->pw_name);
    /*struct spwd *sp = getspnam(pw->pw_name);   //获取影子文件  /etc/passwd
    if (sp == NULL)
    { // 没有找到影子文件，则代表登录也是失败的
        ftp_reply(sess, FTP_LOGINERR, "3Login incorrect.");
        return;
    }*/
    
    // 将明文进行加密
    //char *encrypted_pass/*加密后的密码*/ = crypt(sess->arg/*明文密码*/, sp->sp_pwdp/*一个种子*/);
    // 验证密码
    /*if (strcmp(encrypted_pass, sp->sp_pwdp) != 0)
    {
        ftp_reply(sess, FTP_LOGINERR, "4Login incorrect.");
        return;
    }*/
    
    //把当前的服务进程 从  root  变为 用户的登陆者 ，并更改 工作目录 
    setgid(pw->pw_gid);
    setuid(pw->pw_uid);
    chdir(pw->pw_dir);
    ftp_reply(sess, FTP_LOGINOK, "Login successful.");
}
static void do_cwd(session_t *sess)
{
}
static void do_cdup(session_t *sess)
{
}
static void do_quit(session_t *sess)
{
}
static void do_port(session_t *sess)   //暂存 客户端 发送过来的 地址信息
{
    //PORT 192,168,1,108,244,133
    unsigned int v[6];
    //暂存 ip 和 端口号
    sscanf(sess->arg,"%u,%u,%u,%u,%u,%u",&v[2],&v[3],&v[4],&v[5],&v[0],&v[1]);
    sess->port_addr = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in)); //申请空间
    memset(sess->port_addr, 0, sizeof(struct sockaddr_in));
    sess->port_addr->sin_family = AF_INET;
    unsigned char *p = (unsigned char*)&sess->port_addr->sin_port;
    p[0] = v[0];
    p[1] = v[1];

    p = (unsigned char*)&sess->port_addr->sin_addr.s_addr;
    p[0] = v[2];
    p[1] = v[3];
    p[2] = v[4];
    p[3] = v[5];

    ftp_reply(sess, FTP_PORTOK, "PORT command successful. Consider using PASV,");
    /*FTP服务进程接收到PORT h1,h2,h3,h4,p1,p2
     *    解析ipport
     *       向nobody进程  PRIV_SOCK_GET_DATA_SOCK命令   1个字节
     *          向nobody进程  一个整数   port                            4个字节
     *             向nobody进程 一个字符串  ip                              不定长
     */
}
static void do_pasv(session_t *sess)
{
    //Entering Passive Mode ()
    char ip[16] = {0};
    getlocalip(ip);
    //char *ip = "121.42.196.179";
    printf("%s\n", ip);


    //创建 监听套接字 并 保存 到 会话 中

    /*sess->pasv_listen_fd = tcp_server(ip, 0);
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);

    //获取本机的地址信息
    if(getsockname(sess->pasv_listen_fd, (struct sockaddr*)&addr, &addrlen)<0)
    {
        ERR_EXIT("getsockname");
    }

    //主机字节序 ->  网络字节序
    unsigned short port = ntohs(addr.sin_port);  */



    //监听套接字 由 nobody进程 创建
    //给nobody进程发送命令
    priv_sock_send_cmd(sess->child_fd, PRIV_SOCK_PASV_LISTEN);
    //接收 tcp_server 创建的 listen 套接字的 端口 
    unsigned short port = (int)priv_sock_get_int(sess->child_fd);


    unsigned int v[4];
    sscanf(ip, "%u.%u.%u.%u", &v[0], &v[1], &v[2], &v[3]);
    char text[1024] = {0};
    sprintf(text, "Entering Passive Mode (%u, %u, %u, %u, %u,%u).",
             v[0],v[1],v[2],v[3],port>>8,port &0xFF);
    ftp_reply(sess, FTP_PASVOK, text);

}
static void do_type(session_t *sess)
{

    if(strcmp(sess->arg, "A") == 0)
    {
        sess->is_ascii = 1;
        ftp_reply(sess, FTP_TYPEOK,"Switching to ASCII mode.");
    }
    else if(strcmp(sess->arg, "I") == 0)
    {
        sess->is_ascii = 0;   //非ACSII
        ftp_reply(sess, FTP_TYPEOK,"Switching to Binary mode.");
    }
    else
    {
        ftp_reply(sess, FTP_BADCMD,"Unrecongnised TYPE command.");
    }
}
static void do_stru(session_t *sess)
{
}
static void do_mode(session_t *sess)
{
}
static void do_retr(session_t *sess)
{
}
static void do_stor(session_t *sess)
{
}
static void do_appe(session_t *sess)
{
}
static void do_list(session_t *sess)
{
    //创建数据连接
    if(get_transfer_fd(sess) == 0)
    {
        return ;
    }

    //150
    ftp_reply(sess, FTP_DATACONN, "Here comes the directory listing.");
    //传输列表
    list_common(sess, 1);
    //关闭数据连接套接字
    close(sess->data_fd);
    sess->data_fd = -1;
    //226
    ftp_reply(sess, FTP_TRANSFEROK, "Directory send OK.");
}
static void do_nlst(session_t *sess)
{
    //创建数据连接
     if(get_transfer_fd(sess) == 0)
     {
         return ;
     }
     //150
     ftp_reply(sess, FTP_DATACONN, "Here comes the directory listing.");
     //传输列表  
     list_common(sess, 0);
     //关闭数据连接套接字 
     close(sess->data_fd);
     close(sess->data_fd); 
     sess->data_fd = -1; 
     //226
     ftp_reply(sess, FTP_TRANSFEROK, "Directory send OK.");  
}
static void do_rest(session_t *sess)
{
    printf("*******************\n");
}
static void do_abor(session_t *sess)
{
}
static void do_pwd(session_t *sess)
{
    char text[1024] = {0};
    char dir[1024] = {0};
    getcwd(dir, 1024);
    sprintf(text, "\"%s\"", dir);
    ftp_reply(sess, FTP_PWDOK, text);
}
static void do_mkd(session_t *sess)
{
}
static void do_rmd(session_t *sess)
{
}
static void do_dele(session_t *sess)
{
}
static void do_rnfr(session_t *sess)
{
}
static void do_rnto(session_t *sess)
{
}
static void do_site(session_t *sess)
{
}
static void do_syst(session_t *sess)
{
    ftp_reply(sess, FTP_SYSTOK, "UNIX Type: L8");
}
static void do_feat(session_t *sess)
{
    ftp_lreply(sess , FTP_FEAT, "Features:");
    writen(sess->ctrl_fd, " EPRT\r\n",strlen(" EPRT\r\n"));
    writen(sess->ctrl_fd, " EPSV\r\n",strlen(" EPSV\r\n"));
    writen(sess->ctrl_fd, " MDTM\r\n",strlen(" MDTM\r\n"));
    writen(sess->ctrl_fd, " EPSV\r\n",strlen(" EPSV\r\n"));
    writen(sess->ctrl_fd, " PASV\r\n",strlen(" PASV\r\n"));
    writen(sess->ctrl_fd, " REST STREAM\r\n",strlen(" REST STREAM\r\n"));
    writen(sess->ctrl_fd, " SIZE\r\n",strlen(" SIZE\r\n"));
    writen(sess->ctrl_fd, " TVFS\r\n",strlen(" TVFS\r\n"));
    writen(sess->ctrl_fd, " VTF8\r\n",strlen(" VTF8\r\n"));
    ftp_reply(sess, FTP_FEAT, " End.");
}
static void do_size(session_t *sess)
{
}
static void do_stat(session_t *sess)
{
}
static void do_noop(session_t *sess)
{
}
static void do_help(session_t *sess)
{
}
