#ifndef _LIST_OPERATION_H_
#define _LIST_OPERATION_H_

//The file I want to do template operation, such as message and other list
//but, Not enough time and I want to only use C but no C plus plus
//so the file operates online user list directly.

#include "../common/structs.h"
#include <netinet/in.h>

extern void init_list();

extern void insert_junkfd(unsigned int fd);
extern void destory_junk_list(polling_fds_addrs arr[], int maxfd);
extern bool fd_is_junk(unsigned int fd);

//here, need use C plus plus template<T> next time.
extern void modify_fd(unsigned int uid, int fd);
extern void modify_addr(unsigned int uid, struct sockaddr_in addr);

extern bool user_is_online(unsigned int id_fd, bool is_id);
extern const char* get_name_from_id(unsigned int id);
extern int get_fd_from_id(unsigned int id);
extern void add_user(int fd, unsigned int uid, const char*uname, int namelen, struct sockaddr_in addr);
extern void remove_user(unsigned int id_fd, bool is_id);

extern void list_operate(unsigned int fds[], int *count, void (*func)(online_users_node*));

#endif // _LIST_OPERATION_
