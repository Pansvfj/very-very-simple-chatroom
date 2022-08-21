#include "databases_manager.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

//data.txt
//id password username
//server uid is 1,other user >= 2, -1 is invalid

bool init_db()
{
    if (access("datebases", F_OK)== 0){
        puts("db folder alread exists.");
    }else {
        if (mkdir("datebases", S_IRWXU) == 0){
            puts("create db folder success");
        }else{
            perror("create databases folder");
        }
    }
    int ret = open("./datebases/data.txt", O_RDWR | O_CREAT, S_IRWXU);
    if (ret < 0){
        perror("create db file");
        return false;
    }else {
        puts("-------created db file success.-------");
    }
    return true;
}

int create_new_id(const char* username, const char* passwd)
{
    //TODO add mutex

    puts(__FUNCTION__);
    FILE* fd = fopen("./datebases/data.txt", "rw+");
    if (fd == NULL){
        perror("create_new_id fopen");
        return -1;
    }else{
        char buf[MAX_BUF_LEN];
        int len = 0;
        while(fgets(buf, MAX_BUF_LEN, fd)){
            len = strlen(buf);
            if (len == 0){
                //first create db
                break;
            }
            for (int i = 0; i < len; i++){
                if (buf[i] == ' '){
                    buf[i] = '\0';
                    break;
                }
            }
        }
        int uid = -1;
        if (len == 0){
            uid = 2;
        }else if (len > 0){
            uid = my_atoi(buf) + 1;
        }else
            perror(__FUNCTION__);
        char* uid_str = my_itoa(uid);
        fprintf(fd, "%s %s %s\n", uid_str, passwd, username);
        fflush(fd); //why?
        fclose(fd);
        return uid;
    }
}

bool check_password(unsigned int checkid, const char check_passwd[], int checkpasswd_len)
{
    //TODO add mutex

    puts(__FUNCTION__);
    FILE* fp = fopen("./datebases/data.txt", "rw+");
    if (fp == NULL){
        perror("check_password fopen");
        return -1;
    }else{
        char buf[MAX_BUF_LEN];
        int len = 0;
        int orign_id = -1;
        char origin_passwd[PASSWD_LEN];
        while(fgets(buf, MAX_BUF_LEN, fp)){
            len = strlen(buf);
            for (int i = 0; i < len; i++){
                if (buf[i] == ' ') {
                    buf[i] = '\0';
                    orign_id = my_atoi(buf);
                    assert(orign_id > 0);
                    if (checkid == (unsigned int)orign_id){
                        i++;//start passwd location
                        for (int j = 0; j < PASSWD_LEN; j++, i++){
                            if (buf[i] == ' '){
                                origin_passwd[j] = '\0';
                                break;
                            }
                            origin_passwd[j] = buf[i];
                        }
                        break;
                    }
                    break;
                }
            }
        }
        //checkout passwd
        int passwd_len = strlen(origin_passwd);
        if (passwd_len != checkpasswd_len){
            fclose(fp);
            return false;
        }

        for (int i = 0 ; i < passwd_len; ++i){
            if (check_passwd[i] != origin_passwd[i]){
                fclose(fp);
                return false;
            }
        }

        fclose(fp);
        return true;
    }
}

const char* get_dbname_from_dbid(unsigned int id)
{
    FILE* fp = fopen("./datebases/data.txt", "rw+");
    if (fp == NULL){
        perror("get_name_from_id,fopen");
        return NULL;
    }else{
        char buf[MAX_CONTENT_LEN];
        while (fgets(buf, MAX_BUF_LEN, fp)){
            int len = strlen(buf);
            for(int i = 0; i < len; ++i){
                if (buf[i] == ' '){
                    buf[i] = '\0';
                    int origin_id = my_atoi(buf);
                    i++;
                    if ((unsigned int)origin_id == id){
                        static char name[NAME_LEM];
                        for (int j = i; j < len; ++j){
                            if (buf[j] == ' '){
                                int kk = 0;
                                j++;
                                for (int k = j; k < len; ++k, ++kk){
                                    if (buf[k] == '\n') break;
                                    name[kk] = buf[k];
                                }
                                name[kk] = '\0';
                                fclose(fp);
                                return name;
                            }
                        }
                    }else{
                        break;
                    }
                }
            }
        }

    }
    fclose(fp);
    return NULL;
}



