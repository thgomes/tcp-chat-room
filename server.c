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
    int client_sockfd; // Novo campo adicionado
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
int sockfd, max_fd, newsockfd, bytes_received, opt = 1;
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
        perror("Erro ao abrir o socket\n");
        exit(1);
    }

    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int)) < 0)
    {
        perror("Erro ao definir as opções do socket\n");
        exit(1);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);
    server_addr.sin_port = htons(port);
    memset(&(server_addr.sin_zero), '\0', 8);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Erro ao fazer o bind\n");
        exit(1);
    }

    if (listen(sockfd, 10) < 0)
    {
        perror("Erro ao ouvir a porta\n");
        exit(1);
    }

    printf("\n");
    printf("Socket %d criado com sucesso\n", sockfd);

    return sockfd;
}
void send_message(int sockfd, const char *message)
{
    send(sockfd, message, strlen(message), 0);
    printf("\n");
    printf("Enviando mensagem para o socket %d\n", sockfd);
    printf("Mensagem: %s\n", message);
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
        perror("Erro ao aceitar a conexão\n");
        exit(1);
    }

    int room = find_available_room();
    if (room < 0)
    {
        send_message(newsockfd, "Desculpe, todas as salas estão cheias.\n");
        close(newsockfd);
    }
    else
    {
        rooms[room].clients[rooms[room].clients_count].addr = client_addr;
        rooms[room].clients[rooms[room].clients_count].client_sockfd = newsockfd; // Atribuição do client_sockfd
        FD_SET(newsockfd, &master_fds);
        char welcome_message[BUFFER_SIZE];
        snprintf(welcome_message, BUFFER_SIZE, "Você entrou na sala %s.\n", rooms[room].name);
        send_message(newsockfd, welcome_message);
        rooms[room].clients_count++;

        if (newsockfd > max_fd)
        {
            max_fd = newsockfd;
        }

        // Imprimir informações do cliente conectado
        printf("\n");
        printf("Cliente conectado:\n");
        printf("- Endereço IP: %s\n", inet_ntoa(client_addr.sin_addr));
        printf("- Porta: %d\n", ntohs(client_addr.sin_port));
    }
}
void send_message_to_room(int room, const char *message, int this_client)
{
    printf("\n");
    printf("Enviando mensagem para %s\n", rooms[room].name);
    printf("Mensagem: %s\n", message);
    for (int client = 0; client < rooms[room].clients_count; client++)
    {
        int client_sockfd = rooms[room].clients[client].client_sockfd; // Acesso ao client_sockfd

        if (client_sockfd != this_client)
        {
            send_message(client_sockfd, message);
        }
    }
}
void handle_stdin_input()
{
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));
    if (read(STDIN, buffer, sizeof(buffer)) <= 0)
    {
        perror("Erro na leitura da entrada padrão\n");
        exit(1);
    }
    for (int room = 0; room < MAX_ROOMS; room++)
    {
        send_message_to_room(room, buffer, -1);
    }
}
void remove_client(int room, int client)
{
    struct sockaddr_in client_address = rooms[room].clients[client].addr;
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_address.sin_addr), client_ip, INET_ADDRSTRLEN);
    int client_port = ntohs(client_address.sin_port);

    // Remove o cliente da lista de clientes da sala
    memmove(
        &rooms[room].clients[client],
        &rooms[room].clients[client + 1],
        (rooms[room].clients_count - client - 1) * sizeof(Client));
    rooms[room].clients_count--;

    // Imprime o log informando as informações do endereço do cliente removido
    printf("\n");
    printf("Cliente removido: IP: %s, Porta: %d\n", client_ip, client_port);
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
                if (rooms[room].clients[client].client_sockfd == client_sockfd) // Comparação usando client_sockfd
                {
                    remove_client(room, client);
                    break;
                }
            }
        }
        FD_CLR(client_sockfd, &master_fds);
        close(client_sockfd);
    }
    else
    {
        buffer[bytes_received] = '\0'; // Adicione isso para garantir que a string seja terminada corretamente
        printf("/n");
        printf("Cliente (socket %d): %s\n", client_sockfd, buffer);

        for (int room = 0; room < MAX_ROOMS; room++)
        {
            for (int client = 0; client < rooms[room].clients_count; client++)
            {
                if (rooms[room].clients[client].client_sockfd == client_sockfd)
                {
                    char message[BUFFER_SIZE];
                    snprintf(message, BUFFER_SIZE, "[%s]: %s", rooms[room].clients[client].name, buffer);
                    send_message_to_room(room, message, client_sockfd);
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

    sockfd = create_socket(argv[1], atoi(argv[2]));

    initialize_rooms();

    FD_ZERO(&master_fds);
    FD_SET(sockfd, &master_fds);
    FD_SET(STDIN, &master_fds);

    max_fd = sockfd;

    while (1)
    {
        fd_set read_fds = master_fds;
        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0)
        {
            perror("Erro no select\n");
            exit(1);
        }

        for (int fd = 0; fd <= max_fd; fd++)
        {
            if (FD_ISSET(fd, &read_fds))
            {
                if (fd == sockfd)
                {
                    handle_new_connection();
                }
                else if (fd == STDIN)
                {
                    handle_stdin_input();
                }
                else
                {
                    handle_client_message(fd);
                }
            }
        }
    }

    return 0;
}
