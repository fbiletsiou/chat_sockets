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
#define MAX_USERS 30
#define MAX_GROUPS 50

int main(int argc , char *argv[]){

int max_clients = 30;
int server_socket, client_sock[max_clients], new_socket;
int addrlen,i,j,y,opt=TRUE;
int valread,max_sd,sd;
int activity;
struct sockaddr_in address_server;
char send_buffer[1024]= {0};
char read_buffer[1024] = {0};
char message[1024] = {};
int flag_no_message = FALSE;
int flag_logged = FALSE;
char log_username[10];

typedef struct group
{
    int id;
    int group_admin;
    char name[20];
    int users[MAX_USERS];
}topic;

struct group groups[MAX_GROUPS];

//general public group
groups[0].id = 999;
groups[0].group_admin =-999;
strcpy(groups[0].name,"General");
for(y=0; y<MAX_USERS; y++){
    groups[0].users[y]=-1;
}

for(y=1; y<MAX_GROUPS; y++){
    groups[y].id = -1;
}


typedef struct user
{
    char username[10];
    char password[50];
    int id;
    int logged;
    int friends[MAX_USERS];
    int my_groups[MAX_GROUPS];
    int current_group;
}u;
struct user users[MAX_USERS];
int users_reg_now=0;
int users_log_now=0;
char log_err[3]={};

//empty array
for (i=0; i<MAX_USERS; i++){
    strcpy(users[i].username,"0");
    strcpy(users[i].password,"0");
    users[i].logged=-1;
    users[i].current_group = users[i].my_groups[0];
}


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

                users_log_now--;
                for(y=0; y<MAX_USERS; y++){
                    if(users[y].id == sd){
                        //log out
                        users[y].logged = -1;
                    }
                }
                //Closing the socket and mark it as 0 for reuse
                close(sd);
                client_sock[i] = 0;
            }
            else //sending back the message that came in 
            {
                char delim[] = "/";
           	    char *ptr = strtok(read_buffer, delim);
                int a=0;
                if(!strcmp(ptr,"reg")){
                    //registration
                    if(users_reg_now == 30){
                        printf("Sorry There is not enough space in the chat.Bye\n");
                        close(sd);
                        client_sock[i] = 0;
                    }else
                    { // there is space
                        for(y=0; y<MAX_USERS; y++){ //finding available space in the array
                            if(!strcmp(users[y].username,"0")){
                                break;
                            }
                        }
                        while(ptr != NULL)
                        {
                            if(a==1){
                                //username
                                strcpy(users[y].username,ptr);
                            }
                            else if(a==2)
                            {
                                //password
                                strcpy(users[y].password,ptr);
                                users[y].id = sd;
                                users[y].my_groups[0]= groups[0].id; //Everyone is in the general group
                                for(int k=0; k<MAX_USERS; k++){
                                    if (groups[0].users[k] == -1)
                                    {
                                        groups[0].users[k] = users[y].id;
                                        break;
                                    }
                                    
                                }
                                printf("[+] Creation user %s-id %d-password %s\n",users[y].username,users[y].id,users[y].password);
                                break;
                            }
                            else if(a==0){
                                //thats already done
                            }
                            else
                            {
                                perror("Wrong registry info");
                                exit(EXIT_FAILURE);
                            }
                            ptr = strtok(NULL, delim);
                            a++;
                        }  
                        users_reg_now++;
                        send(sd, "2", strlen("2"), 0); 
                        flag_no_message = TRUE;
                    } 
	                
                }
                else if(!strcmp(ptr,"log")){
                    //Logging in 
                    while(ptr != NULL){
                        if(a==1){
                            //username
                            for(y=0; y<MAX_USERS; y++){
                                if(!strcmp(ptr, users[y].username)){
                                    strcpy(log_username,ptr);
                                    break;
                                }
                            }
                            if(y>=MAX_USERS){
                                //The user is not registered
                                flag_logged=FALSE;
                                strcpy(log_err,"WN"); //WRONG NAME
                                break;
                            }
                        }
                        else if(a==2){
                            //password
                            printf("Username %s, pass %s \n",log_username,ptr);
                            for(y=0; y<MAX_USERS; y++){
                                if(!strcmp(users[y].password,ptr) && !strcmp(users[y].username,log_username)){
                                    printf("ARIBA\n");
                                    flag_logged =TRUE;
                                    users[y].logged=1;
                                    break;
                                }
                            }
                            if(flag_logged==FALSE){
                                strcpy(log_err,"WP"); //WRONG PASSWORD
                            }
                        }
                        else if(a==0){
                            //thats already done
                        }
                        else
                        {
                            perror("Wrong registry info");
                            exit(EXIT_FAILURE);
                        }
                        
                        ptr = strtok(NULL, delim);
                        a++;
                    }  
                    if(flag_logged){ //the user successfully logged in 
                        users_log_now++;
                        send(sd, "3", strlen("3"), 0);
                        flag_no_message=TRUE;

                    }
                    else{
                        send(sd, log_err, strlen(log_err), 0);
                        flag_no_message=TRUE;    
                    }
                    

                }
                else if (!strcmp(ptr,"whoisthere"))
                { //Who is online
                    for(y=0; y<MAX_USERS; y++){
                        if(users[y].logged == 1){
                            send(sd,users[y].username, strlen(users[y].username),0);
                        }
                    }
                    flag_no_message=TRUE;
                }
                else if (!strcmp(ptr, "groups"))
                {
                    for (y=0; y<MAX_USERS; y++)
                    {
                        if(users[y].id == sd){ //Finding the user                            
                            break;
                        }
                    }
                    for(int k=0; k<MAX_GROUPS; k++){ //finding the groups
                        for(int l=0; l<MAX_GROUPS; l++){//finding the name of the group
                            if (users[y].my_groups[k] == groups[l].id)
                            {
                                send(sd,groups[l].name,strlen(groups[l].name),0);
                            }
                        }
                    }

                    flag_no_message=TRUE;  
                }
                else if (!strcmp(ptr, "new/"))
                {
                    for (y=1; y<MAX_GROUPS; y++)
                    {
                        if(groups[y].id != -1){
                            break; //Available space for new group
                        }
                    }
                    if(y >= MAX_GROUPS){
                        //No space
                        send(sd, "NS", strlen("NS"),0);
                        printf("[-] Not enough space for more groups\n");
                    }
                    else
                    {
                        while (ptr != NULL)
                        {
                            groups[y].id = y;
                            strcpy(groups[y].name,ptr);
                            groups[y].group_admin= sd;
                            groups[y].users[0] = groups[y].group_admin;
                            for(int k=1; k<MAX_USERS; k++){
                                groups[y].users[k] = -1;
                            }

                            ptr = strtok(NULL, delim);
                        }
                        
                    }
                }
                
                
                

                if(flag_no_message){ 
                    printf("[+] Authentication for %d Done\n",sd);
                    flag_no_message=FALSE;
                }
                else{ //Simple message
                    for(j=0; j<max_clients; j++){
                        if(client_sock[j] != 0){
                            printf("sender %d , receiver %d \n",sd,client_sock[j]);
                            if(client_sock[j] != sd){
                                //finding the name of the sender
                                for(y=0; y<MAX_USERS; y++){
                                    if(users[y].id == sd){
                                        break;
                                    }
                                }
                                snprintf(message,sizeof(message),"%s%s%s",users[y].username, " > ",read_buffer);
                                printf("Sending to user %d --> %s \n",j,message);
                                send(client_sock[j], message, strlen(message), 0); 
                            }
                            
                        }
                    }
                    memset(read_buffer,0,sizeof(read_buffer));
                    memset(message,0,sizeof(message));                    
                }


               
              
            }
            
        }
    }



}






//Closing the sockets
close(server_socket);
return 0;
}
