// Клиентская реализация модели клиент-сервер UDP
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define PORT 8080
#define LEN 128
#define MSG_CONFIRM 0
#define FAILED -1
#define SUCCESS 0
  
// Код драйвера
int main() {
    int sock;
    char* message[LEN];
    struct sockaddr_in servaddr;

    // Создание дескриптора файла сокета
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
    {
        perror("Socket creation failed");
        return FAILED;
    }

    memset(&servaddr, 0, sizeof(servaddr));

    // Заполнение информации о сервере
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    servaddr.sin_addr.s_addr = INADDR_ANY;

    int n, len;

    while (1)
    {
        printf("Введите целое положительное число: ");
        scanf("%s", message);
        
        if (atoi(message) < 0)
        {
            printf("Введите положительное целое число!\n");
        }
        else
        {
            break;
        }
    }
    
    sendto(sockfd, (const char *)message, strlen(message),
           MSG_CONFIRM, (const struct sockaddr *) &servaddr, 
           sizeof(servaddr));

    printf("Число отправлено!\n");
    
    close(sock);
    return SUCCESS;
}