#include <stdio.h>
#include <netinet/in.h>

#define MAX_CLIENT_CHAR_NAME 50
#define MAX_ROOM_CHAR_NAME 50
#define MAX_CLIENTS_PER_ROOM 10
#define MAX_ROOMS 5

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

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printf("Digite o IP e a porta para este servidor.\n");
        exit(1);
    }

    initialize_rooms();
}
