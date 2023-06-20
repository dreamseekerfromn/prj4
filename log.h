//resubmitting old file
#ifndef LOG_H
#define LOG_H
#include <time.h>

typedef struct data_struct {
	time_t time;
	char *string;
} data_t;

int addmsg (data_t data);
void clearlog (void);
char *getlog (void);
int savelog (char *filename);
void create_log(char *);

#endif /* LOG_H */
