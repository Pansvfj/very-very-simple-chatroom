#ifndef PROTOCOL_C
#define PROTOCOL_C

#include "protocol.h"
#include "assist.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <stdio.h>
#include <unistd.h>

user_info g_current_user = {
  .uid = 0,
  .name = {0},
  .tcp_fd = -1,
  .udp_fd = -1,
  .port = -1,
  .binded = false,
  .status = STATUS_INITSTATUS,
};

static const char* msg_head = "X-HEAD-X";
static const char* msg_tail = "Z-TAIL-Z";
static const char split_sign = '\n';
static const short msg_head_len = 8;
static const short msg_tail_len = 8;

char* pack_msg(msg *m/*input*/)
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

    //msg result
    data[location++] = m->msg_result;
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
    if (m->content_len > MAX_CONTENT_LEN - 2){
        fprintf(stderr, "content too long.\n");
        free(data);
        return NULL;
    }
    for (i = 0; i < m->content_len; i++){
        data[location++] = m->msg_content[i];
    }
    data[location++] = split_sign;

    //cotent len
    tmp_len = strlen(my_itoa(m->content_len));
    for (i = 0; i < tmp_len; i++){
        data[location++] = tmp_ptr[i];
    }
    data[location++] = split_sign;

    //date time
    tmp_ptr = my_itoa(m->msg_datetime);
    tmp_len = strlen(tmp_ptr);
    for (i = 0; i < tmp_len; i++){
        data[location++] = tmp_ptr[i];
    }
    data[location++] = split_sign;

    //msg total len
    int len = strlen(my_itoa(msg_tail_len + location + 2/* '\0' and split_sign */));
    m->msg_len = msg_tail_len + location + 2 + len;

    if (m->msg_len > MAX_BUF_LEN){
        printf("Msg total len too large!");
        free(data);
        return NULL;
    }
    char* msg_len_str = my_itoa(m->msg_len);
    int tmpL = strlen(msg_len_str);
    m->msg_len += len;
    for (int i = 0; i < tmpL; ++i){
        data[location++] = msg_len_str[i];
    }
    data[location++] = split_sign;

    //tail
    for (i = 0; i < msg_tail_len; i++){
        data[location++] = msg_tail[i];
    }
    data[location++] = split_sign;

    //end
    data[location] = '\0';

    //make result
    char* res = (char *)malloc(m->msg_len);
    strncpy(res, data, m->msg_len);
    free(data);

    return res;
}

MSG_ERROR analysis_msg(const char *data, msg *m/*output message*/)
{
    //puts(__FUNCTION__);
    if (m == NULL){
        return ERR_DATA;
    }
    int i = 0;
    int part = 0;
    int start_location = 0;
    int end_location = 0;
    char tmp_buf[MAX_BUF_LEN];
    int split_data_len = 0;
    while (end_location < MAX_BUF_LEN){
        //printf("the char is-------> %c\n", data[end_location]);
        //printf("end location: %d\n", end_location);
        if (data[end_location] == split_sign){
            //printf("part : %d\n", part);
            memset(tmp_buf, 0, sizeof(tmp_buf));
            part++;
            split_data_len = end_location - start_location;
            int tmp_start = start_location;
            for ( i = 0; i < split_data_len; i++){
                tmp_buf[i] = data[tmp_start++];
            }
            assert(tmp_start == end_location);
            tmp_buf[split_data_len] = '\0';
            //printf("tmp_buf------> : %s\n   len--> %d\n", tmp_buf, split_data_len);

            switch (part) {
            case PART_HEAD:
                //check head
                for (i = 0; i < split_data_len; i++){
                    if (tmp_buf[i] != msg_head[i]){
                        return ERR_HEAD;
                    }
                }
                //puts("check head success.");
                break;

            case PART_TYPE:
                //get type
                m->msg_type = (MSG_TYPE)my_atoi(tmp_buf);
                //logi("type", m->msg_type);
                break;

            case PART_RES:
                //get result
                m->msg_result = tmp_buf[0];
                break;

            case PART_SENDID:
                //get send id
                m->send_id = my_atoi(tmp_buf);
                //logi("send_id", m->send_id);
                break;

            case PART_RECEIVEID:
                //get receive
                m->receive_id = my_atoi(tmp_buf);
                //logi("receive_id", m->receive_id);
                break;

            case PART_CONTENT:
                //get content
                bzero(m->msg_content, MAX_CONTENT_LEN);
                for (i = 0; i < split_data_len; i++){
                    m->msg_content[i] = tmp_buf[i];
                    //logc("content", tmp_buf[i]);
                }
                break;
            case PART_CONTENTLEN:
                //get content len
                m->content_len = my_atoi(tmp_buf);
                //logi("content_len", m->content_len);
                break;

            case PART_DATATIME:
                //get date and time
                m->msg_datetime = my_atoi(tmp_buf);
                //logi("msg_datetime", m->msg_datetime);
                break;

            case PART_DATALEN:
                //message total len
                m->msg_len = my_atoi(tmp_buf);
                //logi("msg_len", m->msg_len);
                break;

            case PART_TAIL:
                //check tail
                //puts("start check tail--------------");
                for (i = 0; i < split_data_len; i++){
                    //printf("%c-%c\n", tmp_buf[i], msg_tail[i]);
                    if (tmp_buf[i] != msg_tail[i]){
                        return ERR_TAIL;
                    }
                }
                //puts("check tail success.");
                return ERR_NONE;

            default:
                return ERR_DATA;
                break;
            }
            start_location = end_location + 1;
        }
        end_location++;
    }
    return ERR_DATA;
}



#endif // PROTOCOL_C
