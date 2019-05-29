//Compile : gcc client.c -o client -lpthread -lcrypt
//Run : ./client

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
#include <crypt.h> 

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
    
    int log_reg = -1;
    int client_socket=0;
    char send_buffer[1024] = {0};
    char read_buffer[1024] = {0};
    pthread_t r_thread; 
    char username[10], password[50];
    char auth[100];

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
        //REGISTRATION 
        printf("Type:\n (1) for Registration to the chat\n (2) for Log in\n");
        while (1)
        {
            scanf("%d",&log_reg);
            fflush(stdout);
            fflush(stdin);
            if(log_reg == 1 || log_reg ==2){
                break;
            }
            else{
                printf("You need to enter 1 or 2 , try agian please \n");
                continue;
            }
        }
        while(1){
            if(log_reg == 1){

                //Register
                printf("\n--Registration--\n");
                printf("Username:");
                scanf(" %10[0-9a-zA-Z ]",username);
                fflush(stdout);
                printf("\n Password: ");
                scanf("%s",password);
                fflush(stdout);

                snprintf(auth,sizeof(auth), "%s/%s/%s","reg",username,crypt(password,username));
                //printf("Auth is %s\n",auth);
                send(client_socket, &auth,strlen(auth),0);
                //visual simulation of registration
                printf("\tRegistring.");
                sleep(2);
                printf("..");
                sleep(2);
                printf(".");
                sleep(2);
                printf("..\n");
                sleep(2);
                //Waiting for server answer
                if(read(client_socket, read_buffer, 1024) > 0){
                    if (!(strcmp(read_buffer, "2")))
                    {
                        printf("You successfully registered. You have to log in now and you are ready to go!\n");
                        log_reg=2;
                        continue;
                    }
                    else
                    {
                        printf("Opps, there was some error with your registration, please try again");
                        continue;
                    }
                    memset(auth,0,sizeof(auth));
                    memset(read_buffer,0,sizeof(read_buffer));
                } 
            }
            else if(log_reg == 2){
                //Log in 
                printf("\n--Log In--\n");
                printf("Username:");
                scanf(" %10[0-9a-zA-Z ]",username);
                fflush(stdout);
                printf("\n Password: ");
                scanf("%s",password);
                fflush(stdout);

                snprintf(auth,sizeof(auth), "%s/%s/%s","log",username,crypt(password,username));
                //printf("Auth is %s\n",auth);
                send(client_socket, &auth,strlen(auth),0);
                //visual simulation of registration
                printf("\tLogging.");
                sleep(2);
                printf("..");
                sleep(2);
                printf(".");
                sleep(2);
                printf("..\n");
                sleep(2);
                //Waiting for server answer
                if(read(client_socket, read_buffer, 1024) > 0){
                    if (!(strcmp(read_buffer, "3")))
                    {
                        printf("Logged in!\n");
                        break;
                    }
                    else if(!strcmp(read_buffer,"WN"))
                    {
                        printf("Opps, it seems that this username is wrong, please try again");
                        continue;
                    }
                    else if(!strcmp(read_buffer,"WP")){
                        printf("Opps, it seems that this combination of username and password is wrong, please try again");
                        continue;
                    }
                    memset(auth,0,sizeof(auth));
                    memset(read_buffer,0,sizeof(read_buffer));
                } 
            }
           
            //printf("Username %s, Pass %s \n",username,password);
        }
        
        
        
        
        int *arg = malloc(sizeof(*arg));
        *arg = client_socket;
        pthread_create(&r_thread, 0, readFromServer, (void*)arg); 
    }
        
    while (1)
    {
        

        printf(": ");
        fgets(send_buffer,1024,stdin);
        send(client_socket, &send_buffer,strlen(send_buffer),0);
        fflush(stdin);
        memset(send_buffer,0,sizeof(send_buffer));


    }

    

    //Closing the sockets
    close(client_socket);
    return 0;
}