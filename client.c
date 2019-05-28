#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h> 
#include <sys/types.h> 
#include <netinet/ip.h>
#include <pthread.h>

#define PORT 8080

void *readFromServer(void *arg){
    char read_buffer[1024] = {0};
    int client_socket = *((int *)arg); 
    free(arg);
    int valread;
    while (1)
    {
        if(valread= read(client_socket, read_buffer, 1024) > 0){
            printf("%s\n", read_buffer);
            memset(read_buffer,0,sizeof(read_buffer));
        }  
    }
}




int main(int argc, char const *argv[]){
    struct sockaddr_in addr_server;

    int client_socket=0;
    char send_buffer[1024] = {0};
    char read_buffer[1024] = {0};
    pthread_t r_thread; 

    //Creating a file descriptor for the socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(client_socket < 0){
        perror("Error in socket()\n");
        exit(EXIT_FAILURE);
    }else
    {
        printf("[+] Socket created \n");
    }

    //Socket options
    memset(&addr_server, '0', sizeof(addr_server));

    addr_server.sin_family = AF_INET;
    addr_server.sin_port = htons(PORT);

    //Convert IPv4 and IPv6 addresses from text to binary form 
    if (inet_pton(AF_INET, "127.0.0.1", &addr_server.sin_addr) <= 0)
    {
        perror("[-] Invalid address, Address not supported \n");
        exit(EXIT_FAILURE);
    }

    //connecting to the server

    if(connect(client_socket,(struct sockaddr *) &addr_server, sizeof(addr_server)) < 0){
        perror("Error in connect()\n");
        exit(EXIT_FAILURE);
    }else
    {
        printf("[+] Connection with the server successful \n");
        int *arg = malloc(sizeof(*arg));
        *arg = client_socket;
        pthread_create(&r_thread, 0, readFromServer, (void*)arg); 
    }
        
    while (1)
    {
        

        printf(": ");
        gets(send_buffer);
        send(client_socket, &send_buffer,strlen(send_buffer),0);
        fflush(stdin);
        memset(send_buffer,0,sizeof(send_buffer));


    }

    

    //Closing the sockets
    close(client_socket);
    return 0;
}