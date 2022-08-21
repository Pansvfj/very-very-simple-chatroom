#include "list_operation.h"
#include "../common/assist.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

static online_users_node *g_head = NULL;
static online_users_node *g_tail = NULL;
bool init_finished = false;

typedef struct junk_struct
{
    int fd;
    struct junk_struct *next;
}junk_S;

static junk_S junkhead = {
    .fd = 0,
    .next = NULL
};

pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

void init_list()
{
    if (init_finished) return;
    g_head = (online_users_node*)malloc(sizeof(online_users_node));
    g_head->fd = -1;
    g_head->user_id = SERVERID;
    strcpy(g_head->user_name, "SERVER");
    g_head->pre = g_head->next = NULL;
    g_tail = g_head;
    init_finished = true;
}

void insert_junkfd(unsigned int fd)
{
    puts(__FUNCTION__);
    printf("%s, fd:%d\n", __FUNCTION__, fd);
    junk_S* find_null_jnode = junkhead.next;
    junk_S* last_jnode = NULL;
    while (find_null_jnode != NULL){
        if ((int)fd == find_null_jnode->fd){
            printf("the fd(%d) alread in junkfds\n", fd);
            continue;
        }
        last_jnode = find_null_jnode;
        find_null_jnode = find_null_jnode->next;
    }
    junk_S* new_jnode = (junk_S*)malloc(sizeof(junk_S));
    new_jnode->fd = fd;
    new_jnode->next = NULL;
    if (last_jnode == NULL){
        junkhead.next = new_jnode;
    }else{
        last_jnode->next = new_jnode;
    }
}

void clear_link()
{
    while(junkhead.next){
        junk_S *del = junkhead.next;
        if (junkhead.next->fd > 0){
            close(junkhead.next->fd);
        }
        printf("Fd:%d was Closed\n", junkhead.next->fd);
        junkhead.next = junkhead.next->next;
        free(del);
        del = NULL;
    }
}

void destory_junk_list(polling_fds_addrs arr[], int maxfd) //remain head node
{
    puts(__FUNCTION__);
    junk_S* start = junkhead.next;
    printf("junk fds:");
    while (start){
        printf("%d ", start->fd);
        start = start->next;
    }
    puts("");

    for (int i = 0; i < maxfd; ++i){
        if (arr[i].fd > 0){
            junk_S *node = junkhead.next;
            while (node){
                if (node->fd == arr[i].fd){
                    arr[i].fd = -1;
                    break;
                }
                node = node->next;
            }
        }
    }
    clear_link();
}

bool fd_is_junk(unsigned int fd)
{
    junk_S* start = junkhead.next;
    while (start){
        if (start->fd == (int)fd){
            return true;
        }
        start = start->next;
    }
    return false;
}

void modify_fd(unsigned int uid, int fd)
{
    if (not init_finished) return;
    online_users_node* node = g_head->next;
    while(node != NULL){
        if (uid == node->user_id){
            node->fd = fd;
            return;
        }
        node = node->next;
    }
}

void modify_addr(unsigned int uid, struct sockaddr_in addr)
{
    if (not init_finished) return;
    online_users_node* node = g_head->next;
    while(node != NULL){
        if (uid == node->user_id){
            node->address = addr;
            return;
        }
        node = node->next;
    }
}

bool user_is_online(unsigned int id_fd, bool is_id)
{
    if (not init_finished) return false;
    online_users_node* node = g_head->next;
    while(node != NULL){
        if (id_fd == (is_id ? node->user_id : node->fd)){
            return true;
        }
        node = node->next;
    }
    return false;
}

const char* get_name_from_id(unsigned int id)
{
    if (not init_finished) return NULL;
    online_users_node* node = g_head->next;
    while(node != NULL){
        if (id == node->user_id){
            return node->user_name;
        }
        node = node->next;
    }
    return NULL;
}

int get_fd_from_id(unsigned int id)
{
    if (not init_finished) return -1;

    online_users_node* node = g_head->next;
    while(node != NULL){
        if (id == node->user_id){
            return node->fd;
        }
        node = node->next;
    }
    return -1;
}

