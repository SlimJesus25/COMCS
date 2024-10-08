#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <regex.h>
#include <math.h>
#include <string.h>
#include <pthread.h>

#define BUF_SIZE 50
#define SERVER_PORT "9999"


int main(void) {
    struct sockaddr_storage client;
    int err, sock, res, i;
    unsigned int adl;
    char line[BUF_SIZE], line1[BUF_SIZE];
    char cliIPtext[BUF_SIZE], cliPortText[BUF_SIZE];
    struct addrinfo req, *list;
    bzero((char *)&req, sizeof(req));

    // request a IPv6 local address will allow both IPv4 and IPv6 clients to use it
    req.ai_family = AF_INET6;
    req.ai_socktype = SOCK_DGRAM;
    req.ai_flags = AI_PASSIVE; // local address
    err=getaddrinfo(NULL, SERVER_PORT , &req, &list);

    if(err) {
        printf("Failed to get local address, error: %s\n",gai_strerror(err));
        exit(1);
    }

    sock=socket(list->ai_family, list->ai_socktype, list->ai_protocol);

    if(sock == -1) {
        perror("Failed to open socket"); freeaddrinfo(list);
        exit(1);
    }

    if(bind(sock,(struct sockaddr *)list->ai_addr, list->ai_addrlen)==-1) {
        perror("Bind failed");close(sock);freeaddrinfo(list);
        exit(1);
    }

    freeaddrinfo(list);
    puts("Listening for UDP requests (IPv6/IPv4). Use CTRL+C to terminate the server");
    adl=sizeof(client);

    int requests = 0;
    while(1) {
        res = recvfrom(sock, line, BUF_SIZE, 0, (struct sockaddr *)&client, &adl);
        requests++;

        if(!getnameinfo((struct sockaddr *)&client, adl, cliIPtext, BUF_SIZE, cliPortText, BUF_SIZE, NI_NUMERICHOST|NI_NUMERICSERV))
            printf("Request from node %s, port number %s\n", cliIPtext, cliPortText);
        else
            puts("Got request, but failed to get client address");

        for(i=0; i<res; i++)
            line1[i]=line[i]; // create a mirror of the text line

        char response[50];
        if(requests % 10 == 0)
            strcpy(response, "LightOn\0");
        else
            strcpy(response, "Response received\0");

        sendto(sock, response, 50, 0, (struct sockaddr *)&client, adl);
    }
}