#ifndef _PACK_FUNC_H_
#define _PACK_FUNC_H_

#include <stdio.h>
#include "structs.h"
#include "assist.h"
#include "common_defines.h"


typedef enum{
    INPUT_RL, //register and login
    INPUT_MSG //normal
}INPUT_LIMIT;

extern bool Recv(int fd, char* buf, size_t len, int flags);
//extern bool Send(int fd, char* buf, size_t len, int flags);
extern MSG_ERROR recv_msg(int fd, msg* m);
extern MSG_ERROR send_msg(int fd, msg* m);
extern bool Fgets(char *str, int n, FILE* stream, INPUT_LIMIT T);
extern bool input_message(char *str, int n);
extern int get_int_cmd();
extern char get_char_cmd();
extern void Exit(int status);

#endif
