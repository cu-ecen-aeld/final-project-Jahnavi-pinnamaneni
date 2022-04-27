/*
 * @file: server.c
 * @brief: This program aims at establishing a network socket to communicate with a PC over LAN.
 *      This program runs on the Raspberry Pi and the Raspberry Pi acts as the server.
 *      The server listens for connections and accepts them. Once the connection is accepted from the client,
 *      the server and client start communicating.
 *      The client can control any number of loads and set them to one of the 4 modes available
 *  1) Toggle the load state
 *  2) Set the load to specific state
 *  3) Set the load to a specific state after a certain number of milliseconds.
 *  4) Set the load to a specific state at a certain time of the day.
 * @author: Jahnavi Pinnamaneni; japi8358@colorado.edu
 */

/*
 * Standard Header files
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/*
 * Header files pertaining to network socket
 */
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

/*
 * Header files pertaining to File I/O
 */
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

/*
 * Header files pertaining to signals and logging
 */
#include <syslog.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>


#define GPIO_DRIVER "/dev/my_gpio_driver"

/*
 * Global variable declarations
 */
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
    remove(GPIO_DRIVER);
    close(server_socket);
    closelog();
    exit(0);
}

/*
 * @desc: Signal Handler for alarm, this function is for both Mode 2 and Mode 3.
 *        For the mode 2: The alarm is set for the desired number of seconds.
 *        For the mode 3: An alarm is set every one second and the time is 
 *        compared with the user specified time. 
 * 
 * Takes the signal number as input
 * Returns void
 */
void alarm_handler(int signo)
{
    if(duration_flag)
    {        
        alarm_sig_flag = true;
        duration_flag = false;
    }
    if(instant_flag)
    {
        time(&rawtime);
        info = localtime(&rawtime);
        if(hour == info->tm_hour && min == info->tm_min && sec == info->tm_sec)
            alarm_instant_flag = true;
        else
            alarm(1);        
    }
}

/*
 * @desc: This function is the handler for Mode 0. It reads the present state of the 
 *        mentioned GPIO and toggles its state.
 * 
 * Takes a string as input
 * Returns void
 */

void toggle(char * msg)
{
    char set_state;
    char temp[2];
    char read_state;
    int write_bytes, read_bytes;
    
    int gpio_fd = open(GPIO_DRIVER, O_RDWR, 0777);
    if(gpio_fd < 0)
        syslog(LOG_ERR, "toggle: Cannot open file\n");
        
    read_bytes = read(gpio_fd, temp, 2);
    if(read_bytes == -1)
        syslog(LOG_ERR, "toggle: Cannot read GPIO state\n");
    if(msg[0] == '1')
        read_state = temp[0];
    else if(msg[0] == '2')
        read_state = temp[1];
        
    if(read_state == '1' && msg[0] == '1')
        set_state = '0';
    else if(read_state == '0' && msg[0] == '1')
        set_state = '1';
    else if(read_state == '1' && msg[0] == '2')
        set_state = '2';
    else if(read_state == '0' && msg[0] == '2')
        set_state = '3';
        
    write_bytes = write(gpio_fd, &set_state, 1);
    if(write_bytes == -1)
        syslog(LOG_ERR, "toggle: Cannot write GPIO state\n");
    close(gpio_fd);
}

/*
 * @desc: This function is the handler for Mode 1. Based on the load mentioned 
 *        by the user, the state of the GPIO is set or reset.
 * 
 * Takes a string as input
 * Returns void
 */
void set_reset_load(char * msg)
{
    char set_state = msg[4];
    int write_bytes;
    
    int gpio_fd = open(GPIO_DRIVER, O_RDWR, 0777);
    if(gpio_fd < 0)
        syslog(LOG_ERR, "set_reset_load: Cannot open file\n");
    if(msg[0] == '2')
        set_state += 2;
        
    write_bytes = write(gpio_fd, &set_state, 1);
    if(write_bytes == -1)
        syslog(LOG_ERR, "toggle: Cannot read GPIO state\n");
    close(gpio_fd);
}

/*
 * @desc: This function is the handler for Mode 2. It sets an alarm for the 
 *        time mentioned by the user and ones the alarm goes off the state of the 
 *        GPIO is set or reset based on the input recieved.
 * 
 * Takes a string as input
 * Returns void
 */

void duration(char * msg)
{
    duration_flag = true;
    int i = 6;
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

/*
 * @desc: This function is the handler for Mode 2. It parses the recieved string to 
 *        find the hours, mins and secs. Then recursively sets an alarm for one second 
 *        and wait for the desired amount of time. Once the time has elapsed, the GPIO 
 *        set or reset based on user input.
 * 
 * Takes a string as input
 * Returns void
 */

void instant(char *msg)
{
    instant_flag = true;
    int i =6;
    if(msg[i+1] == ':')
    {
        hour = (msg[i]-'0') - 1;
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


/*
 * @desc: This function determines the mode mentioned by the user.
 * 
 * Takes a string as input
 * Returns void
 */
void mode(char * msg)
{
    switch (msg[2])
    {
        case '0':toggle(msg);
                break;
        case '1':set_reset_load(msg);
                break;
        case '2':duration(msg);
                break;
        case '3':instant(msg);
                break;
    }
}

/*
 * @desc: Application Entry Point
 */
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

    while(1){
    int count = recv(client_socket, &server_message, sizeof(server_message),0);
    printf("Count %d\n", count);
    syslog(LOG_DEBUG,"String %s\n", server_message);
    mode(server_message);}
    closelog();
    remove(GPIO_DRIVER);
    close(server_socket);
    return 0;
}
