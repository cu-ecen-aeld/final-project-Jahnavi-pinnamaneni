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
#define GPIO_DRIVER "/dev/my_gpio_driver"

int server_socket;
int client_socket;

void graceful_shutdown(int signo)
{
    syslog(LOG_NOTICE,"CAUGHT SIGNAL, EXITING GRACEFULLY \n");

}

void toggle()
{
    char set = '1';
    int gpio_fd = open(GPIO_DRIVER, O_RDWR, 0777);
    write(gpio_fd,&set,1);

}

void mode(char * msg)
{
    printf("char %c\n",msg[0]);
    switch (msg[2])
    {
        case '0':toggle(msg);
                break;
        // case '1':set_reset_load();
        //         break;
        // case '2':duration();
        //         break;
        // case '3':instant();
        //         break;
    }
}
int main()
{

    char server_message[256];
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
    
    syslog(LOG_DEBUG, "Connection to client succefull\n");

    int count = recv(client_socket, &server_message, sizeof(server_message),0);
    printf("Count %d\n", count);
    syslog(LOG_DEBUG,"String %s\n", server_message);
    mode(server_message);
    close(server_socket);
    return 0;
}
