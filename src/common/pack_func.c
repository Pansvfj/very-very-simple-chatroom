#include "pack_func.h"
#include "protocol.h"

#include <sys/socket.h>
#include <errno.h>
#include <error.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

bool Recv(int fd, char* buf, size_t len, int flags)
{
    //TODO
    //no block loop read
    int res = 0;
    while (true){
        res = recv(fd, buf, len, flags);
        if (res < 0){
            printf("recv error number:%d\n", errno);
            perror("recv");
            if(errno == EINTR/*system block*/ || errno == EAGAIN /*c/s buf full*/
                    ||errno == EWOULDBLOCK /*==EAGAIN*/){
                puts("go! continue recv!");
                continue;
            }
            return false;
        }else if (res == 0){
            printf("fd:%d disconnect.\n", fd);
            //here,must remove user in onlie list
            return false;
        }else{
            printf("recv bytes:%d\n", res);
            printf("recv content:\n+---------head---------+\n"
                   "%s+---------tail---------+\n", buf);
            return true; //TODO
        }
        buf += res;
    }
    return true;
}

bool Send(int fd, char* buf, size_t len, int flags)
{
    int res = send(fd, buf, len, flags);
    if (res < 0){
        printf("send error number:%d--%s\n", errno, strerror(errno));
        perror("send");
    }else if (res == 0){
        printf("fd:%d disconnect.\n", fd);
    }else{
        printf("send content:\n+---------head---------+\n"
               "%s+---------tail---------+\n", buf);
    }
    return res;
}

MSG_ERROR send_msg(int fd, msg* m)
{
    if (m == NULL) return false;
    m->msg_datetime = get_unix_timestamp();
    char *send_data = pack_msg(m);
    MSG_ERROR ERR = ERR_NONE;
    if (not Send(fd, send_data, m->msg_len, 0)){
        ERR = ERR_SEND;
    }
    free(send_data);
    send_data = NULL;

    return ERR;
}

MSG_ERROR recv_msg(int fd, msg* m)
{
    if (m == NULL) return false;
    char buf[MAX_BUF_LEN] = {0};
    MSG_ERROR ERR = ERR_NONE;
    if (not Recv(fd, buf, MAX_BUF_LEN, 0)){
        ERR = ERR_RECV;
    }else{
        ERR = analysis_msg(buf, m);
    }
    return ERR;
}

void remove_linefeed(char *str, int len)
{
    // remove '\n'
    for (int i = 0; i < len; i++){
        if (str[i] == '\n') {
            str[i] = '\0';
            break;
        }
    }
}

bool Fgets(char *str, int n, FILE* stream, INPUT_LIMIT T)
{
    if (fgets(str, n, stream) == NULL){
        perror("fgets");
        remove_linefeed(str, n);
        return false;
    }
    if (T == INPUT_RL){
        for (int i = 0; i < n; ++i){
            if (str[i] == '\0'){
                break;
            }
            if (str[i] == '-' || str[i] == '\\' ||
                str[i] == '?' || str[i] == '/' ||
                str[i] == '*' || str[i] == ' '){
                puts("Special characters that are not allowed, please input again.");
                remove_linefeed(str, n);
                fflush(stream);
                memset(str, 0, n);
                Fgets(str, n, stream, T);
                return false;
            }
        }
    }
    remove_linefeed(str, n);

    return true;
}

bool input_message(char *str, int n)
{
   return Fgets(str, n, stdin, INPUT_MSG);
}

int get_int_cmd()
{
    char c = getchar();
    while('\n' != getchar()){
        getchar();
    }
    return c - '0';
}

char get_char_cmd()
{
    char ch = getchar();
    getchar();
    return ch;
}

void Exit(int status)
{
    msg m;
    m.msg_type = TYPE_OFFLINE;
    m.msg_result = RES_NONE;
    m.send_id = g_current_user.uid;
    m.receive_id = SERVERID;
    m.content_len = 0;
    send_msg(g_current_user.tcp_fd, &m);
    close(g_current_user.tcp_fd);
    close(g_current_user.udp_fd);
    puts("---bye---");
    sleep(1);
    exit(status);
}
