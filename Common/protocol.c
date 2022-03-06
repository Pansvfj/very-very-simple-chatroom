#ifndef PROTOCOL_C
#define PROTOCOL_C

#include "protocol.h"
#include "assist.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifdef my_debug
#include <stdio.h>
#include <unistd.h>
#endif

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
    data[location++] = split_sign;

    //test out put string
    data[location] = '\0'; // test for printf string
    return data;
}

MSG_ANALYSIS_ERROR analysis_msg(const char *data, msg *m)
{
    MSG_ANALYSIS_ERROR err = ERR_NONE;
    if (m == NULL){
        return ERR_DATA;
    }
    int i = 0;
    int part = 0;
    int start_location = 0;
    int end_location = 0;
    char tmp_buf[MAX_BUF_LEN];
    int split_data_len = 0;
    int location = 0;
    while (end_location < MAX_BUF_LEN){
        printf("the char is-------> %c\n", data[end_location]);
        printf("end location: %d\n", end_location);
        if (data[end_location] == split_sign){
            printf("part : %d\n", part);
            memset(tmp_buf, 0, sizeof(tmp_buf));
            part++;
            split_data_len = end_location - start_location;
            int tmp_start = start_location;
            for ( i = 0; i < split_data_len; i++){
                tmp_buf[i] = data[tmp_start++];
            }
            assert(tmp_start == end_location);
            tmp_buf[split_data_len] = '\0';
            printf("tmp_buf------> : %s\n   len--> %d\n", tmp_buf, split_data_len);

            switch (part) {
            case PART_HEAD:
                //check head
                for (i = 0; i < split_data_len; i++){
                    if (tmp_buf[i] != msg_head[i]){
                        return ERR_HEAD;
                    }
                }
                puts("check head success.");
                break;

            case PART_TYPE:
                //get type
                m->msg_type = (MSG_TYPE)my_atoi(tmp_buf);
                break;

            case PART_SENDID:
                //get send id
                m->send_id = my_atoi(tmp_buf);
                break;

            case PART_RECEIVEID:
                //get receive
                m->receive_id = my_atoi(tmp_buf);
                break;

            case PART_CONTENT:
                //get content
                for (i = 0; i < split_data_len; i++){
                    m->msg_content[i] = tmp_buf[i];
                }
                break;

            case PART_DATATIME:
                //get date and time
                m->msg_datetime = my_atoi(tmp_buf);
                break;

            case PART_TAIL:
                //check tail
                puts("start check tail--------------");
                for (i = 0; i < split_data_len; i++){
                    if (tmp_buf[i] != msg_tail[i]){
                        return ERR_TAIL;
                    }
                }
                puts("check tail success.");
                return ERR_NONE;

            default:
                return ERR_DATA;
                break;
            }
            start_location = end_location + 1;
            usleep(1000 * 100);
        }
        usleep(1000 * 100);
        end_location++;
    }
    return ERR_NONE;
}



#endif // PROTOCOL_C
