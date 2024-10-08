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

    pthread_mutex_init(&mutex, NULL);

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

    while(1) {
        res = recvfrom(sock, line, BUF_SIZE, 0, (struct sockaddr *)&client, &adl);

        if(!getnameinfo((struct sockaddr *)&client, adl, cliIPtext, BUF_SIZE, cliPortText, BUF_SIZE, NI_NUMERICHOST|NI_NUMERICSERV))
            printf("Request from node %s, port number %s\n", cliIPtext, cliPortText);
        else
            puts("Got request, but failed to get client address");

        for(i=0; i<res; i++)
            line1[i]=line[i]; // create a mirror of the text line

        printf("\nLine1: %s\n", line1);
        switch(parser(line1)){

            // Conversion.
            case 0: 
                int decimal, exp;
                decimal = exp = 0;

                for(int i=res-1;i>=0;i--){
                    if(line1[i] == '1')
                        decimal += pow(2, exp);
                    ++exp;
                }

                char* convRes = (char*)calloc(strlen("Conversion result -> ") + log10(decimal) + 1, sizeof(char));
                sprintf(convRes, "Conversion result -> %d", decimal);
                sendto(sock, convRes, strlen("Conversion result -> ") + log10(decimal) + 1, 0, (struct sockaddr *)&client, adl);
                free(convRes);
                break;

            // Operation.
            case 2:

                int i, j;
                char op1[10], op2[10];

                memset(op1, 0, 10);
                memset(op2, 0, 10);

                i = j = 0;

                while(i < 9 && line1[i] != '+' && line1[i] != '-' && line1[i] != '*' && line1[i] != '/'){
                    op1[i] = line1[i];
                    i++;
                }
                

                if(i == res-1)
                    sendto(sock, "Input error.", strlen("Input error."), 0, (struct sockaddr *)&client, adl);

                char op = line1[i++];

                while(i < res && j < 9)
                    op2[j++] = line1[i++];
                
                int o1 = atoi(op1);
                int o2 = atoi(op2);

                int finalRes = 0;

                switch(op){
                    case '+':
                        finalRes = o1 + o2;
                        break;
                    case '-':
                        finalRes = -o2 + o1;
                        break;
                    case '*':
                        finalRes = o1 * o2;
                        break;
                    case '/':
                        finalRes = o1 / o2;
                }

                char signal = 0;
                if(finalRes < 0){
                    finalRes = abs(finalRes);
                    signal = 1;
                }else if(finalRes == 0)
                    finalRes = 1;

                char* finalResStr = (char*)calloc(strlen("Operation result -> ") + log10(finalRes)+1+(int)signal, sizeof(char));
                if(signal)
                    sprintf(finalResStr, "Operation result -> -%d", finalRes);
                else
                    sprintf(finalResStr, "Operation result -> %d", finalRes);
                sendto(sock, finalResStr, strlen("Operation result -> ") + log10(finalRes) + 1 + (int)signal, 0, (struct sockaddr *)&client, adl);
                free(finalResStr);

                break;
            
            case 3:
                int operation = parseDB(line1);

                switch (operation){
                    case 0:
                        int i = strlen("INSERT/");
                        int j = 0;
                        char* key = (char*)calloc(5, sizeof(char));

                        while(line1[i] != '/')
                            *(key+j++) = line1[i++];
                        
                        *(key+i-strlen("INSERT/")) = '\0';
                        ++i;
                        j = 0;

                        char* value = calloc(5, sizeof(char));
                        
                        while(i < res)
                            *(value+j++) = line1[i++];

                        *(value+j) = '\0';

                        

                        int realKey = atoi(key);
                        kvalue_t tmp;
                        tmp.key = realKey;
                        tmp.value = value;

                        struct BinaryTreeNode* n;

                        pthread_mutex_lock(&mutex);

                        char* response;
                        
                        if(database == NULL){
                            database = newNodeCreate(tmp);
                            response = (char*)calloc(26 + strlen(value) + strlen(key), sizeof(char));
                            sprintf(response, "First entry.\nKey: %d\nValue: %s", realKey, value);

                            sendto(sock, response, 26 + strlen(value) + strlen(key), 0, (struct sockaddr *)&client, adl);
                        }else if((n = (struct BinaryTreeNode*)searchNode(database, realKey)) == NULL){
                            database = insertNode(database, tmp);
                            
                            response = (char*)calloc(33 + strlen(value) + strlen(key), sizeof(char));
                            sprintf(response, "New entry inserted.\nKey: %d\nValue: %s", realKey, value);

                            sendto(sock, response, 33 + strlen(value) + strlen(key), 0, (struct sockaddr *)&client, adl);

                        }else{
                            strcpy(n->key.value, tmp.value);
                            response = (char*)calloc(28 + strlen(value) + strlen(key), sizeof(char));
                            sprintf(response, "Entry updated.\nKey: %d\nValue: %s", realKey, value);

                            sendto(sock, response, 28 + strlen(value) + strlen(key), 0, (struct sockaddr *)&client, adl);
                        }
                        pthread_mutex_unlock(&mutex);
                        break;
                    
                    case 1:

                        int ii = strlen("GET/");
                        int jj = 0;

                        char* k = (char*)calloc(res - ii, sizeof(char));

                        while(ii < res)
                            *(k+jj++) = line1[ii++];

                        struct BinaryTreeNode* node;

                        if((node = searchNode(database, atoi(k)))){
                            char* resp = (char*)calloc(strlen("Found key with value ") + strlen(node->key.value), sizeof(char));
                            sprintf(resp, "Found key with value %s", node->key.value);
                            sendto(sock, resp, strlen("Found key with value ") + strlen(node->key.value), 0, (struct sockaddr *)&client, adl);
                        }else{
                            sendto(sock, "Key not found", strlen("Key not found"), 0, (struct sockaddr *)&client, adl);
                        }
                        break;
                    
                    case 2:
                        int iii = strlen("DELETE/");
                        int jjj = 0;

                        char* kk = (char*)calloc(res - ii, sizeof(char));

                        while(iii < res)
                            *(kk+jjj++) = line1[iii++];

                        struct BinaryTreeNode* btn;

                        if((btn = searchNode(database, atoi(kk)))){
                            database = deleteNode(database, btn->key);
                            sendto(sock, "Key was successfully deleted", strlen("Key was successfully deleted"), 0, (struct sockaddr *)&client, adl);
                        }else
                            sendto(sock, "Key not found", strlen("Key not found"), 0, (struct sockaddr *)&client, adl);
                        break;

                    default:
                        break;
                }
                break;

            default:
                sendto(sock, line1, res, 0, (struct sockaddr *)&client, adl);
        }
        
        memset(line, 0, 300*sizeof(char));
        memset(line1, 0, 300*sizeof(char));
        
    }

    // unreachable, but a good practice
    close(sock);
    exit(0);
}


