#ifndef __COMMON_H__
#define __COMMON_H__

#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pwd.h>
#include <shadow.h>
#include <crypt.h>
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/time.h>
#include <signal.h>
#include <linux/capability.h>
#include <sys/syscall.h>


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define ERR_EXIT(m)\
	do\
	{\
		perror(m);\
		exit(EXIT_FAILURE);\
	}\
	while(0)


#define MAX_COMMAND_LINE 1024
#define MAX_COMMAND 32
#define MAX_ARG 1024
#define MINIFTP_CONF "miniftpd.conf"

#endif // __COMMON_H__
