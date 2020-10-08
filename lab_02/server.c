// Серверная реализация модели клиент-сервер UDP
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
#define FAILED -1
#define SUCCESS 0

int convert(int number, int base)
{
    int res_number = 0, i = 1;

    while (number != 0)
    {
        res_number += (number % base) * i;
        number /= base;
        i *= 10;
    }

    printf("%d-ый формат: %d\n", base, res_number);
    return res_number;
}

int main() {
    int sock;
    char buffer[LEN];
    
    struct sockaddr_in servaddr, cliaddr;

    // Создание дескриптора файла сокета
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) 
    {
        perror("Socket creation error");
        return FAILED;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    // Заполнение информации о сервере
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);

    // Привязываем сокет с адресом сервера
    if (bind(sock, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        perror("Binding error");
        return FAILED;
    }

    int len, n;
    len = sizeof(cliaddr);
  
    while (1)
    {
        n = recvfrom(sock, (char *)buffer, LEN, 
                    MSG_WAITALL, ( struct sockaddr *) &cliaddr,
                    &len);

        buffer[n] = '\0';
        
        if (strcmp(buffer, "stop") == 0)
        {
            break;
        }

        int num_dec, num_oct, num_hex, num_bin, num_six;
        num_dec = atoi(buffer);
        num_oct = num_hex = num_bin = num_six = num_dec;

        printf("10-ый формат: %d\n", num_dec);
        printf("8-ый формат: %o\n", num_oct);
        printf("16-ый формат: %x\n", num_hex);

        num_bin = convert(num_dec, 2);
        num_six = convert(num_dec, 6);

        printf("\n");
    }
    
    close(sock);
    return SUCCESS;
}