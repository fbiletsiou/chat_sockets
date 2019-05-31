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
#define BOLDCYAN    "\033[1m\033[36m"
#define RESET       "\033[0m"
#define BOLDWHITE   "\033[1m\033[37m"
#define RED         "\033[31m"
#define GREEN       "\033[32m"


void *readFromServer(void *arg){
    char read_buffer[1024] = {0};
    int client_socket = *((int *)arg); 
    free(arg);
    int valread;

    while (1)
    {
        if(valread= read(client_socket, read_buffer, 1024) > 0){
            if(!strcmp(read_buffer,"NG")){
                printf(GREEN "[+] New group added" RESET "\n");
            }
            else if(!strcmp(read_buffer,"NS")){
                printf(RED "[-] Not enough space for new groups" RESET "\n");
            }
            else if(!strcmp(read_buffer,"MO")){
                printf(GREEN "[+] Changed Group" RESET "\n");
            }
            else if(!strcmp(read_buffer,"NF")){
                printf(RED "[-] Group not found" RESET "\n");
            }
            else if(!strcmp(read_buffer,"BG")){
                printf(GREEN "[+] The group you were was deleted by it's admin, you are now in 'General'" RESET "\n");
            }
            else if(!strcmp(read_buffer,"NR")){
                printf(RED "[-] You are not the admin of this group " RESET "\n");
            }
            else if(!strcmp(read_buffer,"WU")){
                printf(RED "[-] The username you typed doesnt exist, type 'whoisthere/' " RESET "\n");
            }
            else if(!strcmp(read_buffer,"LO")){
                printf(RED "[-] The user is not online, type 'whoisthere/' to see who is online " RESET "\n");
            }
            
            else
            {
                printf("%s\n", read_buffer);
            }
            memset(read_buffer,0,sizeof(read_buffer)); 
            

        }  
    }
}

