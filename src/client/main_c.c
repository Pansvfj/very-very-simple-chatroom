#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <assert.h>
#include <pthread.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>

#include "../common/assist.h"
#include "../common/protocol.h"
#include "../common/structs.h"
#include "../common/pack_func.h"

enum Operate{
    INVALID = -1,
    EXIT,
    REGISTER,
    LOGIN,
};

static bool start_P = false; /*control private message*/
static bool start_G = false; /*control group message*/
static struct sockaddr_in server_address;

char* pack_user_name_passwd(const enum Operate OPE, unsigned short *out_bytes/*no include '\0' & '\n'*/, int* separatorIndex)
{
    if (OPE == REGISTER){
        puts("Please input your username(limit:20):");
    }else if (OPE == LOGIN){
        puts("Please input your id(limit:10):");
    }
    char username[NAME_LEM];
    Fgets(username, NAME_LEM, stdin, INPUT_RL);
    char passwd[PASSWD_LEN];
    puts("Please input your password(limit:20):");
    Fgets(passwd, PASSWD_LEN, stdin, INPUT_RL);

    static char register_data[NAME_LEM + PASSWD_LEN];
    int data_index = 0;
    for (int i = 0; i < 20; i++){
        if (username[i] != '\0')
            register_data[data_index++] = username[i];
        else
            break;
    }
    if (separatorIndex != NULL){ *separatorIndex = data_index; }
    register_data[data_index++] = '-';
    for (int i = 0; i < 20; i++){
        if (passwd[i] != '\0')
            register_data[data_index++] = passwd[i];
        else
            break;
    }

    if (out_bytes != NULL){ *out_bytes = data_index;}

    return register_data;
}

bool doCmd(enum Operate cmd)
{
    msg m;
    switch (cmd) {
    case REGISTER:{
        int separatorIndex = -1;
        char* register_data = pack_user_name_passwd(REGISTER, &m.content_len, &separatorIndex);
        m.msg_type = TYPE_REGISTER;
        m.send_id = 0;
        m.receive_id = SERVERID;
        m.msg_result = RES_NONE;
        strncpy(m.msg_content, register_data, m.content_len);
        send_msg(g_current_user.tcp_fd, &m);

        puts("registering...");

        MSG_ERROR err = recv_msg(g_current_user.tcp_fd, &m);
        if (err == ERR_NONE){
            fprintf(stdout, "msg_type: %d\nmsg_result: %c\nsend_id: %u\nreceive_id: %u\ncontent_len: %u\nmsg_content: %s\nmsg_datetime: %u\nmsg_len: %u\n",
                    m.msg_type, m.msg_result ,m.send_id, m.receive_id,
                    m.content_len, m.msg_content, m.msg_datetime,
                    m.msg_len);

            if (m.msg_result == RES_NONE){
                puts("please remeber you id and password.");
                char username[NAME_LEM];
                strncpy(username, register_data, separatorIndex);
                username[separatorIndex] = '\0';
                char id[ID_LEN];
                strncpy(id, m.msg_content, m.content_len);
                m.msg_content[m.content_len] = '\0';
                username[separatorIndex] = '\0';
                printf("your id:%s\nyour nickname:%s\n", m.msg_content, username);
                return true;
            }else {
                puts("Register fail");
                m.msg_content[m.content_len] = '\0';
                printf("Error:%s\n", m.msg_content);
                return false;
            }
        }else{
            printf("error: %d\n", err);
            return false;
        }
    }
        break;
    case LOGIN:
    {
        int separatorIndex = -1;
        char* login_data = pack_user_name_passwd(LOGIN, &m.content_len, &separatorIndex);
        m.msg_type = TYPE_LOGIN;
        char tmp_char = login_data[separatorIndex];
        login_data[separatorIndex] = '\0';

        m.send_id = my_atoi(login_data);
        login_data[separatorIndex] = tmp_char;
        m.receive_id = SERVERID;
        m.msg_result = RES_NONE;
        strncpy(m.msg_content, login_data, m.content_len);
        send_msg(g_current_user.tcp_fd, &m);

        puts("logining...");

        MSG_ERROR err = recv_msg(g_current_user.tcp_fd, &m);
        if (err == ERR_NONE){
            fprintf(stdout, "msg_type: %d\nmsg_result: %c\nsend_id: %u\nreceive_id: %u\ncontent_len: %u\nmsg_content: %s\nmsg_datetime: %u\nmsg_len: %u\n",
                    m.msg_type, m.msg_result, m.send_id, m.receive_id,
                    m.content_len, m.msg_content, m.msg_datetime,
                    m.msg_len);

            if (m.msg_result == RES_NONE){
                g_current_user.uid = m.receive_id;
                assert(sizeof (g_current_user.name) > m.content_len);
                strncpy(g_current_user.name, m.msg_content, m.content_len);
                g_current_user.name[m.content_len] = '\0';
                g_current_user.status = STATUS_ONLINE;
                puts("Login successfully.");

                //save port, UDP port and TCP port reuse
                g_current_user.port = m.send_id;

                return true;
            }else{
                puts("Login fail: please check your username and passwd.");
                return false;
            }
        }
        printf("error: %d\n", err);
        return false;
        puts("Login fail. Recv");
        return false;
    }
        break;
    default:
        puts("Error: Unkonw operation");
        break;
    }

    return false;
}

