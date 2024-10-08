#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#define PORT 8080
#define MAXLINE 1024
//#define MSG_CONFIRM 0 //possibly needed for MAC users
// Driver code
int main(int argc, char **argv) {
    if( argc != 2 ) {
        printf("usage: %s <ip address>\n", argv[0]);
        exit(0);
    }

    int sockfd;
    char buffer[MAXLINE];
    char *hello = "Hello Server";
    struct sockaddr_in servaddr;

    // Creating socket file descriptor
    // SOCK_DGRAM indicates the type of socket to be created which in this case is a UDP
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket creation failed");
        exit(-1);
    }
    memset(&servaddr, 0, sizeof(servaddr));

    // Filling server information
    servaddr.sin_family = AF_INET; //IPV4 | to use IPV6 just use AF_INET6
    servaddr.sin_port = htons(PORT);
    servaddr.sin_addr.s_addr = inet_addr(argv[1]);
    int n;
    sendto(sockfd, (const char *)hello, strlen(hello), MSG_CONFIRM, (const struct sockaddr *)
    &servaddr, sizeof(servaddr));
    printf("Hello message sent.\n");
    socklen_t slen=sizeof(servaddr);
    n = recvfrom(sockfd, (char *)buffer, MAXLINE, MSG_WAITALL, (struct sockaddr *) &servaddr, &slen);
    buffer[n] = '\0'; //adds a “\0” character to terminate the received string
    printf("Server : %s\n", buffer);
    close(sockfd);
    return 0;
}