#include "parseconf.h"
#include "common.h"
#include "tunable.h"
#include "str.h"



//配置文件中的配置项与配置项变量对应关系表
//如果找到 则保存 在 对应的 变量中
//开关型 配置项
static struct parseconf_bool_setting
{
    const char *p_setting_name;
    int *p_variable;
}
parseconf_bool_array[] = 
{
    {"parse_enable", &tunable_pasv_enable},
    {"port_enable", &tunable_port_enable},
    {NULL, NULL}
};

//无符号 整形 配置项 
static struct parseconf_uint_setting
{
    const char *p_setting_name;
    unsigned int *p_variable;
}
parseconf_uint_array[] =
{
    {"listen_port", &tunable_listen_port},
    {"max_clients", &tunable_max_clients},
    {"max_per_ip", &tunable_max_per_ip},
    {"accept_timeout", &tunable_accept_timeout},
    {"connect_timeout", &tunable_connect_timeout},
    {"idle_session_timeout", &tunable_idle_session_timeout},
    {"data_connection_timeout", &tunable_data_connection_timeout},
    {"local_umask", &tunable_local_umask},
    {"upload_max_rate", &tunable_upload_max_rate},
    {"download_max_rate", &tunable_download_max_rate},
    {NULL, NULL}
};


//字符串 配置项
static struct parseconf_str_setting
{
    const char *p_setting_name;
    const char **p_variable;
}
parseconf_str_array[] = 
{
    {"listen_address", &tunable_listen_address},
    {NULL, NULL}
};


void parseconf_load_file(const char *path)
{
    FILE *fp= fopen(path, "r");
    if(fp == NULL)
        ERR_EXIT("fopen");
    char setting_line[1024] = {0};
    while(fgets(setting_line, sizeof(setting_line), fp) != NULL)
    {
        if(strlen(setting_line) == 0
            || setting_line[0] == '#'
            || str_all_space(setting_line))
            continue;
        str_trim_crlf(setting_line);
        parseconf_load_setting(setting_line);  
        //通过配置文件表 查找对应 字符串 和 对应的 变量值
        //通过 字符串表  查找对应的变量 ，然后 将读取的变量 存储到 对应 tunable.c 文件 中变量中
        memset(setting_line, 0, sizeof(setting_line));
    }
    fclose(fp);
}


//通过 配置文件表 中对应的 key 【字符串】 ->  value 【实际设置的值（开关类型bool，无符号类型unsigned int ，字符串类型 char *）】
void parseconf_load_setting(const char *setting)
{
    //去除左空格
    while(isspace(*setting))
        setting++;
    char key[128] = {0};
    char value[128] = {0};

    str_split(setting, key, value, '=');
    if(strlen(value) == 0)
    {
        fprintf(stderr, "mising value in config file for:%s\n",key);
        exit(EXIT_FAILURE);
    }

    //先查找 字符类型的 配置项 
    const struct parseconf_str_setting *p_str_setting = parseconf_str_array;
    while(p_str_setting->p_setting_name != NULL)
    {
        if(!strcmp(key, p_str_setting->p_setting_name))
        {
            const char **p_cur_setting = p_str_setting->p_variable;
            if(*p_cur_setting)  //原来 存在 过数据
            {
                free((char*)*p_cur_setting);
            }
            *p_cur_setting = strdup(value);    //strdup 先生申请 一块 内存 将 value 复制
            return ;
        }
        p_str_setting++;
    }

    //开关配置项
    const struct parseconf_bool_setting *p_bool_setting = parseconf_bool_array;
    while(p_bool_setting->p_setting_name != NULL)
    {
        if(!strcmp(key,p_bool_setting->p_setting_name))
        {
            str_upper(value);
            if(strcmp(value,"YES") == 0
                || strcmp(value,"TRUE") == 0
                || strcmp(value,"1") == 0)
            {
                *(p_bool_setting->p_variable) = 1;
            }

            else if(strcmp(value,"NO") == 0
                || strcmp(value,"FALSE") == 0
                || strcmp(value,"0") == 0) 
            {
                *(p_bool_setting->p_variable) = 0;
            }
            else
            {
                fprintf(stderr, "mising value in config file for:%s\n",key);
            }
            return ;
        }
        p_bool_setting++;
    }

    //查找 无符号整数
    const struct parseconf_uint_setting *p_uint_setting = parseconf_uint_array;
    while(p_uint_setting->p_setting_name != NULL)
    {
        if(!strcmp(key,p_uint_setting->p_setting_name))
        {
            if(value[0] == '0')
                *(p_uint_setting->p_variable) = str_octal_to_uint(value);
            else
                *(p_uint_setting->p_variable) = atoi(value);
            return ;
        }
        p_uint_setting++;
    }

}