void try_login(int fd)
{
    bool success = false;
    puts("continue login? Y/N");
    char ch = get_char_cmd();

    if (ch == 'Y' || ch == 'y'){
        int try_times = 3;
        while (try_times > 0){
            if (doCmd(LOGIN)){
                success = true;
                break;
            }
        }
        if (not success){
            puts("Login fail times exceed max times");
            Exit(0);
        }
    }else{
        close(fd);
        Exit(0);
    }
}

void hand_interrupt(int signal_no)
{
    if (signal_no == SIGINT && g_current_user.status == STATUS_CHATING){
        start_P = false;
        start_G = false;
        int flags = fcntl(STDOUT_FILENO, F_GETFL, 0);
        if (fcntl(STDOUT_FILENO, F_SETFL, flags | O_NONBLOCK) < 0){
            perror("fcntl");
        }
    }else if (signal_no == SIGINT){
        int flags = fcntl(STDOUT_FILENO, F_GETFL, 0);
        if (fcntl(STDOUT_FILENO, F_SETFL, flags ^ O_NONBLOCK) < 0){
            perror("fcntl");
        }
        Exit(0);
    }
}

void* get_private_msg()
{
    char buf[1];
    msg m;
    while(start_P){
        if (g_current_user.status == STATUS_CHATING){
            //MSG_PEEK also block, so I add MSG_DONTWAIT check msg coming
            int res = recv(g_current_user.tcp_fd, buf, 1, MSG_PEEK | MSG_DONTWAIT);
            if (res < 0){
                if(errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK){
                    //puts("go go continue.\n");
                    sleep(1);
                    continue;
                }
                else{
                    perror("get_private_msg recv");
                    pthread_exit((void *)-1);
                }
            }else if (res == 0){
                puts("Server Close");
                pthread_exit((void *)-2);
            }

            MSG_ERROR ERR = recv_msg(g_current_user.tcp_fd, &m);
            if (ERR == ERR_NONE){
                if ( m.msg_result == RES_NONE
                     && m.msg_type == TYPE_CHAT_ONE/*
                     && m.receive_id == g_current_user.uid*/)
                {
                    m.msg_content[m.content_len] = '\0';
                    printf("[id:%d time:%s]%s\n", m.receive_id, get_current_datetime_str(), m.msg_content);
                }
            }else if (ERR == ERR_RECV){
                pthread_exit((void *)-3);
            }else {
                printf("get_private_msg, analysis fail:%d", ERR);
            }
        }
        sleep(1);
    }
    pthread_exit(0);
}

