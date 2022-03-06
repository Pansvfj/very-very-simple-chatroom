#ifndef ASSIST_H
#define ASSIST_H

char* my_itoa(int num);

int my_atoi(char *str);

unsigned int get_unix_timestamp();

void get_current_datetime_str(char* out);



//I need bool type in C language
//---------------------------------
#ifndef bool
#define bool int
#endif

#ifndef true
#define true 1
#endif

#ifndef false
#define false 0
#endif
//---------------------------------

#endif // ASSIST_H
