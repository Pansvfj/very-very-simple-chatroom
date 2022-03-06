#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include<arpa/inet.h>

#include "../Common/protocol.h"
#include "../Common/structs.h"

int main(int argc, char **agrv)
{
    char buf[MAX_BUF_LEN];
    struct sockaddr_in server_address;
    //init server address
    bzero(&server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr);
    server_address.sin_port = htons(10086);

    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    int connect_fd = connect(socket_fd,
                             (struct sockaddr*)(&server_address), sizeof(server_address));


    int n = 0;
    printf("connect: %d\n",connect_fd);
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
        write(socket_fd, buf, len - 1);
        printf("send data--->%s\n", buf);
        memset(buf, '\0', MAX_BUF_LEN);
        n = read(socket_fd, buf, MAX_BUF_LEN);
        printf("receive data--->%s\n", buf);
        //write(STDOUT_FILENO, buf, n);
    }

    return 0;
}
