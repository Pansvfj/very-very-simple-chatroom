#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>

#include "../Common/structs.h"
#include "../Common/assist.h"
#include "../Common/protocol.h"

int main(int argc, char *argv[])
{
    struct sockaddr_in server_address;
    struct sockaddr_in client_address;
    socklen_t client_address_len;
    int listen_fd;
    int connect_fd;
    char buf[MAX_BUF_LEN];
    char str[INET_ADDRSTRLEN]; //netinet/in.h  INET_ADDRSTRLEN = 16
    int i;
    int n;

    //call socket()
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof (opt));

    //init server address struct
    bzero(&server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(10086);

    //bind
    bind (listen_fd, (struct sockaddr *)(&server_address), sizeof(server_address));

    //listen
    listen(listen_fd, 1000);

    puts("wait client connect...");

    bzero(buf, MAX_BUF_LEN);
    client_address_len = sizeof(client_address);
    //acccept block....
    puts("------acccept block------");




    while (true) {
        bzero(buf, MAX_BUF_LEN);
        connect_fd = accept(listen_fd,
                            (struct sockaddr*)(&client_address),
                            &client_address_len);
        if (connect_fd > 0)
            printf("have connect coming... fd: %d\n", connect_fd);
        else
            continue;

        int res = fork();
        if (res == -1) {
            perror("fork error!!!!!!!!!!!");
        }else if (res == 0){
            while (true){
                //read data from client
                n = read(connect_fd, buf, MAX_BUF_LEN);
                printf("errno: %d\n", errno);
                if (n == 0) {
                    printf("fd: %d exit.\n", connect_fd);
                    break; //client login out
                }
                if (n < 0 && errno == EINTR) {
                    puts("system Interrupted system call");
                    continue;
                }
                printf("data: ");
                for (i = 0; i < n; i++){
                    printf("%c",buf[i]);
                }
                printf("\n");

                printf("received from ip:%s port: %d\n",
                       inet_ntop(AF_INET, &client_address.sin_addr, str, sizeof(str)),
                       ntohs(client_address.sin_port));

                for (i = 0; i < n; i++){
                    buf[i] = toupper(buf[i]);
                }
                write(connect_fd, buf, n);
            }

        }
        //close(connect_fd);
    }

    return 0;
}
