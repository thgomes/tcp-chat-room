#include <stdio.h>
#include <netinet/in.h>

typedef struct
{

    struct sockaddr_in addr;
} Server;

int main()
{
    printf("Hello world!\n");
}