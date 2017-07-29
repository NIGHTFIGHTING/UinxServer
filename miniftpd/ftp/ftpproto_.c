#include "ftpproto.h"
#include "sysutil.h"
#include "str.h"
#include "ftpcodes.h"

static void ftp_reply(session_t *sess, int status, const char *text);
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
    const char *cmd;
    void (*cmd_handler)(session_t *sess);
} ftpcmd_t;


static ftpcmd_t ctrl_cmds[] = {
    /* 访问控制命令 */
    {"USER",    do_user    },
    {"PASS",    do_pass    },
    {"CWD",        do_cwd    },
    {"XCWD",    do_cwd    },
    {"CDUP",    do_cdup    },
    {"XCUP",    do_cdup    },
    {"QUIT",    do_quit    },
    {"ACCT",    NULL    },
    {"SMNT",    NULL    },
    {"REIN",    NULL    },
    /* 传输参数命令 */
    {"PORT",    do_port    },
    {"PASV",    do_pasv    },
    {"TYPE",    do_type    },
    {"STRU",    do_stru    },
    {"MODE",    do_mode    },

    /* 服务命令 */
    {"RETR",    do_retr    },
    {"STOR",    do_stor    },
    {"APPE",    do_appe    },
    {"LIST",    do_list    },
    {"NLST",    do_nlst    },
    {"REST",    do_rest    },
    {"ABOR",    do_abor    },
    {"\377\364\377\362ABOR", do_abor},
    {"PWD",        do_pwd    },
    {"XPWD",    do_pwd    },
    {"MKD",        do_mkd    },
    {"XMKD",    do_mkd    },
    {"RMD",        do_rmd    },
    {"XRMD",    do_rmd    },
    {"DELE",    do_dele    },
    {"RNFR",    do_rnfr    },
    {"RNTO",    do_rnto    },
    {"SITE",    do_site    },
    {"SYST",    do_syst    },
    {"FEAT",    do_feat },
    {"SIZE",    do_size    },
    {"STAT",    do_stat    },
    {"NOOP",    do_noop    },
    {"HELP",    do_help    },
    {"STOU",    NULL    },
    {"ALLO",    NULL    }
};

void handle_child(session_t *sess)
{
    writen(sess->ctrl_fd, "220 (miniftpd 0.1)\r\n", strlen("220 (miniftpd 0.1)\r\n"));
    int ret;
    while (1)
    {
        memset(sess->cmdline, 0, sizeof(sess->cmdline));
        memset(sess->cmd, 0, sizeof(sess->cmd));
        memset(sess->arg, 0, sizeof(sess->arg));
        ret = readline(sess->ctrl_fd, sess->cmdline, MAX_COMMAND_LINE);
        if (ret == -1)
            ERR_EXIT("readline");
        else if (ret == 0)
            exit(EXIT_SUCCESS);

        printf("cmdline=[%s]\n", sess->cmdline);
        // 去除\r\n
        str_trim_crlf(sess->cmdline);
        printf("cmdline=[%s]\n", sess->cmdline);
        // 解析FTP命令与参数
        str_split(sess->cmdline, sess->cmd, sess->arg, ' ');
        printf("cmd=[%s] arg=[%s]\n", sess->cmd, sess->arg);
        // 将命令转换为大写
        str_upper(sess->cmd);
        // 处理FTP命令
        /*
        if (strcmp("USER", sess->cmd) == 0)
        {
            do_user(sess);
        }
        else if (strcmp("PASS", sess->cmd) == 0)
        {
            do_pass(sess);
        }*/
        int i;
        int size = sizeof(ctrl_cmds) / sizeof(ctrl_cmds[0]);
        for (i=0; i<size; i++)
        {
            if (strcmp(ctrl_cmds[i].cmd, sess->cmd) == 0)
            {
                if (ctrl_cmds[i].cmd_handler != NULL)
                {
                    ctrl_cmds[i].cmd_handler(sess);
                }
                else
                {
                    ftp_reply(sess, FTP_COMMANDNOTIMPL, "Unimplement command.");
                }
                
                break;
            }
        }

        if (i == size)
        {
            ftp_reply(sess, FTP_BADCMD, "Unknown command.");
        }
    }
}

static void ftp_reply(session_t *sess, int status, const char *text)
{
    char buf[1024] = {0};
    sprintf(buf, "%d %s\r\n", status, text);
    writen(sess->ctrl_fd, buf, strlen(buf));
}

static void do_user(session_t *sess)
{
    //USER jjl
    struct passwd *pw = getpwnam(sess->arg);
    if (pw == NULL)
    {
        // 用户不存在
        ftp_reply(sess, FTP_LOGINERR, "Login incorrect.");
        return;
    }
    sess->uid = pw->pw_uid;
    ftp_reply(sess, FTP_GIVEPWORD, "Please specify the password.");
}

static void do_pass(session_t *sess)
{
    // PASS 123456
    struct passwd *pw = getpwuid(sess->uid);
    if (pw == NULL)
    {
        // 用户不存在
        ftp_reply(sess, FTP_LOGINERR, "Login incorrect.");
        return;
    }

    printf("name=[%s]\n", pw->pw_name);
    /*struct spwd *sp = getspnam(pw->pw_name);
    if (sp == NULL)
    { // 没有找到影子文件，则代表登录也是失败的
        ftp_reply(sess, FTP_LOGINERR, "Login incorrect.");
        return;
    }
    
    // 将明文进行加密
    char *encrypted_pass = crypt(sess->arg, sp->sp_pwdp);
    // 验证密码
    if (strcmp(encrypted_pass, sp->sp_pwdp) != 0)
    {
        ftp_reply(sess, FTP_LOGINERR, "Login incorrect.");
        return;
    }*/

    setegid(pw->pw_gid);
    seteuid(pw->pw_uid);
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

static void do_port(session_t *sess)
{
}

static void do_pasv(session_t *sess)
{
}

static void do_type(session_t *sess)
{
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
}

static void do_nlst(session_t *sess)
{
}

static void do_rest(session_t *sess)
{
}

static void do_abor(session_t *sess)
{
}

static void do_pwd(session_t *sess)
{
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
}

static void do_feat(session_t *sess)
{
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
