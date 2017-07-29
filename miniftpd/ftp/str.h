#ifndef __STR_H__
#define __STR_H__

void str_trim_crlf(char *str);
void str_split(const char *str, char *left, char *right ,char c);
int str_all_space(const char *Str);
void str_upper(char *str);
long long str_to_longlong(const char *str);
unsigned int str_octal_to_uint(const char *str);


#endif //__STR_H__
