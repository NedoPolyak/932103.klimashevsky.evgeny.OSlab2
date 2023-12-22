#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8085
#define BUFFER_SIZE 1024

int main() {
    struct sockaddr_in serverAddress;
    int sock = 0;
    char* сообщение = "Сообщение от клиента";
    char buffer[BUFFER_SIZE] = { 0 };

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Ошибка при создании сокета");
        exit(EXIT_FAILURE);
    }

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT);

    // Преобразование адреса в бинарную форму
    if (inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr) <= 0) {
        perror("Ошибка: адрес не поддерживается");
        exit(EXIT_FAILURE);
    }

    if (connect(sock, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        perror("Ошибка подключения");
        exit(EXIT_FAILURE);
    }
    else {
        printf("Соединение успешно\n");
    }

    // Отправка сообщения серверу
    ssize_t отправлено_байт = send(sock, сообщение, strlen(сообщение), 0);
    if (отправлено_байт < 0) {
        perror("Ошибка отправки");
        close(sock);
        exit(EXIT_FAILURE);
    }
    printf("Отправлено сообщение: %s\n", сообщение);

    // Получение ответа от сервера
    ssize_t прочитано_байт = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (прочитано_байт > 0) {
        buffer[прочитано_байт] = '\0';  // Нуль-терминирование принятых данных
        printf("Получен ответ от сервера: %s\n", buffer);
    }
    else if (прочитано_байт == 0) {
        printf("Сервер закрыл соединение\n");
    }
    else {
        perror("Ошибка приема");
    }

    close(sock);

    return 0;
}