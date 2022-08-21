#ifndef ASSIST_H
#define ASSIST_H

extern char* my_itoa(int num);

extern int my_atoi(const char *str);

extern unsigned int get_unix_timestamp();

//extern void get_current_datetime_str(char* out);
extern char* get_current_datetime_str();

extern void logc(const char* str, char c);
extern void logi(const char* str, int i);
extern void logs(const char* str, const char *s);

#endif // ASSIST_H
