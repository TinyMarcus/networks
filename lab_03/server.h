#ifndef __SERVER_H__
#define __SERVER_H__

#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#include "list_fd.h"

#define BUFFER_SIZE 128000
#define RFC1123FMT "%a, %d %b %Y %H:%M:%S GMT"

#define ADDR "0.0.0.0"
#define PORT 8080

char date[128];
char root_dir[128];
char stats[1024];
int server_socket_fd = 0;
node_t* head = NULL;

pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

void update_stats(void);
char *get_cur_time(char *, int);
void *client_handler(void *);

#endif