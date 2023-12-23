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
    char buffer[BUFFER_SIZE] = {0};

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
    } else {
        write(STDOUT_FILENO, "Соединение успешно\n", sizeof("Соединение успешно\n") - 1);
    }

    // Отправка сообщения серверу
    ssize_t отправлено_байт = send(sock, сообщение, strlen(сообщение), 0);
    if (отправлено_байт < 0) {
        perror("Ошибка отправки");
        close(sock);
        exit(EXIT_FAILURE);
    }
    write(STDOUT_FILENO, "Отправлено сообщение\n", sizeof("Отправлено сообщение\n") - 1);

    // Получение ответа от сервера
    ssize_t прочитано_байт = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (прочитано_байт > 0) {
        buffer[прочитано_байт] = '\0';  // Нуль-терминирование принятых данных
        write(STDOUT_FILENO, "Получен ответ от сервера: ", sizeof("Получен ответ от сервера: ") - 1);
        write(STDOUT_FILENO, buffer, strlen(buffer));
        write(STDOUT_FILENO, "\n", sizeof("\n") - 1);
    } else if (прочитано_байт == 0) {
        write(STDOUT_FILENO, "Сервер закрыл соединение\n", sizeof("Сервер закрыл соединение\n") - 1);
    } else {
        perror("Ошибка приема");
    }

    close(sock);

    return 0;
}
