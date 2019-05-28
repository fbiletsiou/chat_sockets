// Florina Biletsiou
#include <stdio.h>
#include <unistd.h> 
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h> 
#include <arpa/inet.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h> 
#include <sys/time.h>

#define TRUE 1
#define FALSE 0
#define PORT 8080

int main(int argc , char *argv[]){

int max_clients = 30;
int server_socket, client_sock[max_clients], new_socket;
int addrlen,i,j,opt=TRUE;
int valread,max_sd,sd;
int activity;
struct sockaddr_in address_server;
char send_buffer[1024]= {0};
char read_buffer[1024] = {0};
char message[1024] = {0};

//set of socket descriptors
fd_set readfds;

puts("Starting...\n");

//Initializing all the client socket to 0 
for(i=0; i<max_clients; i++){
    client_sock[i] = 0;
}
printf("[+] Client sockets init done\n");

//creating a file descriptor for the socket
server_socket = socket(AF_INET, SOCK_STREAM, 0);
if(server_socket == -1){
    perror("[-]Error in socket()\n");
    exit(EXIT_FAILURE);
}else{
    printf("[+] Socket Created \n");
}

//setting the socekt options for reuse
if(setsockopt(server_socket,SOL_SOCKET,SO_REUSEADDR | SO_REUSEPORT,(char*) &opt, sizeof(opt)) < 0 ){
    perror("[-] Error in setsockopt() \n");
    exit(EXIT_FAILURE);
}

//type of socket created
address_server.sin_family = AF_INET;
address_server.sin_addr.s_addr = INADDR_ANY;
address_server.sin_port = htons(PORT);

//binding the address_server with the socket descriptor
if(bind(server_socket,(struct sockaddr *) &address_server,sizeof(address_server)) < 0){
    perror("[-] Error in bind()\n");
    exit(EXIT_FAILURE);
}else
{
    printf("[+] Binding Done \n");
}


//listening for any connections
//maximum 3 pending connections 
if(listen(server_socket, 3) < 0){
    perror("[-] Error in listen()\n");
    exit(EXIT_FAILURE);
}else
{
    printf("[+] Listening on port %d\n",PORT);
}


//accepting connection requests 
addrlen = sizeof(struct sockaddr_in);
puts("Waiting for incomming connections...\n");

while (TRUE)
{

    //Cleaning the socket set
    FD_ZERO(&readfds);

    //Adding the server socket to the set
    FD_SET(server_socket, &readfds);
    max_sd = server_socket;

    //Adding the client socket to the set
    for(i=0; i<max_clients; i++){
        //socket descriptor
        sd = client_sock[i];

        //socket descriptor validation
        if(sd > 0){
            FD_SET(sd, &readfds);
        }

        //highest file descriptor for select
        if(sd > max_sd){
            max_sd = sd;
        }
    }

    //Waiting for activity
    activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

    if((activity < 0) && (errno != EINTR)){
        perror("[-] Error in select()\n");
    }

    //If there is activity in the server socket 
    // it is an incoming connection
    if(FD_ISSET(server_socket, &readfds)){
        if((new_socket = accept(server_socket,(struct sockaddr *) &address_server, (socklen_t *) &addrlen)) < 0){
            perror("[-] Error in accept()\n");
            exit(EXIT_FAILURE);
        }else{
            printf("[+] Acception done\n New Connection with id = %d , ip = %s , port = %d \n",new_socket, inet_ntoa(address_server.sin_addr),ntohs(address_server.sin_port));
        }

        //Adding the new socket to array of sockets
        for(i=0; i<max_clients; i++){
            //if empty
            if(client_sock[i] == 0){
                client_sock[i] = new_socket;
                printf("[+] Added to the list of sockets as %d \n",i);
                break;
            }
        }
    }

    //else is some IO operation on some other socket
    for(i=0; i<max_clients; i++){
        sd = client_sock[i];

        if(FD_ISSET(sd , &readfds)){
            // Check if it was for closing
            //Reading the incoming message
            if((valread = read(sd, read_buffer, 1024)) == 0 ){
                getpeername(sd,(struct sockaddr*)&address_server, (socklen_t*)&addrlen);
                printf("Client disconnected , %s:%d\n",inet_ntoa(address_server.sin_addr), ntohs(address_server.sin_port));

                //Closing the socket and mark it as 0 for reuse
                close(sd);
                client_sock[i] = 0;
            }
            else //sending back the message that came in 
            {
                char name;
                sprintf(name, "%d", sd);
                strcat(message,name);
                strcat(message,":");
                strcat(message,read_buffer);
                
                for(j=0; j<max_clients; j++){
                    if(client_sock[j] != 0){
                        printf("to %d: %s \n",j,message);

                        send(client_sock[j], message, strlen(message), 0); 
                        
                    }
                }
                memset(read_buffer,0,sizeof(read_buffer));
                memset(message,0,sizeof(message));

               
              
            }
            
        }
    }



}






//Closing the sockets
close(server_socket);
return 0;
}
