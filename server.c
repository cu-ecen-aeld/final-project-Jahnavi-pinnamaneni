#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <syslog.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>

int server_socket;
int client_socket;

void graceful_shutdown(int signo)
{
    syslog(LOG_NOTICE,"CAUGHT SIGNAL, EXITING GRACEFULLY \n");

}

int main()
{
    //signal callback functions
    signal(SIGINT, graceful_shutdown);
    signal(SIGTERM, graceful_shutdown);

    //Initialization of syslog
    openlog("AESD", LOG_PERROR, LOG_USER);

    //create server socket
    int get_fd;
    struct addrinfo hints;
    struct addrinfo *rp;
    struct sockaddr_storage cl_addr;
    socklen_t addr_size;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = 0;

    get_fd = getaddrinfo(NULL, "9000", &hints, &rp);
    if(get_fd != 0)
    {
        perror("getaddrinfo");
        exit(1);
    }

    server_socket = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if(server_socket == -1)
    {
        perror("socket");
        exit(1);
    }
    if(setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int))<0)
        perror("setsockopt");

    //bind the socket to the specified IP and port
    if(bind(server_socket, rp->ai_addr, rp->ai_addrlen)!= 0)
    {
        perror("bind");
        exit(1);
    }
    syslog(LOG_DEBUG, "Binded Succefully");
    freeaddrinfo(rp);


    //listen for connection
    listen(server_socket, 5);

    addr_size = sizeof(cl_addr);
    client_socket = accept(server_socket, (struct sockaddr*)&cl_addr, &addr_size);
    
    

    
}