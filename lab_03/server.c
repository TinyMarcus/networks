#include "server.h"

void catch_sigint(int signum) {
    close(server_socket_fd);
    free_lst(&head);
    exit(1);
}

// взятие текущего времени из системы
char* get_cur_time(char *str, int len) {
    time_t t = time(NULL);
    struct tm cur_time;
    strftime(str, len, RFC1123FMT, localtime_r(&t, &cur_time));
    return str;
}

// обновление файла stats со статистикой посещений
void update_stats(void) {
    char str[1024] = {0};
    time_t t = time(NULL);
    FILE *file = NULL;
    struct tm res;
    static int days[7] = {0}, hours[24] = {0}, all_requests = 0;
    char buf[1024];
    const char *weekdays[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    localtime_r(&t, &res);

    hours[res.tm_hour] += 1;
    days[res.tm_wday] += 1;
    all_requests += 1;

    snprintf(buf, sizeof(buf), "HOUR  COUNT OF REQUESTS  PERS OF REQUESTS\n");
    strcat(str, buf);
    memset(buf, '\0', sizeof(buf));

    for (int i = 0; i < 24; i++) {
        snprintf(buf, sizeof(buf), "%d            %d          %g %% \n", i, hours[i], hours[i] / (double)all_requests * 100);
        strcat(str, buf);
        memset(buf, '\0', sizeof(buf));
    }

    snprintf(buf, sizeof(buf), "DAY  COUNT OF REQUESTS  PERS OF REQUESTS\n");
    strcat(str, buf);
    memset(buf, '\0', sizeof(buf));

    for (int i = 0; i < 7; i++) {
        snprintf(buf, sizeof(buf), "%s            %d           %g %% \n", weekdays[i], days[i], days[i] / (double)all_requests * 100);
        strcat(str, buf);
        memset(buf, '\0', sizeof(buf));
    }

    file = fopen("stats", "w");
    fprintf(file, "%s\n", str);
    fclose(file);
}

// обработчик клиента
void *client_handler(void *argument) {
    node_t *p, **list_head = NULL;
    char buffer[BUFFER_SIZE];
    char http_header[BUFFER_SIZE];
    int client_socket_fd;
    char dir[1024];
    char request[1024];
    char path[1024];
    char method[3];
    int fd, all_pages = 0, all_bytes = 0;
    
    while (1) {
        printf("Thread %lu\n", pthread_self());

        // блокировка мьютекса потоком клиента перед работой с данными
        // используется быстрый мьютекс (происходит ожидание пока поток сам не освободит себя)
        pthread_mutex_lock(&mtx);
        list_head = (node_t**)argument;

        // ожидание пока очередь не будет непустой
        while (*list_head == NULL) {
            pthread_cond_wait(&cond, &mtx);
        }
        p = pop(list_head);
        client_socket_fd = p->fd;

        printf("Thread %lu started...\n", pthread_self());
        
        // разблокировка мьютекса и получение запроса от клиента
        pthread_mutex_unlock(&mtx);
        recv(client_socket_fd, &request, sizeof(request), 0);
        sscanf(request, "%s%s", method, path);

        // обновление статистики
        update_stats();

        if (strcmp(method, "GET") && strcmp(method, "STATS")) {
            snprintf(http_header, sizeof(http_header), "Usage: GET </siteA/pageA_B.html> OR STATS\n");
            send(client_socket_fd, http_header, sizeof(http_header), 0);
            memset(http_header, '\0', sizeof(http_header));
            memset(path, '\0', sizeof(path));
            memset(method, '\0', sizeof(method));
            continue;
        }
        
        if (!strcmp(method, "STATS")) {
            if ((fd = open("stats" , O_RDONLY)) != -1) {
                read(fd, http_header, sizeof(http_header));
                close(fd);
            }
            send(client_socket_fd, http_header, sizeof(http_header), 0);
            memset(http_header, '\0', sizeof(http_header));
            memset(path, '\0', sizeof(path));
            memset(method, '\0', sizeof(method));
            continue;
        }

        strcpy(dir, root_dir);
        strcat(dir, path);

        // отправка данных клиенту (если что-то не так - файла нет и т.д.)
        if ((fd = open(dir, O_RDONLY)) == -1) {
            if (access(dir, R_OK) != 0) {
                if (errno == EACCES) {
                    snprintf(http_header, sizeof(http_header), 
                                 "\nHTTP/1.1 403 Forbidden\r\n"
		                         "Date: %s\r\n"
		                         "Content-Type: text/html\r\n"
		                         "Connection: Closed\r\n\r\n",
		                         get_cur_time(date, sizeof(date)));
                    send(client_socket_fd, http_header, sizeof(http_header), 0);

                    printf("File \"%s\" is not accessible\n", dir);
                    memset(http_header, '\0', sizeof(http_header));
                    memset(dir, '\0', sizeof(dir));
                    memset(path, '\0', sizeof(path));
                } else {
                    snprintf(http_header, sizeof(http_header), 
                                 "\nHTTP/1.1 404 Not Found\r\n"
		                         "Date: %s\r\n"
		                         "Content-Type: text/html\r\n"
		                         "Connection: Closed\r\n\r\n",
		                         get_cur_time(date, sizeof(date)));
                    send(client_socket_fd, http_header, sizeof(http_header), 0);

                    printf("File \"%s\" does not exist\n", dir);
                    memset(http_header, '\0', sizeof(http_header));
                    memset(dir, '\0', sizeof(dir));
                    memset(path, '\0', sizeof(path));
                }
            }
            // блокировка мьютекса, закрытие сокета и освбождение места в очереди
            pthread_mutex_lock(&mtx);
            close(client_socket_fd);
            free(p);
            pthread_mutex_unlock(&mtx);
            continue;
        }

        // запись данных в буфер для дальнейшей передачи
        read(fd, buffer, sizeof(buffer));
		
        // если данные нашлись, то отправка, что все ОК
        snprintf(http_header, sizeof(http_header), 
                     "\nHTTP/1.1 200 OK\r\n"
		             "Date: %s\r\n"
		             "Content-Type: text/html\r\n"
		             "Content-Length: %ld\r\n"
		             "Connection: Closed\r\n\r\n",
		             get_cur_time(date, sizeof(date)),
		             strlen(buffer));

        // отправка данных клиенту
        strcat(http_header, buffer);
        send(client_socket_fd, http_header, sizeof(http_header), 0);
        printf("File \"%s\" has been sent to client\n", dir);

        memset(http_header, '\0', sizeof(http_header));
        memset(dir, '\0', sizeof(dir));
        memset(path, '\0', sizeof(path));

        // блокировка мьютекса, закрытие сокета и освбождение места в очереди
        pthread_mutex_lock(&mtx);
        close(client_socket_fd);
        free(p);
        pthread_mutex_unlock(&mtx);
    }

    // завершение вызывающего потока
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    int opt = 0, serving_port = PORT, num_of_threads = 0, client_socket_fd, flags = 0;
    struct sockaddr_in address, caddr;
    socklen_t clientlen;
    node_t *list_node;
    fd_set set;
    head = NULL;
    
    // считывание аргументов
    while ((opt = getopt(argc, argv, "t:d:")) != -1) {
        switch(opt) {
            case 't':
                num_of_threads = atoi(optarg);
                break;
            case 'd':
                strcpy(root_dir, optarg);
                break;
        }
    }

    // выделение памяти под потоки
    pthread_t *thread_ids = malloc(num_of_threads * (sizeof(pthread_t)));

    // создание пула потоков
    for (int i = 0 ; i < num_of_threads ; i ++) {
        pthread_create(&thread_ids[i], NULL, client_handler, &head);
    }
    
    // создание сокета
    // домен - AF_INET - для сетевого протокола IPv4
    // тип сокета - SOCK_STREAM - потоковый сокет
    // протокол - TCP
    if ((server_socket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        perror("Error creating socket");
        return EXIT_FAILURE;
    }
    
    signal(SIGINT, catch_sigint);
    clientlen = sizeof(caddr);
    
    // описание сервера
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(serving_port);
    flags = fcntl(server_socket_fd, F_GETFL, 0);
    fcntl(server_socket_fd, F_SETFL, flags | O_NONBLOCK);
    FD_ZERO(&set);
    FD_SET(server_socket_fd, &set);
         
    printf("ADDRESS %s:%d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
    
    // привязка сокета к адресу и порту
    if (bind(server_socket_fd, (struct sockaddr *) &address, sizeof(address)) < 0) {
        perror("bind error");
        exit(EXIT_FAILURE);
    }
    
    // прослушка порта - объявление о готовности принимать соединения
    if (listen(server_socket_fd, 6) < 0) {
        perror("listen error");
        exit(EXIT_FAILURE);
    }
        
    while (1) {
        sleep(1);
        puts("Waiting for connections...");

        if (select(server_socket_fd + 1, &set, NULL, NULL, NULL) <= 0) {
            perror("select error");
            exit(EXIT_FAILURE);
        }
                     
        if (FD_ISSET(server_socket_fd, &set)) {      
            clientlen = sizeof(caddr);
            if ((client_socket_fd = accept(server_socket_fd, (struct sockaddr *) &caddr, &clientlen)) < 0) {
                perror("accept error");
                exit(EXIT_FAILURE);
            }
            getsockname(client_socket_fd, (struct sockaddr *) &caddr, &clientlen);
            printf("Client connected to port: %d\n", ntohs(caddr.sin_port));
        }
        
        // добавление клиента в очередь при подключении
        pthread_mutex_lock(&mtx);
        push(&head, client_socket_fd);
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mtx);
    }

    // ждет завершения выполнения всех потоков-клиентов
    for (int i = 0 ; i < num_of_threads ; i ++) {
        pthread_join(thread_ids[i], NULL);
    }
    
    // освобождение места под очередь и закрытие сокета
    free_lst(&head);
    close(server_socket_fd);
    
    return 0;
}