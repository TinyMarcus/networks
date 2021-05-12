#include "server.h"

int main(int argc, char *arvg[]) {
    char *addr = ADDR;
    int port = PORT;
    int rc = 0;
    int client_socket_fd;
    char request[1024];
    char response[BUFFER_SIZE];

    // создание сокета
    // домен - AF_INET - для сетевого протокола IPv4
    // тип сокета - SOCK_STREAM - потоковый сокет
    // протокол - TCP
    client_socket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (client_socket_fd < 0) {
        perror("socket error");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in rem_addr;
    struct hostent *rem;

    // описание сервера
    rem_addr.sin_family = AF_INET;
    rem_addr.sin_port = htons(port);

    // конвертация имени хоста в IP-адрес
    rem = gethostbyname(addr);

    if (rem == NULL) {
        perror("gethostbyname error");
        exit(EXIT_FAILURE);
    }

    // установка соединения
    memcpy(&rem_addr.sin_addr, rem->h_addr, rem->h_length);
    rc = connect(client_socket_fd, (struct sockaddr*)&rem_addr, sizeof(rem_addr));

    if (rc < 0) {
        perror("connect error");
        exit(EXIT_FAILURE);
    }

    puts("Input a request to server: ");
    fgets(request, sizeof(request), stdin);

    // отправка запроса на сервер и получение ответа из сокета
    send(client_socket_fd, request, sizeof(request), 0);
    rc = recv(client_socket_fd, &response, sizeof(response), MSG_WAITALL);

    if (!rc) {
        perror("recv error");
    }

    // закрытие сокета (соединения)
    printf("\n%s\n", response);
    close(client_socket_fd);

    return 0;
}