void *get_goup_msg()
{
    puts(__FUNCTION__);
    char buf[MAX_BUF_LEN];
    msg m;
    while(start_G){
        if (g_current_user.status == STATUS_CHATING){
            //MSG_PEEK also block, so I add MSG_DONTWAIT check msg coming
            int recv_res = recv(g_current_user.udp_fd, buf, 1, MSG_PEEK | MSG_DONTWAIT);
            if (recv_res <= 0){//UDP only care >0
                if(errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK){
                    //puts("go go continue.\n");
                    sleep(1);
                    continue;
                }
                else{
                    perror("get_goup_msg recv");
                    pthread_exit((void *)-1);
                }
            }
            /*else if (res == 0){
                puts("Server Close");
                pthread_exit((void *)-2);
            }*/

            socklen_t len = sizeof (server_address);
            int res = recvfrom(g_current_user.udp_fd, buf, MAX_BUF_LEN, 0,
                     (struct sockaddr*)&server_address, &len);
            if (res <= 0){
                printf("res:%d\n", res);
                perror("get_goup_msg recvfrom");
                sleep(1);
                continue;
            }else{
                analysis_msg(buf, &m);
                if (m.send_id == g_current_user.uid){
                    sleep(1);
                    continue;
                }
                printf("[id:%d time:%s]%s\n", m.send_id, get_current_datetime_str(), m.msg_content);
            }
        }
        sleep(1);
    }
    puts("get_goup_msg thread exit");
    pthread_exit(0);
}

void restore_stdin()
{
    int flags = fcntl(STDOUT_FILENO, F_GETFL, 0);
    if (fcntl(STDOUT_FILENO, F_SETFL, flags ^ O_NONBLOCK) < 0){
        perror("fcntl");
    }
}

