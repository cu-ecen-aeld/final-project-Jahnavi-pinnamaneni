#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>

#include <sys/types.h>

int main()
{
    int network_socket;
    char *server_msg = "You have connected to the client\n";
    network_socket = socket(AF_INET, SOCK_STREAM,0);

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(9000);
    server_address.sin_addr.s_addr = INADDR_ANY;

    int connect_status = connect(network_socket, (struct sockaddr *) &server_address, sizeof(server_address));
    if(connect_status == -1)
    {
        printf("There was an error making a connection\n");
    }


    int count = send(network_socket, server_msg, strlen(server_msg),0);
    printf("count %d\n", count);

    close(network_socket);
    return 0;
}