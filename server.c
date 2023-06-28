#include <stdio.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>

#define MAX_CLIENT_CHAR_NAME 50
#define MAX_ROOM_CHAR_NAME 50
#define MAX_CLIENTS_PER_ROOM 10
#define MAX_ROOMS 5
#define STDIN 0
#define BUFFER_SIZE 256

typedef struct
{
    int id;
    char name[MAX_CLIENT_CHAR_NAME];
    struct sockaddr_in addr;
} Client;
typedef struct
{
    struct sockaddr_in addr;
} Server;
typedef struct
{
    int id;
    char name[MAX_ROOM_CHAR_NAME];
    Client clients[MAX_CLIENTS_PER_ROOM];
    int clients_count;
} Room;

Room rooms[MAX_ROOMS];
int sockfd;
int sockfd, newsockfd, bytes_received, opt = 1;
fd_set master_fds;
char buffer[BUFFER_SIZE];

void initialize_rooms()
{
    for (int room = 0; room < MAX_ROOMS; room++)
    {
        int room_id = room + 1;
        rooms[room].id = room_id;
        rooms[room].clients_count = 0;
        snprintf(rooms[room].name, sizeof(rooms[room].name), "Sala %d", room_id);
    }
}
int create_socket(const char *ip, int port)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("Erro ao abrir o socket");
        exit(1);
    }

    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int)) < 0)
    {
        perror("Erro ao definir as opções do socket");
        exit(1);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);
    server_addr.sin_port = htons(port);
    memset(&(server_addr.sin_zero), '\0', 8);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Erro ao fazer o bind");
        exit(1);
    }

    if (listen(sockfd, 10) < 0)
    {
        perror("Erro ao ouvir a porta");
        exit(1);
    }

    return sockfd;
}
void send_message()
{
    for (int j = 0; j <= sockfd; j++)
    {
        if (FD_ISSET(j, &master_fds) && j != sockfd && j != STDIN)
        {
            send(j, buffer, bytes_received, 0);
        }
    }
}
int find_available_room()
{
    for (int room = 0; room < MAX_ROOMS; room++)
    {
        if (rooms[room].clients_count < MAX_CLIENTS_PER_ROOM)
        {
            return room;
        }
    }
    return -1;
}
void handle_new_connection()
{
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int newsockfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);

    if (newsockfd < 0)
    {
        perror("Erro ao aceitar a conexão");
        exit(1);
    }

    int room = find_available_room();
    if (room < 0)
    {
        send_message(newsockfd, "Desculpe, todas as salas estão cheias.");
        close(newsockfd);
    }
    else
    {
        rooms[room].clients[rooms[room].clients_count].addr = client_addr;
        FD_SET(newsockfd, &master_fds);
        char welcome_message[BUFFER_SIZE];
        snprintf(welcome_message, BUFFER_SIZE, "Você entrou na sala %s.\n", rooms[room].name);
        send_message(newsockfd, welcome_message);
        rooms[room].clients_count++;
    }
}
void send_message_to_room(int room, const char *message)
{
    for (int client = 0; client < rooms[room].clients_count; client++)
    {
        int client_sockfd = rooms[room].clients[client].addr.sin_family;
        send_message(client_sockfd, message);
    }
}
void handle_stdin_input()
{
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));
    if (read(STDIN, buffer, sizeof(buffer)) <= 0)
    {
        perror("Erro na leitura da entrada padrão");
        exit(1);
    }
    for (int room = 0; room < MAX_ROOMS; room++)
    {
        send_message_to_room(room, buffer);
    }
}
void handle_client_message(int client_sockfd)
{
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));
    int bytes_received = recv(client_sockfd, buffer, sizeof(buffer), 0);

    if (bytes_received <= 0)
    {
        for (int room = 0; room < MAX_ROOMS; room++)
        {
            for (int client = 0; client < rooms[room].clients_count; client++)
            {
                if (rooms[room].clients[client].addr.sin_family == client_sockfd)
                {
                    memmove(
                        &rooms[room].clients[client],
                        &rooms[room].clients[client + 1],
                        (rooms[room].clients_count - client - 1) * sizeof(Client));
                    rooms[room].clients_count--;
                    break;
                }
            }
        }
        FD_CLR(client_sockfd, &master_fds);
        close(client_sockfd);
    }
    else
    {
        for (int room = 0; room < MAX_ROOMS; room++)
        {
            for (int client = 0; client < rooms[room].clients_count; client++)
            {
                if (rooms[room].clients[client].addr.sin_family == client_sockfd)
                {
                    send_message_to_room(room, buffer);
                    break;
                }
            }
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printf("Digite o IP e a porta para este servidor.\n");
        exit(1);
    }

    initialize_rooms();

    FD_ZERO(&master_fds);

    sockfd = create_socket(argv[1], atoi(argv[2]));

    FD_SET(sockfd, &master_fds);
    FD_SET(STDIN, &master_fds);
    int fdmax = sockfd;

    while (1)
    {
        fd_set read_fds = master_fds;

        if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) < 0)
        {
            perror("Erro no select");
            exit(1);
        }

        for (int i = 0; i <= fdmax; i++)
        {
            if (FD_ISSET(i, &read_fds))
            {
                if (i == sockfd)
                {
                    handle_new_connection();
                }
                else if (i == STDIN)
                {
                    handle_stdin_input();
                }
                else
                {
                    handle_client_message(i);
                }
            }
        }
    }

    return 0;
}