int chat() // 0 normal exit, 1 try again chat, 2 net error
{
    puts("How do you want to chat?");
    puts("+------------------+");
    puts("|1. Private Chat   |");
    puts("|2. Group Chat     |");
    puts("|0. Exit           |");
    puts("+------------------+");

    int chat_cmd = get_int_cmd();
    if (chat_cmd < 0 || chat_cmd > 2){
        return 1;
    }
    if (chat_cmd == 0){
        Exit(0);
    }

    if (chat_cmd == 1){
        //create thread read private msg
        pthread_t t_id;
        if (pthread_create(&t_id, NULL, get_private_msg, NULL) == 0){
            start_P = true;
            //pthread_detach(t_id);
        }else{
            perror("pthread_create");
            Exit(0);
        }

        puts("Who do you want to chat with?Please input your friend's id.");
        int friend_id = 0;
        do{
           char buf[ID_LEN];
           Fgets(buf, ID_LEN, stdin, INPUT_RL);
           friend_id = my_atoi(buf);
        }while (friend_id < 2 && (puts("input id error") != EOF));

        //judge the id is onlie or offline
        puts("judge if online");
        msg m;
        m.msg_type = TYPE_CHECK;
        m.msg_result = RES_NONE;
        m.send_id = g_current_user.uid;
        m.receive_id = friend_id;
        m.content_len = 0;
        m.msg_datetime = get_unix_timestamp();
        int try_times = 3;
        MSG_ERROR err = send_msg(g_current_user.tcp_fd, &m);
        while(err != ERR_NONE && try_times){
            try_times--;
            sleep(1);
        }
        if (try_times < 1){
            puts("Network error");
            return 2;
        }else{
            char buf[MAX_BUF_LEN];
            MSG_ERROR ERR = recv_msg(g_current_user.tcp_fd, &m);
            if (ERR_NONE == ERR){
                if (/*m.receive_id == g_current_user.uid &&*/ m.msg_result == RES_NONE){
                    m.msg_content[m.content_len] = '\0';
                    char friend_name[NAME_LEM];
                    strcpy(friend_name, m.msg_content);

                    g_current_user.status = STATUS_CHATING;
                    m.msg_type = TYPE_CHAT_ONE;
                    m.send_id = g_current_user.uid;
                    m.receive_id = friend_id;

                    printf("your friend:%s online.\n", m.msg_content);
                    puts  ("+------------------------------------->>>");
                    puts  ("|          private  chatroom             ");
                    printf("|my id      : %d, my nick name      :%s\n",
                           g_current_user.uid, g_current_user.name);
                    printf("|friend's id: %d, friend's nick name:%s\n",
                           friend_id, friend_name);
                    puts  ("+------------------------------------->>>");
                    puts("Tips:use Ctrl + C stop chat.");

                    while(start_P){
                        Fgets(buf, MAX_CONTENT_LEN, stdin, INPUT_MSG);
                        if (not start_P) break;

                        m.content_len = strlen(buf);
                        strncpy(m.msg_content, buf, m.content_len);
                        MSG_ERROR err = send_msg(g_current_user.tcp_fd, &m);

                        if (err == ERR_NONE){
                            m.msg_content[m.content_len] = '\0';
                            printf("[%s(%d) time:%s]%s\n",
                                   g_current_user.name,
                                   g_current_user.uid,
                                   get_current_datetime_str(),
                                   m.msg_content);
                        }else{
                            puts("send fail");
                            return 2;
                        }
                    }
                    restore_stdin();
                    g_current_user.status = STATUS_ONLINE;
                    puts("Chat room was closed.");
                    start_P = false;
                    //wait thread finished
                    pthread_join(t_id, NULL);
                    return 1;
                }else{

                    start_P = false;
                    printf("user id : %d is offline.\n", friend_id);
                    //wait thread finished
                    pthread_join(t_id, NULL);
                    //restore_stdin();
                    return 1;
                }
            }else{
                printf("analysis error:%d\n", ERR);
                start_P = false;
                pthread_join(t_id, NULL);
                return 0;
            }
        }
        start_P = false;
        pthread_join(t_id, NULL);
        return 0;
    }

    if (chat_cmd == 2){      
        //int socket(int af, int type, int protocol);
        //protocl:IPPROTO_TCP or IPPTOTO_UDP, 0 auto
        if (g_current_user.udp_fd == -1){
            g_current_user.udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
        }
        printf("udp fd:%d\n", g_current_user.udp_fd);
        if (g_current_user.udp_fd < 0){
            start_G = false;
            return 0;
        }

        //create thread read private msg
        pthread_t t_id;
        if (pthread_create(&t_id, NULL, get_goup_msg, NULL) == 0){
            start_G = true;
            //pthread_detach(t_id);
        }else{
            perror("pthread_create");
            Exit(0);
        }

        msg m;
        g_current_user.status = STATUS_CHATING;
        m.msg_type = TYPE_CHAT_GROUP;
        m.send_id = g_current_user.uid;
        m.receive_id = 0;
        m.msg_result = RES_NONE;
        char buf[MAX_BUF_LEN];

        //init server address
        memset(&server_address, 0, sizeof(server_address));
        server_address.sin_family = AF_INET;
        inet_pton(AF_INET, SERVER_ADDR, &server_address.sin_addr);
        server_address.sin_port = htons(SERVER_PORT);

        struct  sockaddr_in local_addr;
        memset(&local_addr, 0, sizeof (local_addr));
        local_addr.sin_family = AF_INET;
        local_addr.sin_port = htons(g_current_user.port);//use tcp port
        local_addr.sin_addr.s_addr = htonl(INADDR_ANY);

        if (g_current_user.binded == false){
            int err = bind(g_current_user.udp_fd,(const struct sockaddr*)&local_addr,sizeof (local_addr));
            if (err != 0){
                perror("udp bind");
                printf("errno: %d\n", errno);
                if (/*errno != */true){
                    close(g_current_user.udp_fd);
                    g_current_user.udp_fd = -1;
                    return 0;
                }
            }
            g_current_user.binded = true;
        }

        in_port_t tmp_port = server_address.sin_port;
        server_address.sin_port = htons(S_UDP_PORT);

        //launch UDP
        system("clear");
        puts("------------------welcome group chat room------------------");
        while (start_G){
            Fgets(buf, MAX_CONTENT_LEN, stdin, INPUT_MSG);
            if (not start_G || strlen(buf) == 0) break;

            m.content_len = strlen(buf);
            strncpy(m.msg_content, buf, m.content_len);
            m.msg_datetime = get_unix_timestamp();

            char *data = pack_msg(&m);
            int err = sendto(g_current_user.udp_fd, data,m.msg_len,0,
                   (const struct sockaddr*)&server_address, sizeof (server_address));
            //printf("errno:%d=%s\n", errno, strerror(errno));
            if (err > 0){
                m.msg_content[m.content_len] = '\0';
                printf("[%s(%d) time:%s]%s\n",
                       g_current_user.name,
                       g_current_user.uid,
                       get_current_datetime_str(),
                       m.msg_content);
                continue;
            }else{
                puts("upd send fail");
                pthread_join(t_id, NULL);
                server_address.sin_port = tmp_port;
                start_G = false;
                pthread_join(t_id, NULL);
                restore_stdin();
                return 2;
            }

        }
        g_current_user.status = STATUS_ONLINE;
        puts("Chat room was closed.");
        start_G = false;
        pthread_join(t_id, NULL);//wait thread finished
        restore_stdin();
        server_address.sin_port = tmp_port;

        return 1;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    for (int i = 0; i < argc; ++i){
        printf("%s ", argv[i]);
        //I think it can control compile debug and release version
    }
    puts("");

    puts("*********************");
    puts("Welcome to chat room.");
    puts("*********************");

    //init server address
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    inet_pton(AF_INET, SERVER_ADDR, &server_address.sin_addr);
    server_address.sin_port = htons(SERVER_PORT);

    puts("Connecting server...");
    int  conn_fd = socket(AF_INET, SOCK_STREAM, 0);
    int res = connect(conn_fd, (struct sockaddr*)(&server_address), sizeof(server_address));
    printf("connect result: %d\n", res);
    if (res < 0){
        perror("connect");
        return -1;
    }else{
        puts("Connected server successfully.");
        g_current_user.tcp_fd = conn_fd;
    }

#if 1
    int cmd = -1;
    bool success = false;
    do{
        puts("+------------------+");
        puts("|1. Register       |");
        puts("|2. Login          |");
        puts("|0. Exit           |");
        puts("+------------------+");
        cmd = get_int_cmd();
        if (cmd == EXIT){
            Exit(0);
        }
        success = doCmd(cmd);
    }while(cmd != REGISTER && cmd != LOGIN);

    if (cmd == REGISTER){
        if (not success){
            close(conn_fd);
            Exit(0);
        }else{
            try_login(conn_fd);
        }
    }else{
        if (not success){
            try_login(conn_fd);
        }
    }

    //login success
    assert(g_current_user.status == STATUS_ONLINE);
    printf("--->>>Welcome %s<<<---\n", g_current_user.name);

    //catch SIGINT signal
    signal(SIGINT, hand_interrupt);

    //chat
    int ret = chat();
    fflush(stdin);
    while (ret == 1){
        ret = chat();
    }

#else
    while (connect_fd == 0){
        memset(buf, '\0', MAX_BUF_LEN);
        puts("please input character:");
        fgets(buf, MAX_BUF_LEN, stdin);
        int len = strlen(buf);
        if (len == 1 || buf[0] == '\n'){
            puts("input error");
            continue;
        }
        buf[len - 1] = '\0';
        send(socket_fd, buf, len - 1, 0);
        printf("send data--->%s\n", buf);
        memset(buf, '\0', MAX_BUF_LEN);
        int n = recv(socket_fd, buf, MAX_BUF_LEN, 0);
        puts("<---recv msg:");
        //printf("receive data<---%s\n", buf);
        write(STDOUT_FILENO, buf, n);
        puts("");
    }
#endif
    return 0;
}
