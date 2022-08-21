#include "server.h"
#include "list_operation.h"

#include "../common/structs.h"
#include "../common/assist.h"
#include "../common/protocol.h"
#include "../common/pack_func.h"
#include "../common/common_defines.h"

#include "databases_manager.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <pthread.h>

struct sockaddr_in server_address;

static int udp_sockfd = -1;
static char upd_buf[MAX_BUF_LEN];
static int udp_recvs = -1;
pthread_mutex_t udp_mutex = PTHREAD_MUTEX_INITIALIZER;

//Instruction:in the actual situation,use the udp brocast isn't necessary,
//better way is handling group msg from TCP connect, loop send msg to all online use.
//But, I want to play udp, ha ha ha.
int init_brocast() //this funciton must run after init_server()
{
    puts(__FUNCTION__);
    //call socket()
    int socket_res = socket(AF_INET, SOCK_DGRAM/*UDP*/, 0);
    if (socket_res < 0){
        perror("udp socket");
        return -1;
    }

    //modify udp port
    server_address.sin_port = htons(S_UDP_PORT);

    //call bind
    int bind_res = bind (socket_res, (struct sockaddr *)(&server_address), sizeof(server_address));
    if (bind_res < 0){
        perror("udp bind");
        return -2;
    }
    udp_sockfd = socket_res;
    printf("upd brocast init finished success. udpfd:%d\n", udp_sockfd);

    return 0;
}

void send_node(online_users_node* node)
{
    if (udp_recvs > 0){
        sendto(udp_sockfd, upd_buf, udp_recvs, 0,
               (const struct sockaddr*)&node->address, sizeof (node->address));

    }
}

void* do_brocast()
{
    while(true){
        puts(__FUNCTION__);
        udp_recvs = 0;
        memset(upd_buf, 0, sizeof(upd_buf));
        if (udp_sockfd == -1){
            puts("udp_sockfd is valid:-1\n");
            return NULL;
        }
        struct sockaddr_in addr;
        socklen_t len = sizeof (addr);
        udp_recvs = recvfrom(udp_sockfd, upd_buf, MAX_BUF_LEN, 0, (struct sockaddr*)&addr, &len);
        //printf("udp data: %s\n", upd_buf);
        list_operate(NULL, NULL, send_node);
    }
}

int init_server()
{
    int socket_fd = -1;
    //netinet/in.h  INET_ADDRSTRLEN = 16

    //socket()
    socket_fd = socket(AF_INET/*IPV4*/, SOCK_STREAM/*TCP*/, 0);
    if (socket_fd == -1){
        perror("socket");
        exit(-1);
    }
    int opt = 1;
    //solve address aleady in use. port reuse, server reboot
    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof (opt));

    //init server address struct
    bzero(&server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(SERVER_PORT);

    //bind return 0 success
    int ret = 0;
    ret = bind (socket_fd, (struct sockaddr *)(&server_address), sizeof(server_address));
    if (ret < 0){
        perror("bind");
        exit(-2);
    }

    //listen
    ret = listen(socket_fd, FD_MAX_COUNT);
    if (ret < 0){
        perror("listen");
        return -1;
    }

    return socket_fd;
}

void separate_name_id_passwd(const char* data, int len,  char* outname_id, char*outpasswd)
{
    if (data == NULL) return;

    int assist_i = 0;
    bool flag = true;
    for (int i = 0; i < len; ++i){
        if (data[i] != '-' && flag){
            outname_id[assist_i++] = data[i];
        }else {
            if (flag){
                outname_id[assist_i] = '\0';
                assist_i = 0;
                flag = false;
                continue;
            }
            outpasswd[assist_i++] = data[i];
        }
    }
    outpasswd[assist_i] = '\0';
}

int handle_register(const msg* m)
{
    if (m == NULL) return false;
    puts(__FUNCTION__);
    if (m->send_id != 0 || m->receive_id != SERVERID){
        return -1;
    }
    char username[NAME_LEM];
    char passwd[PASSWD_LEN];
    separate_name_id_passwd(m->msg_content, m->content_len, username, passwd);
    int uid = create_new_id(username, passwd);
    if (uid < 0){
        puts("register fail");
        return -1;
    }

    return uid;
}

bool handle_login(const msg* m, int* targetId)
{
    puts(__FUNCTION__);
    if (m == NULL) return false;
    char id[ID_LEN];
    char passwd[PASSWD_LEN];
    separate_name_id_passwd(m->msg_content, m->content_len, id, passwd);
    int id_num = my_atoi(id);
    *targetId = id_num;

    return check_password(id_num, passwd, strlen(passwd));
}

const char* handle_check_online(msg* m)
{
    puts(__FUNCTION__);
    if (m == NULL) return false;

    if (user_is_online(m->receive_id, true))
        return get_name_from_id(m->receive_id);
    else
        return NULL;
}

