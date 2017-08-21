#ifndef SEAPUNK_H
#define SEAPUNK_H

void seashell(void);
char *seapunk_read(void);
char *seapunk_time(void);
char **seapunk_split(char *line);
int seashell_execute(char **arguments);
int seapunk_do(char **args);

#endif
