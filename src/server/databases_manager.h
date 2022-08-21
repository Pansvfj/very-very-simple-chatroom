#ifndef DATABASES_MANAGER_H
#define DATABASES_MANAGER_H

#include "../common/assist.h"
#include "../common/common_defines.h"

extern bool init_db();
extern int create_new_id(const char* username, const char* passwd);
extern bool check_password( unsigned int checkid, const char check_passwd[], int checkpasswd_len);
extern const char* get_dbname_from_dbid( unsigned int id);

#endif // DATABASES_MANAGER_H
