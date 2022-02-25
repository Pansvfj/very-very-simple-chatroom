#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "../Common/assist.h"
#include "../Common/protocol.h"

int main(int argc, char *argv[])
{
    while(0){
        int n = 0;
        scanf("%d", &n);
        printf("the num is %d\n", n);
        char buf[10];
        char *p = buf;
        p = my_itoa(n);
        printf("the str is %s\n", p);
    }

    while(0){
        char str[10];
        scanf("%s", &str);
        printf("the str is %s\n", str);
        int n = my_atoi(str);
        printf("the num is %d\n", n);
    }

    while (0){
    char s[100];
    get_current_datetime_str(s);
    printf("current date time: %s\n", s);
    sleep(1);
    }

    while (1){
        msg m;
        m.msg_type = 3;
        m.send_id = 5;
        m.receive_id = 8;
        //scanf("%s", m.msg_content);
        printf("the size is %d\n", sizeof(m.msg_content));
        fgets(m.msg_content, sizeof(m.msg_content), stdin);
        m.msg_datetime = get_unix_timestamp();

        printf("type:%d\nsend_id:%d\nreceive_id:%d\n"
               "content:%s\ndatetime:%d\n",
               m.msg_type, m.send_id, m.receive_id, m.msg_content, m.msg_datetime);

        printf("\n===>:\n%s\n", pack_msg(&m));
        printf("-------\n");
    }

    return 0;
}
