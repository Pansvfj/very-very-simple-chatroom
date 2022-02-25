#ifndef STRUCTS_H
#define STRUCTS_H

typedef struct{
    char user_id[10];
    char user_name[20];
    short user_status;
}user_info;

typedef struct{
    short msg_type;
    unsigned int send_id;
    unsigned int receive_id;
    char msg_content[1024];
    unsigned int msg_datetime;
}msg;

#endif // STRUCTS_H
