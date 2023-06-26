#include <stdio.h>
#include <netinet/in.h>

#define MAX_CLIENT_CHAR_NAME 50

typedef struct
{
    int id;
    char name[MAX_CLIENT_CHAR_NAME];
    struct sockaddr_in addr
} Client;

typedef struct
{
    struct sockaddr_in addr;
} Server;

int main()
{
    printf("Hello world!\n");
}