void handle_offline(const unsigned int fd)
{
    remove_user(fd, false);
}

bool handle_chatone(msg* m)
{
    puts(__FUNCTION__);
    if (m == NULL) return false;

    //judege sender if oline
    if (not user_is_online(m->send_id, true)){
        return false;
    }

    //judge recver if online
    if (not user_is_online(m->receive_id, true)){
        //get fd of sender
        int fd = get_fd_from_id(m->send_id);
        m->msg_result = RES_OFFLINE;
        m->send_id = SERVERID;
        m->content_len = 0;
        send_msg(fd, m);
    }else{
        //get fd of recver
        int target_fd = get_fd_from_id(m->receive_id);
        m->msg_result = RES_NONE;
        //swap sender and recver
        m->send_id ^= m->receive_id;
        m->receive_id ^= m->send_id;
        m->send_id ^= m->receive_id;
        send_msg(target_fd, m);
    }
}

bool handle_groupchat(/*msg* m*/)
{
    return false;
}

void *handleRequest(void *ptr)
{
    puts(__FUNCTION__);
    fflush(stdout);
    static int res = 0;

    //if (ptr == NULL || *((int **)ptr) == NULL){
    if (ptr == NULL || ((fd_and_addr*)ptr)->fd_ptr == NULL){
        res = -1;
        pthread_exit((void *)(&res));
    }


    //int fd = **((int **)ptr);
    int fd = *((fd_and_addr*)ptr)->fd_ptr;
    msg recvMsg;
    res = recv_msg(fd, &recvMsg);

    if(res == ERR_RECV){
        //remove invalid fd
        remove_user(fd, false);

        puts("stop handle Request");

        //free(*((int **)ptr));
        free(((fd_and_addr*)ptr)->fd_ptr);
        //*(int **)(ptr) = NULL;
        ((fd_and_addr*)ptr)->fd_ptr = NULL;

        pthread_exit((void *)&res);
    }
    //handling
    if(res != ERR_NONE){
        printf("analysis fail, code : %d\n", res);
        recvMsg.msg_result = RES_ANALYSIS;
        recvMsg.send_id = SERVERID;
        send_msg(fd, &recvMsg);
        //free(*((int **)ptr));
        free(((fd_and_addr*)ptr)->fd_ptr);
        //*(int **)(ptr) = NULL;
        ((fd_and_addr*)ptr)->fd_ptr = NULL;

        pthread_exit((void *)&res);
    }

    //msg format true
    bool save_fd = false;
    switch (recvMsg.msg_type) {
    case TYPE_REGISTER:
    {
        int uid = handle_register(&recvMsg);
        recvMsg.send_id = SERVERID;
        if (uid < 0){
            recvMsg.msg_result = RES_RIGSTERFAIL;
            recvMsg.content_len = 0;
            send_msg(fd, &recvMsg);

            puts("register fail.");
            //free(*((int **)ptr));
            remove_user(fd, false);// remove it
            free(((fd_and_addr*)ptr)->fd_ptr);
            //*((int **)ptr) = NULL;
            ((fd_and_addr*)ptr)->fd_ptr = NULL;
            res = RES_RIGSTERFAIL;

            pthread_exit((void *)&res);
        }
        char *uid_str = my_itoa(uid);
        recvMsg.msg_result = RES_NONE;
        recvMsg.content_len = strlen(uid_str);
        strncpy(recvMsg.msg_content, uid_str, recvMsg.content_len);
        send_msg(fd, &recvMsg);
        save_fd = true;
    }
        break;
    case TYPE_LOGIN:
    {
        int uid = -1;
        if (handle_login(&recvMsg, &uid)){
            const char *name = get_dbname_from_dbid(uid);
            if (name != NULL){
                recvMsg.msg_result = RES_NONE;
                recvMsg.content_len = strlen(name);
                strncpy(recvMsg.msg_content, name, recvMsg.content_len);

                //login successlly
                add_user(fd, uid, name, recvMsg.content_len, ((fd_and_addr*)ptr)->address);
                recvMsg.send_id = ntohs(((fd_and_addr*)ptr)->address.sin_port); //look!here is port!
            }else{
                recvMsg.msg_result = RES_CANNOTFINDNAME;
                const char* str = "Can't get user name";
                int len = strlen(str);
                strncpy(recvMsg.msg_content, str, len);
                recvMsg.content_len = len;
                recvMsg.send_id = SERVERID;
            }
        }else{
            recvMsg.msg_result = RES_IDORPASSWDERR;
            recvMsg.content_len = 0;
            recvMsg.send_id = SERVERID;
            save_fd = true;
        }
        recvMsg.receive_id = uid;
        send_msg(fd, &recvMsg);
    }
        break;
    case TYPE_CHECK:
        recvMsg.msg_datetime = get_unix_timestamp();
        const char* name = handle_check_online(&recvMsg);
        recvMsg.receive_id = recvMsg.send_id;
        recvMsg.send_id = SERVERID;

        if (name != NULL){
            recvMsg.msg_result = RES_NONE;
            recvMsg.content_len = strlen(name);
            strncpy(recvMsg.msg_content, name, recvMsg.content_len);
        }else{
            recvMsg.msg_result = RES_CANNOTFINDNAME;
            recvMsg.content_len = 0;
        }
        send_msg(fd, &recvMsg);

        break;
    case TYPE_OFFLINE:
        //handle_offline(recvMsg.send_id);
        handle_offline(fd);
        break;
    case TYPE_CHAT_ONE:
        handle_chatone(&recvMsg);
        break;
    case TYPE_CHAT_GROUP:
        //ha ha ha, here I use UDP
        //handle_groupchat(&recvMsg);
        break;
    default:
        break;
    }
    if (not save_fd){
        //free(*((int **)ptr));
        free(((fd_and_addr*)ptr)->fd_ptr);
        //*(int **)(ptr) = NULL;
        ((fd_and_addr*)ptr)->fd_ptr = NULL;
    }

    //return (void *)&res;
    pthread_exit((void *)(&res));
}

