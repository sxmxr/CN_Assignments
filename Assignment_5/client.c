/*Name : Samarth Shah
AU1841145
Problem Statement: To implement Client-Server Chat application and Calculator.
*/

/*
	Client Side
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ctype.h>

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[256];
    if (argc < 3)
    {
        fprintf(stderr, "usage %s hostname port\n", argv[0]);
        exit(0);
    }
    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    server = gethostbyname(argv[1]);
    if (server == NULL)
    {
        fprintf(stderr, "ERROR, no such host\n");
        exit(0);
    }
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
          (char *)&serv_addr.sin_addr.s_addr,
          server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR connecting");

    int num1;
    int num2;
    int ans;
    int choice;
    while (1)
    {
        bzero(buffer, 256);
        n = read(sockfd, buffer, 255); //Read Server String
        if (n < 0)
            error("ERROR reading from socket");
        printf("Server - %s\n", buffer);
        bzero(buffer, 256);
        fgets(buffer, 255, stdin);
        n = write(sockfd, buffer, strlen(buffer));
        if (n < 0)
            printf("ERROR writing to socket");
        int i = strncmp("Calc", buffer, 4);
        if (i == 0)
        {
            bzero(buffer, 256);
            n = read(sockfd, buffer, 255); //Read Server String
            if (n < 0)
                error("ERROR reading from socket");
            printf("Server - %s\n", buffer);
            scanf("%d", &num1);                //Enter No 1
            write(sockfd, &num1, sizeof(int)); //Send No 1 to Server

            bzero(buffer, 256);
            n = read(sockfd, buffer, 255); //Read Server String
            if (n < 0)
                error("ERROR reading from socket");
            printf("Server - %s\n", buffer);
            scanf("%d", &num2);                //Enter No 2
            write(sockfd, &num2, sizeof(int)); //Send No 2 to Server

            bzero(buffer, 256);
            n = read(sockfd, buffer, 255); //Read Server String
            if (n < 0)
                error("ERROR reading from socket");
            printf("Server - %s", buffer);
            scanf("%d", &choice);                //Enter choice
            write(sockfd, &choice, sizeof(int)); //Send choice to Server

            read(sockfd, &ans, sizeof(int));              //Read Answer from Server
            printf("Server - The answer is : %d\n", ans); //Get Answer from server
        }
        int J = strncmp("Bye", buffer, 3);
        if(J==0)
        {
            goto Z;
        }
    }
Z:   close(sockfd);
    return 0;
}

/*
Server side terminal:
sxmxr@samarth:~/CNAssignment/Assignment_4$ ./server 8000
Client: Calc

Client - Number 1 is : 23
Client - Number 2 is : 2
Client - Choice is : 2
Client: 

Client: Date

Client: Time

Client: Bye

sxmxr@samarth:~/CNAssignment/Assignment_4$ 

Client side Terminal:
sxmxr@samarth:~/CNAssignment/Assignment_4$ ./client 127.0.0.1 8000
Server - 
Hello there, what can I do for you?

Calc
Server - Enter Number 1
23
Server - Enter Number 2
2
Server - Enter your choice : 
1.Addition
2.Subtraction
3.Multiplication
4.Division
5.Exit
2
Server - The answer is : 21
Server - 
Hello there, what can I do for you?

Server - 
Hello there, what can I do for you?

Date
Server - Mon Oct 12 02:11:33 2020
Hello there, what can I do for you?

Time
Server - Mon Oct 12 02:11:42 2020
Hello there, what can I do for you?

Bye
sxmxr@samarth:~/CNAssignment/Assignment_4$ 

*/