void instructions(){
//INSTRUCTIONS
    printf(BOLDCYAN "\tList of available actions:\n" RESET);
    printf(BOLDWHITE "\t'Ctrl + C' to exit the chat \n");
    printf("\t'whoisthere/' to see the online users \n");
    printf("\t'groups/' to display the availabe groups\n");
    printf("\t'new/' to create a new group\n");
    printf("\t'change/' to change group\n");
    printf("\t'private/' if you want to start a private chat with someone\n ");
    printf("\t'delgroup/' if you are the admin of the group and you want to delete it\n");
    printf("\t'help/' to see the instructions \n" RESET);
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
    char new_group_name[20] = {0};
    char moving_group_name[20] = {0};
    char delete_group_name[20] = {0};
    char private_user[10] = {0};
    char private_msg[1024] = {0};

    //Creating a file descriptor for the socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(client_socket < 0){
        perror(RED "[-] Error in socket()\n" RESET);
        exit(EXIT_FAILURE);
    }else
    {
        printf(GREEN "[+] Socket created \n" RESET);
    }

    //Socket options
    memset(&addr_server, '0', sizeof(addr_server));

    addr_server.sin_family = AF_INET;
    addr_server.sin_port = htons(PORT);

    //Convert IPv4 and IPv6 addresses from text to binary form 
    if (inet_pton(AF_INET, "127.0.0.1", &addr_server.sin_addr) <= 0)
    {
        perror(RED "[-] Invalid address, Address not supported \n" RESET);
        exit(EXIT_FAILURE);
    }

    //connecting to the server

    if(connect(client_socket,(struct sockaddr *) &addr_server, sizeof(addr_server)) < 0){
        perror(RED "[-] Error in connect()\n" RESET);
        exit(EXIT_FAILURE);
    }else
    {
        printf(GREEN "[+] Connection with the server successful \n" RESET);
        
        instructions();

        //REGISTRATION 
        printf("Type:\n (1) for Registration to the chat\n (2) for Log in\n");
        char *p;
        int temp[10];
        while (1)
        {
            fgets(temp, sizeof(temp), stdin);

            log_reg = strtol(temp, &p, 10);
            if (p == temp || *p != '\n') {
                printf("You need to enter 1 or 2 , try again please \n");
            } 
            else{
                //scanf("%d",&log_reg);
                //fflush(stdout);
                //fflush(stdin);
                if(log_reg == 1 || log_reg ==2){
                    break;
                }

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
                printf("Registring.");
                fflush(stdout);
                sleep(1);
                printf(".");
                fflush(stdout);
                sleep(1);
                printf(".");
                fflush(stdout);
                sleep(1);
                printf(".\n");
                fflush(stdout);
                sleep(1);
                //Waiting for server answer
                if(read(client_socket, read_buffer, 1024) > 0){
                    if (!(strcmp(read_buffer, "2")))
                    {
                        printf(GREEN "[+] You successfully registered. You have to log in now and you are ready to go!\n" RESET);
                        log_reg=2;
                        continue;
                    }
                    else
                    {
                        printf(RED "[-]Opps, there was some error with your registration, please try again" RESET);
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
                printf("\nPassword: ");
                scanf("%s",password);
                fflush(stdout);

                snprintf(auth,sizeof(auth), "%s/%s/%s","log",username,crypt(password,username));
                //printf("Auth is %s\n",auth);
                send(client_socket, &auth,strlen(auth),0);
                printf("Logging.");
                fflush(stdout);
                sleep(1);
                printf(".");
                fflush(stdout);
                sleep(1);
                printf(".");
                fflush(stdout);
                sleep(1);
                printf(".\n");

                //Waiting for server answer
                if(read(client_socket, read_buffer, 1024) > 0){
                    //printf("[DEBUG] answer is %s\n",read_buffer);
                    if (!(strcmp(read_buffer, "3")))
                    {
                        printf(GREEN "[+] Logged in!\n" RESET);
                        memset(auth,0,sizeof(auth));
                        memset(read_buffer,0,sizeof(read_buffer));
                        break;
                    }
                    else if(!strcmp(read_buffer,"WN"))
                    {
                        printf(RED "[-] Opps, it seems that this username is wrong, please try again" RESET);
                        memset(auth,0,sizeof(auth));
                        memset(read_buffer,0,sizeof(read_buffer));
                        continue;
                    }
                    else if(!strcmp(read_buffer,"WP")){
                        printf(RED "[-] Opps, it seems that this combination of username and password is wrong, please try again" RESET);
                        memset(auth,0,sizeof(auth));
                        memset(read_buffer,0,sizeof(read_buffer));
                        continue;
                    }

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
        fflush(stdin);
        fgets(send_buffer,1024,stdin);
        fflush(stdin);
        fflush(stdout);
        send_buffer[strcspn(send_buffer, "\n")]=0;

        if(strcmp(send_buffer,"new/") == 0){
            printf("--CREATION OF GROUP--\n");
            printf("Name of the group:");
            fgets(new_group_name,20,stdin);
            new_group_name[strcspn(new_group_name, "\n")]=0;
            fflush(stdin);
            snprintf(send_buffer,sizeof(send_buffer), "%s/%s/","new",new_group_name);
            memset(new_group_name,0,sizeof(new_group_name));
        }
        else if(strcmp(send_buffer,"change/")==0){
            printf("--CHANGE OF GROUP--\n");
            printf("Moving to:");
            fgets(moving_group_name,20,stdin);
            moving_group_name[strcspn(moving_group_name, "\n")]=0;
            fflush(stdin);
            snprintf(send_buffer,sizeof(send_buffer), "%s/%s/","change",moving_group_name);
            memset(moving_group_name,0,sizeof(moving_group_name));
        }
        else if(strcmp(send_buffer,"delgroup/") ==0){
            printf("--DELETION OF GROUP--\n");
            printf("**Beware, you can only delete groups that you created**\n");
            printf("Group to delete:");
            fgets(delete_group_name,20,stdin);
            delete_group_name[strcspn(delete_group_name,"\n")] = 0;
            fflush(stdin);
            snprintf(send_buffer,sizeof(send_buffer),"%s/%s/","delgroup",delete_group_name);
            memset(delete_group_name,0,sizeof(delete_group_name));
        }
        else if(strcmp(send_buffer,"private/") ==0){
            printf("(private) to:");
            fgets(private_user,10,stdin);
            private_user[strcspn(private_user,"\n")] = 0;
            fflush(stdin);
            printf("(msg) to %s:",private_user);
            fgets(private_msg,1024,stdin);
            private_msg[strcspn(private_msg,"\n")] = 0;
            fflush(stdin);
            snprintf(send_buffer,sizeof(send_buffer),"%s/%s/%s","private",private_user,private_msg);
            memset(private_user,0,sizeof(private_user));
            memset(private_msg,0,sizeof(private_msg));
        }
        else if(strcmp(send_buffer,"help/")==0){
            instructions();
            memset(send_buffer,0,sizeof(send_buffer));
            continue;
        }

        send(client_socket, &send_buffer,strlen(send_buffer),0);
        fflush(stdin);
        memset(send_buffer,0,sizeof(send_buffer));

    }

    

    //Closing the sockets
    close(client_socket);
    return 0;
}