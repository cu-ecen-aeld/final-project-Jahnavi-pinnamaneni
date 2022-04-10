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
    

    
}