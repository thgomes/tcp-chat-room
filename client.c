#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/select.h> // Inclua essa linha para utilizar fd_set

#define MAX_MESSAGE_SIZE 1024

// Cores para a interface de linha de comando
#define COLOR_DEFAULT 0
#define COLOR_RED 1
#define COLOR_GREEN 2
#define COLOR_YELLOW 3
#define COLOR_BLUE 4
#define COLOR_MAGENTA 5
#define COLOR_CYAN 6

void set_color(int color)
{
    printf("\033[0;3%dm", color);
}

void reset_color()
{
    printf("\033[0m");
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Uso: %s <IP> <PORTA>\n", argv[0]);
        return 1;
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);

    // Criar um socket TCP
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        perror("Erro ao criar o socket");
        return 1;
    }

    // Definir o endereço IP e a porta do servidor
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);

    // Conectar ao servidor
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("Erro ao conectar ao servidor");
        close(sockfd);
        return 1;
    }

    // Loop principal
    char message[MAX_MESSAGE_SIZE];
    while (1)
    {
        // Verificar se há entrada do usuário ou do socket
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(fileno(stdin), &fds);
        FD_SET(sockfd, &fds);

        int maxfd = (fileno(stdin) > sockfd) ? fileno(stdin) : sockfd;

        if (select(maxfd + 1, &fds, NULL, NULL, NULL) == -1)
        {
            perror("Erro ao esperar por eventos");
            break;
        }

        // Verificar se há mensagem para enviar
        if (FD_ISSET(fileno(stdin), &fds))
        {
            if (fgets(message, sizeof(message), stdin) == NULL)
            {
                perror("Erro ao ler entrada");
                break;
            }
            size_t len = strlen(message);
            if (len > 0 && message[len - 1] == '\n')
            {
                message[len - 1] = '\0';
            }
            len = strlen(message);
            strcat(message, "\r\n");
            len += 2;
            if (send(sockfd, message, strlen(message), 0) == -1)
            {
                perror("Erro ao enviar mensagem para o servidor");
                break;
            }
            set_color(COLOR_CYAN);
            printf("Você: %s\n", message);
            reset_color();
        }

        // Verificar se há mensagem recebida do servidor
        if (FD_ISSET(sockfd, &fds))
        {
            char recv_buffer[MAX_MESSAGE_SIZE];
            ssize_t recv_bytes = recv(sockfd, recv_buffer, sizeof(recv_buffer) - 1, 0);
            if (recv_bytes > 0)
            {
                recv_buffer[recv_bytes] = '\0';
                set_color(COLOR_GREEN);
                printf("%s", recv_buffer);
                reset_color();
            }
            else if (recv_bytes == 0)
            {
                // O servidor encerrou a conexão
                break;
            }
            else
            {
                perror("Erro ao receber mensagem do servidor");
                break;
            }
            fflush(stdout);
        }
    }

    // Fechar o socket
    close(sockfd);

    return 0;
}
