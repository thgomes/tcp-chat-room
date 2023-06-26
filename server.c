#include <stdio.h>
#include <netinet/in.h>

#define MAX_CLIENT_CHAR_NAME 50
#define MAX_ROOM_CHAR_NAME 50
#define MAX_CLIENTS_ON 25

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
    Client clients_on[MAX_CLIENTS_ON];
} Room;

int main()
{
    printf("Hello world!\n");
}
