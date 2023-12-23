#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>

#define PORT 8085
#define MAX_CONNECTIONS 5
#define BUFFER_SIZE 1024

volatile sig_atomic_t wasSigHup = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void sigHupHandler(int signal) {
    pthread_mutex_lock(&mutex);
    wasSigHup = 1;
    pthread_mutex_unlock(&mutex);
    
    write(STDOUT_FILENO, "SIGHUP received.\n", sizeof("SIGHUP received.\n") - 1);
}

void *cleanupThread(void *arg) {
    int *socket_ptr = (int *)arg;
    sleep(1); // Даем основному потоку время на обработку клиентов
    close(*socket_ptr);
    free(socket_ptr);
    pthread_exit(NULL);
}

int main() {
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    // Создание сокета
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Ошибка при создании сокета");
        exit(EXIT_FAILURE);
    }

    // Настройка адреса сервера
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Привязка сокета к адресу и порту
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Ошибка при привязке сокета");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Прослушивание входящих соединений
    if (listen(server_socket, MAX_CONNECTIONS) == -1) {
        perror("Ошибка при прослушивании сокета");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Установка обработчика сигнала SIGHUP
    struct sigaction signalAction;
    signalAction.sa_handler = sigHupHandler;
    signalAction.sa_flags = SA_RESTART;
    sigaction(SIGHUP, &signalAction, NULL);

    sigset_t blockedMask, origMask;
    sigemptyset(&blockedMask);
    sigaddset(&blockedMask, SIGHUP);
    sigprocmask(SIG_BLOCK, &blockedMask, &origMask);

    fd_set master_fds, read_fds;
    FD_ZERO(&master_fds);
    FD_SET(server_socket, &master_fds);
    int max_fd = server_socket;

    while (1) {
        read_fds = master_fds;

        // Вызов функции pselect()
        if (pselect(max_fd + 1, &read_fds, NULL, NULL, NULL, &origMask) == -1) {
            if (errno == EINTR) {
                // Обработка сигнала
                pthread_mutex_lock(&mutex);
                if (wasSigHup) {
                    wasSigHup = 0; // Сброс флага получения сигнала
                }
                pthread_mutex_unlock(&mutex);
            } else {
                perror("Ошибка в pselect()");
                exit(EXIT_FAILURE);
            }
        }

        // Проверка новых соединений
        if (FD_ISSET(server_socket, &read_fds)) {
            int *new_socket_ptr = (int *)malloc(sizeof(int));
            *new_socket_ptr = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
            if (*new_socket_ptr > 0) {
                printf("Новое подключение от %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                pthread_t cleanup_thread;
                pthread_create(&cleanup_thread, NULL, cleanupThread, (void *)new_socket_ptr);
                pthread_detach(cleanup_thread);
            }
        }

        // Обработка данных на активных соединениях
        for (int fd = server_socket + 1; fd <= max_fd; ++fd) {
            if (FD_ISSET(fd, &read_fds)) {
                char buffer[BUFFER_SIZE];
                ssize_t bytes_received = recv(fd, buffer, sizeof(buffer), 0);
                if (bytes_received <= 0) {
                    // Соединение закрыто или произошла ошибка
                    printf("Соединение %d закрыто\n", fd);
                    close(fd);
                    FD_CLR(fd, &master_fds);
                } else {
                    // Вывод сообщения о количестве полученных данных
                    printf("Получено %zd байт данных от соединения %d\n", bytes_received, fd);
                }
            }
        }
    }

    // Очистка ресурсов - закрытие сокета
    close(server_socket);

    return 0;
}
