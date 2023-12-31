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
#include <time.h>

#define MAX_CLIENT_CHAR_NAME 50
#define MAX_ROOM_CHAR_NAME 50
#define MAX_CLIENTS_PER_ROOM 10
#define MAX_ROOMS 10
#define STDIN 0
#define BUFFER_SIZE 256

typedef struct
{
    int id;
    char name[MAX_CLIENT_CHAR_NAME];
    struct sockaddr_in addr;
    int client_sockfd; // Novo campo adicionado
    int current_room;
} Client;
typedef struct
{
    struct sockaddr_in addr;
} Server;
typedef struct
{
    int id;
    char name[MAX_ROOM_CHAR_NAME];
    int clients[MAX_CLIENTS_PER_ROOM];
    int clients_count;
} Room;

Room rooms[MAX_ROOMS];
Client clients[MAX_ROOMS * MAX_CLIENTS_PER_ROOM];
int sockfd, max_fd, newsockfd, bytes_received, opt = 1;
int clients_count = 0;
int rooms_count = 0;
fd_set master_fds;
char buffer[BUFFER_SIZE];

void initialize_rooms()
{
    for (int room = 0; room < MAX_ROOMS; room++)
    {

        if (room < 5)
        {
            int room_id = room + 1;
            rooms[room].id = room_id;
            rooms[room].clients_count = 0;
            snprintf(rooms[room].name, sizeof(rooms[room].name), "Sala %d", room_id);
            rooms_count++;
        }
        else
        {
            rooms[room].id = -1;
        }
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

    clients_count++;
    clients[clients_count - 1].addr = client_addr;
    clients[clients_count - 1].client_sockfd = newsockfd;                                                            // Atribuição do client_sockfd
    snprintf(clients[clients_count - 1].name, sizeof(clients[clients_count - 1].name), "Cliente %d", clients_count); // Atribuição do client_sockfd
    printf("\n");
    printf("Cliente conectado:\n");
    printf("- Endereço IP: %s\n", inet_ntoa(client_addr.sin_addr));
    printf("- Porta: %d\n", ntohs(client_addr.sin_port));

    FD_SET(newsockfd, &master_fds);

    if (newsockfd > max_fd)
    {
        max_fd = newsockfd;
    }

    send_message(newsockfd, "Bem vindo(a), voce esta no saguao.\n-----LISTA DE COMANDOS-----.\n$setname <nome> para escolher um nome.\n$join <nome_da_sala> para entrar numa sala.\n$listrooms para listar salas existentes.\n$create <nome_da_sala> para criar uma sala.\n$listroomclients <id_da_sala> para listar clientes de uma sala.\n$delete <nome_da_sala> deleta a sala e manda os participantes pro saguao\n$lobby Te faz sair da sala e voltar para o saguao.\n");
}
void send_message_to_room(int room, const char *message, int this_client)
{
    printf("\n");
    printf("Enviando mensagem para %s\n", rooms[room].name);
    printf("Mensagem: %s\n", message);
    for (int client = 0; client < rooms[room].clients_count; client++)
    {

        for (int idx = 0; idx < MAX_ROOMS * MAX_CLIENTS_PER_ROOM; idx++)
        {
            if (rooms[room].clients[client] == this_client)
            {
                continue;
            }

            if (clients[idx].client_sockfd == rooms[room].clients[client]) //&&
            {
                send_message(clients[idx].client_sockfd, message);
            }
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

void cmd_leave_room(int client_sockfd)
{
    for (int client_idx = 0; client_idx < MAX_ROOMS * MAX_CLIENTS_PER_ROOM; client_idx++)
    {
        if (clients[client_idx].client_sockfd == client_sockfd)
        {
            for (int room_idx = 0; room_idx < MAX_ROOMS; room_idx++)
            {
                if (rooms[room_idx].id == clients[client_idx].current_room)
                {
                    for (int i = 0; i < rooms[room_idx].clients_count; i++)
                    {
                        if (rooms[room_idx].clients[i] == client_sockfd)
                        {
                            rooms[room_idx].clients[i] = 0;
                        }
                    }

                    rooms[room_idx].clients_count--;

                    break;
                }
            }

            clients[client_idx].current_room = -1;
            break;
        }
    }
}

void handle_client_command(int client_sockfd, char *command)
{
    char *commands[] = {"$setname", "$join", "$listrooms", "$listroomclients", "$lobby", "$create", "$delete"};

    if (strncmp(command, commands[0], strlen(commands[0])) == 0)
    {
        char *prefixPosition = strstr(command, &commands[0][1]);

        char *name = prefixPosition + strlen(commands[0]);

        size_t length = strlen(name);

        if (length >= 2)
        {
            size_t penultimateIndex = length - 2;
            name[penultimateIndex] = '\0';
        }

        for (int idx = 0; idx < MAX_ROOMS * MAX_CLIENTS_PER_ROOM; idx++)
        {
            if (clients[idx].client_sockfd == client_sockfd)
            {
                strcpy(clients[idx].name, name);
                break;
            }
        }
    }
    else if (strncmp(command, commands[1], strlen(commands[1])) == 0)
    {
        char *prefixPosition = strstr(command, &commands[1][1]);

        char *name = prefixPosition + strlen(commands[1]);

        size_t length = strlen(name);

        if (length >= 2)
        {
            size_t penultimateIndex = length - 2;
            name[penultimateIndex] = '\0';
        }
        short foundRoom = 0;

        for (int room = 0; room < MAX_ROOMS; room++)
        {
            if (strcmp(rooms[room].name, name) == 0)
            {
                foundRoom = 1;
                for (int idx = 0; idx < MAX_ROOMS * MAX_CLIENTS_PER_ROOM; idx++)
                {

                    if (clients[idx].client_sockfd == client_sockfd)
                    {

                        if (clients[idx].current_room != -1)
                        {
                            cmd_leave_room(client_sockfd);
                        }

                        clients[idx].current_room = rooms[room].id;

                        for (int client_idx = 0; client_idx < MAX_CLIENTS_PER_ROOM; client_idx++)
                        {

                            if (rooms[room].clients[client_idx] == 0)
                            {
                                rooms[room].clients[client_idx] = client_sockfd;
                                rooms[room].clients_count++;

                                break;
                            }
                        }

                        break;
                    }
                }
                char welcome_message[BUFFER_SIZE];
                snprintf(welcome_message, BUFFER_SIZE, "Você entrou na sala %s.\n", name);
                send_message(client_sockfd, welcome_message);

                break;
            }
        }

        if (foundRoom == 0)
        {
            send_message(client_sockfd, "Não há encontramos nenhuma sala com este nome.\n");
        }
    }
    else if (strncmp(command, commands[2], strlen(commands[2])) == 0)
    {
        char rooms_list_title[BUFFER_SIZE];
        char room_info[BUFFER_SIZE];

        snprintf(rooms_list_title, BUFFER_SIZE, "\nSALAS:\n\n");
        send_message(client_sockfd, rooms_list_title);

        for (int room = 0; room < MAX_ROOMS; room++)
        {
            if (rooms[room].id != -1)
            {
                snprintf(room_info, BUFFER_SIZE, "ID: %d, Nome: %s, Clientes: %d/%d\n", rooms[room].id, rooms[room].name, rooms[room].clients_count, MAX_CLIENTS_PER_ROOM);
                send_message(client_sockfd, room_info);
            }
        }
    }
    else if (strncmp(command, commands[3], strlen(commands[3])) == 0)
    {
        int room_id, clients_count = 0;
        char clients_list_title[BUFFER_SIZE], client_info[BUFFER_SIZE], total_clients[BUFFER_SIZE];

        sscanf(command, "$listroomclients %d", &room_id);
        snprintf(clients_list_title, BUFFER_SIZE, "\nCLIENTES DA SALA %d:\n\n", room_id);
        send_message(client_sockfd, clients_list_title);

        for (int i = 0; i < MAX_ROOMS * MAX_CLIENTS_PER_ROOM; i++)
        {
            Client client = clients[i];
            if (client.current_room == room_id)
            {
                clients_count += 1;
                snprintf(client_info, BUFFER_SIZE, "ID: %d, Nome: %s\n", client.id, client.name);
                send_message(client_sockfd, client_info);
            }
        }
        snprintf(total_clients, BUFFER_SIZE, "Total: %d\n\n", clients_count);
        send_message(client_sockfd, total_clients);
    }

    else if (strncmp(command, commands[4], strlen(commands[4])) == 0)
    {
        cmd_leave_room(client_sockfd);

        send_message(client_sockfd, "Voce saiu da sala e foi para o saguao\n");

        send_message(client_sockfd, "Bem vindo(a), voce esta no saguao.\n-----LISTA DE COMANDOS-----.\n $setname <nome> para escolher um nome.\n$join <nome_da_sala> para entrar numa sala.\n$listrooms para listar salas existentes.\n$create <nome_da_sala> para criar uma sala.\n$listroomclients <id_da_sala> para listar clientes de uma sala.\n$create <nome_da_sala> para criar uma sala.\n$lobby para sair da sala e voltar ao saguao.\n");
    }

    else if (strncmp(command, commands[5], strlen(commands[5])) == 0)
    {

        char *prefixPosition = strstr(command, &commands[5][1]);

        char *name = prefixPosition + strlen(commands[5]);

        size_t length = strlen(name);

        if (length >= 2)
        {
            size_t penultimateIndex = length - 2;
            name[penultimateIndex] = '\0';
        }

        int empty_index = -1;
        if (rooms_count < MAX_ROOMS)
        {
            for (int i = 0; i < MAX_ROOMS; i++)
            {

                if ((strncmp(rooms[i].name, name, strlen(name)) == 0))
                {
                    send_message(client_sockfd, "Já existe uma sala com esse nome!\n");
                    return;
                }

                if (rooms[i].id == -1)
                {
                    empty_index = i;
                }
            }
        }

        printf("index %d", empty_index);
        fflush(stdout);
        int randomNum;

        srand(time(NULL));

        randomNum = rand();

        rooms[empty_index].id = randomNum;
        rooms[empty_index].clients_count = 0;
        strcpy(rooms[empty_index].name, name);
        memset(rooms[empty_index].clients, 0, sizeof(rooms[empty_index].clients));
    }
    else if (strncmp(command, commands[6], strlen(commands[6])) == 0)
    {
        char *prefixPosition = strstr(command, &commands[6][1]);

        char *name = prefixPosition + strlen(commands[6]);

        size_t length = strlen(name);

        if (length >= 2)
        {
            size_t penultimateIndex = length - 2;
            name[penultimateIndex] = '\0';
        }

        for (int i = 0; i < MAX_ROOMS; i++)
        {

            if ((strncmp(rooms[i].name, name, strlen(name)) == 0))
            {
                /* Removendo todos os clientes da sala que será
                deletada .*/
                for (int j = 0; j++; j < rooms[i].clients_count)
                {
                    cmd_leave_room(rooms[i].clients[j]);
                    // send_message(rooms[i].clients[j], "Voce foi movido para o saguao pois sua sala foi deletada.\n\0");
                }

                /*Excluindo a sala em si*/
                rooms[i].id = -1;
                rooms[i].clients_count = 0;
                memset(rooms[i].clients, 0, sizeof(rooms[i].clients));
            }
        }
    }
    else
    {
        printf("Unknown command.\n");
    }
}

void remove_client(int room, int client)
{
    struct sockaddr_in client_address = clients[rooms[room].clients[client]].addr;
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
                if (clients[rooms[room].clients[client]].client_sockfd == client_sockfd) // Comparação usando client_sockfd
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
        printf("\n");
        printf("Cliente (socket %d): %s\n", client_sockfd, buffer);

        if (buffer[0] == '$')
        {
            handle_client_command(client_sockfd, buffer);
            return;
        }

        for (int idx = 0; idx < MAX_ROOMS * MAX_CLIENTS_PER_ROOM; idx++)
        {
            if (clients[idx].client_sockfd == client_sockfd)
            {
                for (int room = 0; room < MAX_ROOMS; room++)
                {
                    if (rooms[room].id == clients[idx].current_room)
                    {

                        char message[BUFFER_SIZE];
                        snprintf(message, BUFFER_SIZE, "[%s]: %s", clients[idx].name, buffer);
                        send_message_to_room(room, message, client_sockfd);
                        break;
                    }
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
