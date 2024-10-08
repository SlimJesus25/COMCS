#include <stdio.h>
#include <string.h> // bzero()
#include <stdlib.h> // exit()
#include <sys/socket.h>
#include <arpa/inet.h> // inet_addr()
#include <unistd.h> // read(), write(), close()

int main(int argc, char **argv) {
    int sockfd;
    struct sockaddr_in server_addr;
    char server_message[2000], client_message[2000];

    if(argc!=2) {
        puts("Server's IPv4 name is required as argument");
        exit(1);
    }

    // Clean buffers:
    bzero(server_message, sizeof(server_message));
    bzero(client_message, sizeof(client_message));

    // Create socket:
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){
        printf("Unable to create socket\n");
        return -1;
    }
    printf("Socket created successfully\n");

    // Set port and IP the same as server-side:
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(2001);
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);

    // Send connection request to server:
    if(connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        printf("Unable to connect\n");
        close(sockfd);
        return -1;
    }
    printf("Connected with server successfully\n");

    // Get input from the user:
    printf("Enter message: \n");
    fgets(client_message,sizeof(client_message), stdin);

    // Send the message to server:
    if( write(sockfd, client_message, sizeof(client_message)) < 0){
        printf("Unable to send message\n");
        close(sockfd);
        return -1;
    }

    // Receive the server's response:
    if( read(sockfd, server_message, sizeof(server_message)) < 0){
        printf("Error while receiving server's msg\n");
        close(sockfd);
        return -1;
    }
    printf("Server's response: %s\n",server_message);

    // Close the socket:
    close(sockfd);
    return 0;
}