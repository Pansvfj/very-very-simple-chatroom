#include "assist.h"
#include "common_defines.h"


#include <math.h> //gcc add -lm
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

char* my_itoa(int num)
{
   static char result[10];
   if (num == 0){
       result[0] = '0';
       result[1] = '\0';
       return result;
   }

   char str[10];
   int natural_num =  (num >= 0) ? num : 0 - num;
   int i = 0;

   do{
       str[i] = natural_num % 10 + '0';
       natural_num /= 10;
       i++;
   }while(natural_num);

   int j = 0;
   if (num < 0){
       result[j] = '-';
       j++;
   }

   //reverse
   while (i > 0){
       result[j] = str[i - 1];
       j++;
       i--;
   }
   result[j] = '\0';

   return result;
}

int my_atoi(const char *str)
{
    if (str == NULL){
        return 0;
    }
    int len = strlen(str);
    //printf("len = %d\n", len);
    if (len == 0 || len > 10){
        assert(len != 0);
        return 0;
    }
    if (len > 10){
        assert(len < 10);
        return 0;
    }
    int i = 0;
    if (str[i] == '-'){
        i++;
    }

    int result = 0;
    for (; i < len; i++){
        result += (str[i] - '0') * (int)pow(10, len - i - 1);
    }

    if (str[0] == '-'){
        result = 0 - result;
    }
    return result;
}

unsigned int get_unix_timestamp()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec;
}

char* get_current_datetime_str()
{
//    if (out == NULL)
//        return;
    static char str[DATE_LEN];

    time_t utc_time;
    struct tm *timenow;

    time(&utc_time);
    //timenow = gmtime(&utc_time); //0 time zone
    timenow = localtime(&utc_time); // local time zone

    sprintf(str, "%d-%02d-%02d %02d:%02d:%02d", timenow->tm_year + 1900,
            timenow->tm_mon + 1,timenow->tm_mday,
            timenow->tm_hour, timenow->tm_min,timenow->tm_sec);

    return str;
}

void logc(const char* str, char c)
{
    printf("%s:%c\n",str,c);
}

void logi(const char* str, int i)
{
    printf("%s:%d\n",str, i);
}

void logs(const char* str, const char *s)
{
    printf("%s:%s\n",str, s);
}