int parseDB(char *line){
    int r1, r2, r3;
    regex_t re1, re2, re3;

    memset(&re1, 0, sizeof(regex_t));
    memset(&re2, 0, sizeof(regex_t));
    memset(&re3, 0, sizeof(regex_t));

    r1 = regcomp(&re1, "^INSERT/[0-9]+/[a-zA-Z0-9]+$", REG_EXTENDED);
    if (r1) {
        fprintf(stderr, "Could not compile regex\n");
        exit(1);
    }

    r2 = regcomp(&re2, "^GET/[0-9]+$", REG_EXTENDED);
    if (r2) {
        fprintf(stderr, "Could not compile regex\n");
        exit(1);
    }

    r3 = regcomp(&re3, "^DELETE/[0-9]+$", REG_EXTENDED);
    if (r3) {
        fprintf(stderr, "Could not compile regex\n");
        exit(1);
    }

    if(regexec(&re1, line, 0, NULL, 0) == 0)
        return 0;
    else if(regexec(&re2, line, 0, NULL, 0) == 0)
        return 1;
    else if(regexec(&re3, line, 0, NULL, 0) == 0)
        return 2;
    return -1;
}

//the following function receives a string and returns an identifier to its representation
//0 – binary text (only has 0s and 1s)
//1 – normal text
//2 – operation
int parser(char *line){
    int ret, ret2, ret3, ret4;
    regex_t regex, regex2, regex3, regex4;

    memset(&regex, 0, sizeof(regex_t));
    memset(&regex2, 0, sizeof(regex_t));
    memset(&regex3, 0, sizeof(regex_t));
    memset(&regex4, 0, sizeof(regex_t));

    // to use regular expressions, first you need to compile the regular expression using regcomp that is
    // stored in a regex_t type
    // the following regular expression ("[01]+$” represents all strings that are constituted only by 0s 
    // and 1s. “[01]” indicates that strings can only have 0s and 1s, + sign indicates that the empty string 
    // is not valid and $ tells the search to continue until the end of string (avoids cases like “0111023”).
    ret = regcomp(&regex, "^[01]+$", REG_EXTENDED);
    if (ret) {
        fprintf(stderr, "Could not compile regex\n");
        exit(1);
    }

    // the following regular expression “[a-zA-Z]” represents all strings that have at least a letter
    ret2 = regcomp(&regex2, "^[a-zA-Z]*$", REG_EXTENDED);
    if (ret2) {
        fprintf(stderr, "Could not compile regex\n");
        exit(1);
    }

    ret3 = regcomp(&regex3, "^[1-9][0-9]*[\\+-\\*][1-9][0-9]*$", REG_EXTENDED);
    if (ret3) {
        fprintf(stderr, "Could not compile regex\n");
        exit(1);
    }

    ret4 = regcomp(&regex4, "^([A-Z]{1,10}/[0-9]+(/[a-zA-Z0-9]+)?)$", REG_EXTENDED);
    if(ret4){
        fprintf(stderr, "Could not compile regex\n");
        exit(1);
    }   

    // after compiling the regex then you can use it in a regexec function to compare with the string that
    // needs to be analysed
    ret = regexec(&regex, line, 0, NULL, 0);
    ret2 = regexec(&regex2, line, 0, NULL, 0);
    ret3 = regexec(&regex3, line, 0, NULL, 0);
    ret4 = regexec(&regex4, line, 0, NULL, 0);

    printf("\n %d %d %d %d \n", ret, ret2, ret3, ret4);

    if(ret == 0)
        return 0;
    else if(ret3 == 0)
        return 2;
    else if(ret4 == 0)
        return 3;
    else return 1;
}