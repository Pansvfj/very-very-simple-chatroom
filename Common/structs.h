#ifndef STRUCTS_H
#define STRUCTS_H

#define my_debug

typedef enum
{
    MSG_NONE = 0,
    MSG_REGISTER,
    MSG_LOGIN,
    MSG_OFFLINE,
    MSG_CHAT_ONE,
    MSG_CHAT_GROUP
}MSG_TYPE;

typedef enum
{
    PART_START = 0,
    PART_HEAD,
    PART_TYPE,
    PART_SENDID,
    PART_RECEIVEID,
    PART_CONTENT,
    PART_DATATIME,
    PART_TAIL
}MSG_PART;

typedef enum
{
    ERR_NONE = 0,
    ERR_HEAD,
    ERR_TAIL,
    ERR_CONTENT_EXCEED_LEN,
    ERR_CONTENT_INVALID_CHAR,
    ERR_DATA
}MSG_ANALYSIS_ERROR;

typedef struct{
    char user_id[10];
    char user_name[20];
    short user_status;
}user_info;

typedef struct{
    MSG_TYPE msg_type;
    unsigned int send_id;
    unsigned int receive_id;
    char msg_content[1024];
    unsigned int msg_datetime;
}msg;

#endif // STRUCTS_H
