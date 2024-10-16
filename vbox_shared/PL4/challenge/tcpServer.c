#include <stdio.h>
#include <string.h> // bzero()
#include <sys/socket.h>
#include <arpa/inet.h> // inet_addr()
#include <unistd.h> // read(), write(), close()
#include <time.h>
#include <netdb.h>

#define PORT 9998
#define CLIENT_MESSAGE 32
#define SERVER_MESSAGE 32

int main(){

    int sockfd, client_sock;
    struct sockaddr_in server_addr, client_addr;
    char server_message[SERVER_MESSAGE], client_message[CLIENT_MESSAGE];
    struct hostent *host_entry;

    char host_buffer[256];
    gethostname(host_buffer, sizeof(host_buffer));
    host_entry = gethostbyname(host_buffer);

    // Clean buffers:
    bzero(server_message, sizeof(server_message)); // Nota: O bzero e o memset conseguem fazer o mesmo (limpar o buffer), contudo o bzero é mais eficiente.
    bzero(client_message, sizeof(client_message));

    // Create socket:
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if(sockfd < 0){
        printf("Error while creating socket\n");
        return -1;
    }
    printf("Socket created successfully\n");

    // Set port and IP:
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("0.0.0.0");

    // Bind to the set port and IP:
    if(bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        printf("Couldn't bind to the port\n");
        close(sockfd);
        return -1;
    }

    printf("Done with binding\n");

    // Listen for clients:
    if(listen(sockfd, SOMAXCONN) < 0){
        printf("Error while listening\n");
        close(sockfd);
        return -1;
    }
    printf("\nListening for incoming connections.....\n");

    for(;;){
        // Accept an incoming connection:
        socklen_t slen=sizeof(client_addr);
        client_sock = accept(sockfd, (struct sockaddr*)&client_addr, &slen);
        if (client_sock < 0){
            printf("Can't accept\n");
            close(sockfd);
            return -1;
        }

        if(!fork()){

            time_t begin;
            time(&begin);

            printf("Client connected at IP: %s and port: %i\n", inet_ntoa(client_addr.sin_addr),
                ntohs(client_addr.sin_port));
            
            for(;;){

                // Receive client's message:
                if( read(client_sock, client_message, sizeof(client_message)) < 0){
                    printf("Can't read\n");
                    return -1;
                }

                if(client_message[strlen(client_message)-1] != '\n')
                    strcat(client_message, "\n");

                printf("%s\n", client_message);

                if(strcmp(client_message, "uptime\n") == 0){
                    time_t end;
                    time(&end);
                    int seconds = (end - begin);
                    int hours = seconds / 3600;
                    seconds %= 3600;
                    int minutes = seconds / 60;
                    seconds %= 60;
                    sprintf(server_message, "%dh:%dm:%ds\n", hours, minutes, seconds);
                }else if(strcmp(client_message, "address\n") == 0){
                    // Note: h_addr_list is pointing to zero, because the interface that I want to print is the first one (presented in ifconfig command).
                    sprintf(server_message, "%s:%d\n", inet_ntoa(*((struct in_addr*)host_entry->h_addr_list[0])), (int) ntohs(server_addr.sin_port));
                }else if(strcmp(client_message, "user\n") == 0){
                    sprintf(server_message, "%s:%d\n", inet_ntoa(client_addr.sin_addr), (int) ntohs(client_addr.sin_port));
                }else
                    strcpy(server_message, "ACK\n");
                
                if(write(client_sock, server_message, sizeof(server_message)) < 0){
                    printf("Can't write\n");
                    return -1;
                }

                bzero(client_message, CLIENT_MESSAGE);
                bzero(server_message, SERVER_MESSAGE);
            }
        }
    }
    // Closing the socket:
    close(client_sock);
    close(sockfd);
    return 0;
}
