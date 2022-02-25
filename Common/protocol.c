#ifndef PROTOCOL_C
#define PROTOCOL_C

#include "protocol.h"
#include "assist.h"
#include <stdlib.h>
#include <string.h>

const char* msg_head = "X-HEAD-X";
const char* msg_tail = "Z-TAIL-Z";
const char split_sign = '\n';
const short msg_head_len = 8;
const short msg_tail_len = 8;

char* pack_msg(msg *m)
{
    if (m == NULL)
        return NULL;
    char* data = (char *)malloc(MAX_BUF_LEN);
    int location;
    int i;
    //head
    for (i = 0; i < msg_head_len; i++){
        data[i] = msg_head[i];
    }
    location = i;
    data[location++] = split_sign;

    //msg_type
    data[location++] = m->msg_type + '0';
    data[location++] = split_sign;

    //send ID
    char *tmp_ptr;
    int tmp_len;
    tmp_ptr = my_itoa(m->send_id);
    tmp_len = strlen(tmp_ptr);
    for (i = 0; i < tmp_len; i++){
        data[location++] = tmp_ptr[i];
    }
    data[location++] = split_sign;

    //receive ID
    tmp_ptr = my_itoa(m->receive_id);
    tmp_len = strlen(tmp_ptr);
    for (i = 0; i < tmp_len; i++){
        data[location++] = tmp_ptr[i];
    }
    data[location++] = split_sign;

    //content
    tmp_len = strlen(m->msg_content);
    for (i = 0; i < tmp_len; i++){
        data[location++] = m->msg_content[i];
    }
    data[location++] = split_sign;

    //data time
    tmp_ptr = my_itoa(m->msg_datetime);
    tmp_len = strlen(tmp_ptr);
    for (i = 0; i < tmp_len; i++){
        data[location++] = tmp_ptr[i];
    }
    data[location++] = split_sign;

    //tail
    for (i = 0; i < msg_tail_len; i++){
        data[location++] = msg_tail[i];
    }

    //test out put string
    data[location] = '\0'; // test
    return data;
}

msg analysis_msg(char *data, int len)
{

}



#endif // PROTOCOL_C