void start_up()
{
    //sock fd use for read#include<pthread.h>
    const int socket_fd = init_server();
    if (socket_fd < 0){
        perror("init server");
        return;
    }

    //init udp
    init_brocast();

    //create udp thread
    pthread_t udp_thread;
    pthread_create(&udp_thread, NULL, do_brocast, NULL);
    //pthread_detach(udp_thread);

    fd_set read_sockets;
    fd_set write_sockets;
    fd_set error_sockets;
    //int fds[FD_MAX_COUNT];
    polling_fds_addrs polling_data[FD_MAX_COUNT];
    memset(&polling_data, 0, sizeof (polling_data));
    int max_fd = 0;
    pthread_t thread_arr[MAX_THREAD_COUNT] = {0};
    int threads_count = 0;
    fd_and_addr thread_args[FD_MAX_COUNT]; //store register pending login fds and address
    memset(&thread_args, 0, sizeof (thread_args));
    init_list();

    FD_SET(socket_fd, &read_sockets);

    //set timeout
    struct timeval timeout;
    timeout.tv_sec = 2;
    timeout.tv_usec = 500;

    for (int i = 0; i < FD_MAX_COUNT; i++){
        polling_data[i].fd = -1;
    }
    max_fd = polling_data[0].fd = socket_fd;
    unsigned short negativeIndex = 0;
    unsigned int onlie_fds_arr[FD_MAX_COUNT];
    int online_count = 0;

    while(true){
        FD_ZERO(&read_sockets);
        FD_ZERO(&write_sockets);
        FD_ZERO(&error_sockets);

        timeout.tv_sec = 2;
        timeout.tv_usec = 0;
        negativeIndex = 0;

        threads_count = 0;

        //remove -1 in fds center, T(n) need optimize
        for (int i = 0; i < max_fd; ++i){
            if (polling_data[i].fd < 0){
                if (thread_args[i].fd_ptr != NULL){
                    //remain register success fd
                    polling_data[i].fd = *thread_args[i].fd_ptr;
                    printf("thread fd:%d into.", polling_data[i].fd);
                    polling_data[i].address = thread_args[i].address;
                    free(thread_args[i].fd_ptr);
                    thread_args[i].fd_ptr = NULL;
                    FD_SET(polling_data[i].fd, &read_sockets);

                    continue;
                }
                for (int j = i + 1; j < max_fd; ++j){
                    if (polling_data[j].fd > 0){
                        polling_data[i].fd = polling_data[j].fd;
                        polling_data[j].fd = -1;
                        break;
                    }
                }
            }else{
//                if (fd_is_junk(polling_data[i].fd)){
//                    printf("the fd %d is junk\n", polling_data[i].fd);
//                    FD_CLR(polling_data[i].fd, &read_sockets);
//                    close(polling_data[i].fd);
//                    polling_data[i].fd = -1;
//                }else{
                    FD_SET(polling_data[i].fd, &read_sockets);
                    negativeIndex++;
                //}
            }
        }

        //add online user id, T(n) need optimize
        list_operate(onlie_fds_arr, &online_count, NULL);

        for (int i = 0;
             negativeIndex < (FD_MAX_COUNT - negativeIndex - 1) && (i < online_count);
             negativeIndex++, i++)
        {
//            try use pthread_detach, no block, emmmmmmmm, fail
//            int res = recv(onlie_fds_arr[i], NULL, 1, MSG_PEEK | MSG_DONTWAIT);
//            if (errno > 0) printf("MSG_PEEK ERR! line:%d errno:%d\n", __LINE__, errno);
//            if (onlie_fds_arr[i] > 0 /*&& res == 0*/){
//                FD_CLR(onlie_fds_arr[i], &read_sockets);
//                remove_user(onlie_fds_arr[i], false);
//            }else if (onlie_fds_arr[i] > 0) {

            //judge repetion
            bool repetion = false;
            for (int ii = 0; ii < negativeIndex; ++ii){
                if (onlie_fds_arr[i] == (unsigned int)polling_data[ii].fd){
                    repetion = true;
                    break;
                }
            }
            if (not repetion){
                polling_data[negativeIndex].fd = onlie_fds_arr[i];
                printf("fd:%d add into.\n", onlie_fds_arr[i]);
                FD_SET(polling_data[negativeIndex].fd, &read_sockets);
            }
        }

        max_fd = socket_fd;
        for (int i = 0; i < FD_MAX_COUNT; ++i){
            if (max_fd < polling_data[i].fd){
                max_fd = polling_data[i].fd;
            }
            printf("%d ", polling_data[i].fd);
        }
        printf("|MAX=%d",max_fd);
        puts("");

        switch (select(100 + 1, &read_sockets, NULL, NULL, &timeout) ) {
        case 0:         
            printf("polling select:%s\n", get_current_datetime_str());
            break; //no client connect
        case -1:
            perror("select ERROR!!!");
            //pause();
            exit(1);
        default:
            // > 0 success
            for (int i = 0; i < max_fd; i++){
                const int fd = polling_data[i].fd;
                if (fd < 0)
                    continue;

                if (fd == socket_fd && FD_ISSET(fd, &read_sockets)){
                    struct sockaddr_in client_address;
                    socklen_t len = sizeof (client_address);
                    int accept_fd = accept(socket_fd, (struct sockaddr *)(&client_address),&len);
                    if (accept_fd < 0){
                        perror("accept");
                        printf("error code: %d\n", errno);
                        return;
                    }else if (accept_fd == 0){
                        puts("no client connect.");
                        continue;
                    }else{
                        printf("connected:[fd:%d ip:%s port:%d]\n", accept_fd,
                               inet_ntoa(client_address.sin_addr),
                               htons(client_address.sin_port));

                        if (accept_fd > FD_MAX_COUNT){
                            puts("too many clinet");
                            remove_user(accept_fd, false);
                            break;
                        }

                        if (accept_fd > max_fd){
                            max_fd = accept_fd;
                        }
                        //accept_fd into fds array
                        for (int i = 0; i < FD_MAX_COUNT; ++i){
                            if(polling_data[i].fd < 0){
                                polling_data[i].fd = accept_fd;
                                polling_data[i].address = client_address;
                                break;
                            }
                        }
                    }
                }else if(fd > 3 && FD_ISSET(fd, &read_sockets)){
                    if (threads_count > FD_MAX_COUNT){
                        puts("server too busy!!!");
                        return;
                    }

                    printf("fd:%d to read\n", fd);
                    if (errno > 0) {
                        printf("ERR! line:%d errno:%d\n", __LINE__, errno);
                        polling_data[i].fd = -1;
                        //printf("fd:%d was Closed!\n",fd);
                        //insert_junkfd(fd);
                        remove_user(fd, false);
                        break;
                    }
                    pthread_t tid;
                    thread_args[i].fd_ptr = (int *)malloc(sizeof(int));
                    if (thread_args[i].fd_ptr == NULL){
                        perror("malloc");
                        break;
                    }
                    *thread_args[i].fd_ptr = fd;
                    thread_args[i].address = polling_data[i].address;

                    //create thread for handle client request
                    int err = pthread_create(&tid, NULL, handleRequest, (void *)&thread_args[i]);
                    if (err == 0){
                        printf("create thread successly. thread id: %lu\n", tid);
                        polling_data[i].fd = -1;
                        //FD_CLR(fds[i], &read_sockets); //seem sometimes occur login fail
                        thread_arr[threads_count++] = tid;
                    }else{
                        printf("create thread failed. error no: %d\n", err);
                    }
                    //pthread_detach(tid); //occur some problem, wo shi sha bi

                    if (errno > 0) printf("ERR! line:%d errno:%d\n", __LINE__, errno);
                }else if(fd > 0 && FD_ISSET(fd, &write_sockets)){
                    printf("%d to write\n", fd);
                }else if(fd > 0 && FD_ISSET(fd, &error_sockets)){
                    printf("%d to error\n", fd);
                }
            }

            //wait threads finished
            for (int i = 0; i < threads_count; i++){
                int *res = NULL;
                pthread_join(thread_arr[i], (void *)&res);
                printf("thread:%lu, exit:%d\n", thread_arr[i], *res);
            }
            break;
        }
        //close and clear junk fd
        destory_junk_list(polling_data, max_fd);
    }

}
