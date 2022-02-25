#ifndef PROTOCOL_H
#define PROTOCOL_H

#define MAX_BUF_LEN 2048

#include "structs.h"
#include <string.h>

extern const char* msg_head;
extern const char* msg_tail;
extern const char split_sign;
extern const short msg_head_len;
extern const short msg_tail_len;

char* pack_msg(msg *m);

#endif // PROTOCOL_H