void add_user(int fd, unsigned int uid, const char*uname, int namelen, struct sockaddr_in addr)
{
    puts(__FUNCTION__);
    if (not init_finished) return;
    if (namelen > NAME_LEM - 1){
        puts("Name length too long.");
        return;
    }

    pthread_mutex_lock(&g_mutex);

    if (g_head == g_tail){
        g_tail = (online_users_node*)malloc(sizeof(online_users_node));

        g_tail->fd = fd;
        g_tail->user_id = uid;
        strncpy(g_tail->user_name, uname, namelen);
        g_tail->user_name[namelen] = '\0';
        g_tail->address = addr;

        g_tail->pre = g_head;
        g_tail->next = NULL;
        g_head->next = g_tail;
    }else{
        //check repetition, Squeeze out the previous login
        //remebere: don't close mutiple times fd, it make server errno9, clinet errno104
        if (user_is_online(uid, true)){
            insert_junkfd(get_fd_from_id(uid));   //shit!here make select errno 9
            printf("insert_junkfd %d\n", get_fd_from_id(uid));
            modify_fd(uid, fd);  //replace new fd
            modify_addr(uid, addr);

            puts("user alread exist.");
            pthread_mutex_unlock(&g_mutex);//shit, remeber unlock!
            return;
        }

        //append at tail
        online_users_node* new_node = (online_users_node*)malloc(sizeof(online_users_node));
        new_node->fd = fd;
        new_node->user_id = uid;
        new_node->address = addr;
        strncpy(new_node->user_name, uname, namelen);
        new_node->user_name[namelen] = '\0';
        g_tail->next = new_node;
        new_node->pre = g_tail;
        new_node->next = NULL;
        g_tail = new_node;
    }
    pthread_mutex_unlock(&g_mutex);
}

void remove_user(unsigned int id_fd, bool is_id)
{
    puts(__FUNCTION__);
    if (not init_finished) return;

    pthread_mutex_lock(&g_mutex);

    online_users_node* to_delete = NULL;
    if (id_fd == (is_id ? g_tail->user_id : g_tail->fd)){
        to_delete = g_tail;
        g_tail->pre->next = NULL;
        g_tail = g_tail->pre;
        insert_junkfd(is_id ? get_fd_from_id(id_fd) : id_fd);
        printf("%s:%d was removed.\n",(is_id ? "id" : "fd") , id_fd);

        free(to_delete);
        to_delete = NULL;
        pthread_mutex_unlock(&g_mutex);
        return;
    }

    to_delete = g_head->next;
    while(to_delete != NULL){
        if (id_fd == (is_id ? to_delete->user_id : to_delete->fd)){
            if (to_delete->pre != NULL){
                to_delete->pre->next = to_delete->next;
                assert(to_delete->next != NULL);//node->next == NULL should is tail node
                to_delete->next->pre = to_delete->pre;
                insert_junkfd(is_id ? get_fd_from_id(id_fd) : id_fd);
                printf("%s:%d was removed.\n",(is_id ? "id" : "fd") , id_fd);

                free(to_delete);
                to_delete = NULL;
                pthread_mutex_unlock(&g_mutex);
                return;
            }
        }
        to_delete = to_delete->next;
    }
    insert_junkfd(is_id ? get_fd_from_id(id_fd) : id_fd);
    printf("%s:%d CAN'T find.\n",(is_id ? "id" : "fd") , id_fd);

    pthread_mutex_unlock(&g_mutex);
}

void list_operate(unsigned int fds[], int *count, void (*func)(online_users_node*))
{
    puts(__FUNCTION__);
    if (not init_finished) return;

    pthread_mutex_lock(&g_mutex); //lock
    int i = 0;
    printf("online list   :------------------------------\n");
    online_users_node* node = g_head->next;
    while(node != NULL){
        printf("[id:%d,fd:%d,name:%s,ip:%s,port:%d]\n",
               node->user_id, node->fd, node->user_name,
               inet_ntoa(node->address.sin_addr), htons(node->address.sin_port));
        if (fds != NULL) fds[i] = node->fd;
        if (func != NULL) func(node);

        node = node->next;
        i++;
    }
    printf("list total(%02d):------------------------------\n", i);
    if (count != NULL) *count = i;

    pthread_mutex_unlock(&g_mutex);//unlock
}

void modify_user_status()
{
    //TODO
}
