#ifndef STRUCTS_H
#define STRUCTS_H

#include "common_defines.h"
#include <netinet/in.h>

typedef enum
{
    TYPE_NONE = 0, //invalid type
    TYPE_REGISTER,
    TYPE_LOGIN,
    TYPE_OFFLINE,
    TYPE_CHAT_ONE,
    TYPE_CHAT_GROUP,
    TYPE_CHECK
}MSG_TYPE;

typedef enum
{
    PART_START = 0,
    PART_HEAD,
    PART_TYPE,
    PART_RES,
    PART_SENDID,
    PART_RECEIVEID,
    PART_CONTENT,
    PART_CONTENTLEN,
    PART_DATATIME,
    PART_DATALEN,
    PART_TAIL
}MSG_PART;

typedef enum
{
    ERR_SEND = -2,
    ERR_RECV = -1,
    ERR_NONE = 0,
    ERR_HEAD,                   //1
    ERR_TAIL,                   //2
    ERR_CONTENT_EXCEED_LEN,     //3
    ERR_CONTENT_INVALID_CHAR,   //4
    ERR_DATA,                   //5
    ERR_UNKNOW                  //6
}MSG_ERROR;

typedef enum
{
    STATUS_INITSTATUS = -1,
    STATUS_OFFLINE,
    STATUS_ONLINE,
    STATUS_CHATING,
    STATUS_BANSPEAK,
    STATUS_BLACKLIST
}USER_STATUS;

typedef enum
{
    RES_NONE = 48,     //'0'
    RES_ANALYSIS,      //'1'
    RES_RIGSTERFAIL,   //'2'
    RES_IDORPASSWDERR, //'3'
    RES_OFFLINE,       //'4'
    RES_CANNOTFINDNAME,//'5'
    RES_RECV,
    RES_UNKNOWERR,     //
}MSG_RES_CODE;

typedef struct{
    unsigned int uid;
    char name[NAME_LEM];
    int tcp_fd;
    int udp_fd;
    int port;
    bool binded;
    USER_STATUS status;
}user_info;

typedef struct{
    MSG_TYPE msg_type;
    unsigned char msg_result; //'0' is no error, look enum MSG_RES_CODE
    unsigned int send_id; //id: 0:NoId 1:server (2~X):user, also is port after login successlly,hahaha!
    unsigned int receive_id;    
    unsigned short content_len; // include '\0'
    char msg_content[MAX_CONTENT_LEN];
    unsigned int msg_datetime;
    unsigned short msg_len;
}msg;

//TODO why multiple define?
//const int SERVERID = 1;
//update: .h extern const int xxx.  .c init, const int xxx = 0

typedef struct{
    int* fd_ptr;
    struct sockaddr_in address;
}fd_and_addr;

typedef struct{
    int fd;
    struct sockaddr_in address;
}polling_fds_addrs;

typedef struct online_users_list{
    unsigned int user_id;
    char user_name[NAME_LEM];
    unsigned int fd;
    struct sockaddr_in address;
    struct online_users_list* pre;
    struct online_users_list* next;
    //expand:
    //add status: control ban speak
}online_users_node;

#endif // STRUCTS_H
