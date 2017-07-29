#include "str.h"
#include "common.h"


void str_trim_crlf(char *str)   //把末尾的\r\n去掉
{
    char *p = &str[strlen(str)-1];
    while(*p == '\r' || *p == '\n')
    {
        *p-- = '\0';
    }
}
void str_split(const char *str, char *left, char *right ,char c) //把字符创 用空格 分开 分别放到left 和 right
{
    char * p =strchr(str, c);
    if(p == NULL)
    {
        strcpy(left, str);
    }
    else
    {
        strncpy(left, str,p-str);
        strcpy(right, p+1);
    }
}
int str_all_space(const char *str)    //判断字符串是否 全部为空格 
{
    while(*str)
    {
        if(!isspace(*str))
            return 0;
        str++;
    }
    return 1;
}
void str_upper(char *str)    //字符创变为大写字母
{
    while(*str)
    {
        *str = toupper(*str);
        str++;
    }
}
long long str_to_longlong(const char *str)    //字符串 -》 长整形
{
    //return atoll(str);
    long long result = 0;
    long long mult = 1;
    unsigned int len = strlen(str);
    int i;

    if(len >15)
        return 0;
    for(i=len-1; i>=0; i--)
    {
        long long val;
        if(str[i] < '0' || str[i] > '9')
            return 0;

        val = str[i] - '0';
        val *= mult;
        result += val;
        mult *=10;
    }
    return result;
}
unsigned int str_octal_to_uint(const char *str)  // 字符串  -》 8进制数字
{
    unsigned int result = 0;
    int seen_non_zero_digit = 0;

    while(*str)
    {
        int digit = *str;
        if(!isdigit(digit) || digit >'7')
            break;
        if(digit != '0')
            seen_non_zero_digit = 1;
        if(seen_non_zero_digit)
        {
            result <<= 3;
            result += (digit - '0');
        }
        str++;
    }

    return result;
}
