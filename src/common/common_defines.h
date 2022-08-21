#ifndef COMMON_DEFINES_H
#define COMMON_DEFINES_H

#define SERVER_ADDR "127.0.0.1"
#define SERVER_PORT 55555 //udp and tcp reuse
#define S_UDP_PORT 60000 //sever use it
#define MAX_BUF_LEN 2048
#define MAX_CONTENT_LEN 1024
#define FD_MAX_COUNT 20 //900
#define JUNK_FD_COUNT 100
#define ID_LEN 10
#define NAME_LEM 20
#define PASSWD_LEN 20
#define SERVERID 1
#define DATE_LEN 50
#define MAX_THREAD_COUNT 100

//#define DEBUG //use gcc -D define it

//I need bool type in C language
//---------------------------------
#ifndef bool
#define bool int
#endif

#ifndef true
#define true 1
#endif

#ifndef false
#define false 0
#endif
//---------------------------------

#define not !

#endif // COMMON_DEFINES_H
