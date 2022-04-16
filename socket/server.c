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

bool alarm_sig_flag = false;
bool alarm_instant_flag = false;
bool instant_flag = false, duration_flag = false;

//There might be a case where you need to handle duration and instant fucntion's alarms separately
int hour = 0, min = 0, sec = 0;
time_t rawtime;
struct tm *info;

void graceful_shutdown(int signo)
{
    syslog(LOG_NOTICE,"CAUGHT SIGNAL, EXITING GRACEFULLY \n");
    close(server_socket);
}

void alarm_handler(int signo)
{
    if(duration_flag)
    {
        
        alarm_sig_flag = true;
        duration_flag = false;
    }
    if(instant_flag)
    {
        syslog(LOG_DEBUG,"Inside alarm handler\n");
        time(&rawtime);
        info = localtime(&rawtime);
        if(hour == info->tm_hour && min == info->tm_min && sec == info->tm_sec)
            alarm_instant_flag = true;
        else
            alarm(1);        
    }
}

void toggle()
{
    char set_state;
    char read_state;
    int read_bytes, write_bytes;
    int gpio_fd = open(GPIO_DRIVER, O_RDWR, 0777);
    //Error handling required
    read_bytes = read(gpio_fd, &read_state, 1);
    //Error handling required
    printf("State %c \n", read_state);
    if(read_state == '1')
    {
        set_state = '0';
        write_bytes = write(gpio_fd, &set_state, 1);
        //Error handling required
    }
    else if(read_state == '0')
    {
        set_state = '1';
        write_bytes = write(gpio_fd, &set_state, 1); 
        //Error handling required 
    }
    close(gpio_fd);
}

void set_reset_load(char * msg)
{
    printf("set fucntion\n");
    char set_state = msg[4];
    int write_bytes;
    int gpio_fd = open(GPIO_DRIVER, O_RDWR, 0777);
    //Error handling required
    write_bytes = write(gpio_fd, &set_state, 1);
    //Error handling required
    close(gpio_fd);
}

void duration(char * msg)
{
    duration_flag = true;
    int i = 6, j = 0;
    int temp = 0;
    while(msg[i] != ';')
    {
        temp = (temp*10) + (msg[i] - '0');
        i++;
    }
    printf("duration in seconds: %d\n", temp);
    alarm(temp);
    while(1)
    {
        sleep(1);
        if(alarm_sig_flag)
        {
            set_reset_load(msg);
            break;
        }
    }
}

void instant(char *msg)
{
    instant_flag = true;
    int i =6;
    if(msg[i+1] == ':')
    {
        hour = (msg[i]-'0');
        i = i+2;
    }
    else
    {
        hour = (msg[i]-'0')*10 + (msg[i+1] - '0');
        i = i+3;
    }
    
    if(msg[i+1] == ':')
    {
        min = (msg[i] - '0');
        i = i+2;
    }
    else
    {
        min = (msg[i] - '0')*10 + (msg[i+1] - '0');
        i = i+3;
    }
    
    if(msg[i+1] == ';')
    {
        sec = (msg[i] - '0');
    }
    else
    {
        sec = (msg[i] - '0')*10 + (msg[i+1] - '0');
    }
    printf("%d %d %d\n", hour, min,sec);
    alarm(1);
    while(1)
    {
        sleep(1);
        if(alarm_instant_flag)
        {
            set_reset_load(msg);
            break;
        }
    }
    
}

void mode(char * msg)
{
    printf("char %c\n",msg[0]);
    switch (msg[2])
    {
        case '0':toggle();
                break;
        case '1':set_reset_load(msg);
                break;
        case '2':duration(msg);
                break;
        case '3':instant(msg);
                break;
    }
}
int main()
{

    char server_message[256];
    //signal callback functions
    signal(SIGINT, graceful_shutdown);
    signal(SIGTERM, graceful_shutdown);
    signal(SIGALRM, alarm_handler);

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
