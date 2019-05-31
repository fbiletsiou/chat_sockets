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

int server_socket, client_sock[MAX_USERS], new_socket;
int addrlen,i,j,y,k,l,opt=TRUE;
int valread,max_sd,sd;
int activity;
struct sockaddr_in address_server;
char send_buffer[1024]= {0};
char read_buffer[1024] = {0};
char message[1024] = {};
int flag_no_message = FALSE;
int flag_logged = FALSE;
int flag_deleted = FALSE;
char log_username[10];
int num_of_groups=0;
int group_found =FALSE;
int not_online=FALSE;
int private = FALSE;
int private_err = -1;

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
    users[i].current_group = groups[0].id;
}


//set of socket descriptors
fd_set readfds;

puts("Starting...\n");

//Initializing all the client socket to 0 
for(i=0; i<MAX_USERS; i++){
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
    for(i=0; i<MAX_USERS; i++){
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
        for(i=0; i<MAX_USERS; i++){
            //if empty
            if(client_sock[i] == 0){
                client_sock[i] = new_socket;
                printf("[+] Added to the list of sockets as %d \n",i);
                break;
            }
        }
    }

    //else is some IO operation on some other socket
    for(i=0; i<MAX_USERS; i++){
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
                printf("[DEBUG]PTR IS %s\n",ptr);
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
                                users[y].current_group = 999;
                                for(k=0; k<MAX_USERS; k++){
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
                        memset(read_buffer,0,sizeof(read_buffer));

                    } 
	                
                }
                else if(!strcmp(ptr,"log")){
                    //Logging in 
                    a=0;
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
                            printf("[DEBUG] Username %s, pass %s \n",log_username,ptr);
                            for(y=0; y<MAX_USERS; y++){
                                if(!strcmp(users[y].password,ptr) && !strcmp(users[y].username,log_username)){
                                    printf("[DEBUG] ARIBA\n");
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
                            break;
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
                    memset(log_username,0,sizeof(log_username));
                    memset(read_buffer,0,sizeof(read_buffer));


                }
                else if (!strcmp(ptr,"whoisthere"))
                { //Who is online
                    for(y=0; y<MAX_USERS; y++){
                        if(users[y].logged == 1){
                            snprintf(send_buffer,sizeof(send_buffer),"%s%s", users[y].username,"\n");
                            send(sd,send_buffer, strlen(send_buffer),0);
                            memset(send_buffer,0,sizeof(send_buffer));
                        }
                    }
                    flag_no_message=TRUE;
                    memset(read_buffer,0,sizeof(read_buffer));

                }
                else if (!strcmp(ptr, "groups"))
                {
                    for(int y=0; y<MAX_GROUPS; y++){ //finding the groups
                        if(groups[y].id != -1){
                            snprintf(send_buffer,sizeof(send_buffer),"%s%s",groups[y].name,"\n");
                            send(sd,send_buffer,strlen(send_buffer),0);
                        }
                    }

                    flag_no_message=TRUE;
                    memset(read_buffer,0,sizeof(read_buffer));
                    memset(send_buffer,0,sizeof(send_buffer));

                }
                else if (!strcmp(ptr, "new"))
                {
                    if(num_of_groups > MAX_GROUPS){
                        //No space
                        send(sd, "NS", strlen("NS"),0);
                        printf("[-] Not enough space for more groups\n");
                    }                    


                    else
                    {
                        while (ptr != NULL)
                        {
                            if(a==1){
                                //printf("[DEBUG] new group %s\n",ptr);
                                for (y=1; y<MAX_GROUPS; y++){
                                    if(groups[y].id == -1){
                                        groups[y].id = y;
                                        strcpy(groups[y].name,ptr);
                                        groups[y].group_admin= sd;
                                        groups[y].users[0] = groups[y].group_admin; //first user is always the admin
                                        for(k=1; k<MAX_USERS; k++){
                                            groups[y].users[k] = -1; //rest is empty so far
                                        }
                                        num_of_groups++;
                                        break;
                                    }
                                }
                                 
                            }
                            else if (a>1) {
                                break;
                            }
                            
                            ptr = strtok(NULL, delim);
                            a++;
                        }
                        printf("[+] New group with id %d and name %s by %d\n",groups[y].id,groups[y].name,groups[y].group_admin); 
                        send(sd,"NG", strlen("NG"),0);
                        flag_no_message=TRUE;
                    }
                    memset(read_buffer,0,sizeof(read_buffer));
                    
                }
                else if (!strcmp(ptr, "change"))
                {
                    while (ptr != NULL)
                    {
                        if(a==1){
                            printf("[DEBUG] MOving to group %s\n",ptr);
                            for (y=0; y<MAX_GROUPS; y++){
                                if(!strcmp(groups[y].name,ptr)){ //Finding the wanted group
                                    group_found=TRUE;
                                    break;
                                }
                            }
                            if(group_found){
                                for(k=0; k<MAX_USERS; k++){
                                    if(users[k].id == sd){
                                        users[k].current_group = groups[y].id;
                                        break;
                                    }
                                }    
                            }
     
                            }
                            else if (a>1) {
                                break;
                            }
                            
                            ptr = strtok(NULL, delim);
                            a++;
                        }
                        if(group_found){
                            send(sd,"MO", strlen("MO"),0);
                            group_found=FALSE;  
                        }
                        else if(!group_found){
                            send(sd,"NF", strlen("NF"),0);
                        }
                        
                        flag_no_message=TRUE;
                    
                    memset(read_buffer,0,sizeof(read_buffer));
                    
                }
                else if (!strcmp(ptr, "delgroup"))
                {
                    while (ptr != NULL)
                    {
                        if(a==1){
                            printf("[DEBUG] deleting group %s\n",ptr);
                            group_found=FALSE;
                            for (y=0; y<MAX_GROUPS; y++){
                                if(!strcmp(groups[y].name,ptr)){ //Finding the wanted group
                                    group_found=TRUE;
                                    if(groups[y].group_admin == sd){
                                        //Deleting the group
                                        for(k=0; k<MAX_USERS; k++){//if a user is at the group now
                                            if(users[k].current_group == groups[y].id){
                                                users[k].current_group = 999; //back in the general
                                                send(users[k].id,"BG", strlen("BG"),0);
                                            }
                                        }
                                        groups[y].id = -1;
                                        groups[y].group_admin = -1;
                                        strcpy(groups[y].name,"");
                                        for(k=0; k<50; k++){
                                            groups[y].users[k] =-1;
                                        }
                                        flag_deleted = TRUE;
                                    }
                                    break;
                                }
                            }
                        }
                        else if (a>1) {
                            break;
                        }
                            
                        ptr = strtok(NULL, delim);
                        a++;
                    }
                    if(flag_deleted){
                        send(sd,"DE", strlen("DE"),0);
                        group_found=FALSE; 
                        flag_deleted = FALSE;                    
                        flag_no_message=TRUE;
                    }
                    else if(!group_found){
                        send(sd,"NF", strlen("NF"),0);
                        flag_no_message=TRUE;
                    }
                    else if(!flag_deleted){
                        send(sd,"NR", strlen("NR"),0);
                        group_found=FALSE; 
                        flag_deleted = FALSE;
                        flag_no_message=TRUE;
                    }
                                        
                    memset(read_buffer,0,sizeof(read_buffer));
                    
                }
                else if (!strcmp(ptr, "private"))
                {
                    while (ptr != NULL)
                    {
                        if(a==1){
                            printf("[DEBUG] private to %s\n",ptr);
                            not_online = FALSE;
                            for(y=0; y<MAX_USERS; y++){
                                if(!strcmp(ptr, users[y].username)){
                                    private_err = FALSE;
                                    if(users[y].logged != 1){
                                        not_online = TRUE;
                                        break;
                                    }
                                    break;
                                }
                            }
                            if(private_err == -1 || not_online){
                                //if the username is wrong or the user in not online
                                break;
                            }
                        }
                        else if (a==2)
                        {
                            printf("[DEBUG] private msg %s\n",ptr);
                            snprintf(read_buffer,sizeof(read_buffer),"%d>%s>",users[y].id,ptr);
                            private=TRUE;
                        }
                        
                        else if (a>2) {
                            break;
                        }
                            
                        ptr = strtok(NULL, delim);
                        a++;
                    }
                    if(not_online){
                        send(sd,"LO", strlen("LO"),0);
                        flag_no_message=TRUE;
                    }
                    else if(private_err == -1){
                        send(sd,"WU", strlen("WU"),0);
                        private_err = -1;
                        flag_no_message=TRUE;
                    }
                                    
                }
                
                

                if(flag_no_message){ 
                    flag_no_message=FALSE;
                }
                else{ //Simple message
                    for(j=0; j<MAX_USERS; j++){
                        if(client_sock[j] != 0){
                            printf("[DEBUG] sender %d , receiver %d \n",sd,client_sock[j]);
                            if(client_sock[j] != sd){
                                if(private){ //PRIVATE MESSAGE
                                    char *pri = strtok(read_buffer, ">");
                                    int b =0;
                                    char sender[10];
                                    int receiver = -1;
                                    while(pri != NULL){
                                        if(b==0){ //receiver
                                            for(y=0; y<MAX_USERS; y++){
                                                if(atoi(pri) == users[y].id){
                                                    receiver=users[y].id;
                                                }
                                                if(sd == users[y].id){
                                                    strcpy(sender,users[y].username);
                                                }
                                            }
                                        }
                                        else if(b == 1){
                                            snprintf(message, sizeof(message),"[%s (dm)] %s",sender,pri);
                                            send(receiver, message, strlen(message), 0);    
                                            private=FALSE;
                                            printf("[DEBUG] message is %s\n",message);
                                        }
                                        else if(b > 1){
                                            printf("!!!\n");
                                            break;
                                        }
                                        
                                        pri = strtok(NULL, ">");
                                        b++;
                                    }
                                    memset(sender,0,sizeof(sender));   
                                    memset(message,0,sizeof(message));
                                    memset(read_buffer,0,sizeof(read_buffer));
                                    break;                    
                               }
                                else{
                                    printf("[DEBUG] message is %s\n",message);

                                    //finding the name of the sender
                                    for(y=0; y<MAX_USERS; y++){
                                        if(users[y].id == sd){ //SENDER id , group
                                            for(k=0; k<MAX_GROUPS; k++){
                                                if(users[y].current_group == groups[k].id)
                                                    break;
                                            }
                                            break;
                                        }
                                    }
                                    for(l=0; l<MAX_USERS;l++){
                                        if(users[l].id == client_sock[j]) //RECEIVER id
                                            break;
                                    }  
                                    if(users[l].current_group == users[y].current_group){
                                        snprintf(message,sizeof(message),"||%s||%s%s%s",groups[k].name,users[y].username, " > ",read_buffer);
                                        printf("[DEBUG] Sending to user %d --> %s \n",j,message);
                                        send(client_sock[j], message, strlen(message), 0);    
                                    }

                                }
                            
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
