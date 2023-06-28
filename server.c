#include <stdio.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/types.h>

#define MAX_CLIENT_CHAR_NAME 50
#define MAX_ROOM_CHAR_NAME 50
#define MAX_CLIENTS_PER_ROOM 10
#define MAX_ROOMS 5
#define STDIN 0

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
fd_set master_fds;

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

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printf("Digite o IP e a porta para este servidor.\n");
        exit(1);
    }

    initialize_rooms();

    sockfd = create_socket(argv[1], atoi(argv[2]));

    FD_ZERO(&master_fds);
}
