/*Name : Samarth Shah
AU1841145
Problem Statement: To implement Client-Server Chat application and Calculator.
*/

/*
	Server Side
*/
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>
#include <time.h>

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    char buffer[256];
    struct sockaddr_in serv_addr, cli_addr;
    int n;
    if (argc < 2)
    {
        fprintf(stderr, "ERROR, no port provided\n");
        exit(1);
    }
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    bzero((char *)&serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *)&serv_addr,
             sizeof(serv_addr)) < 0)
        error("ERROR on binding");
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd,
                       (struct sockaddr *)&cli_addr,
                       &clilen);
    if (newsockfd < 0)
        error("ERROR on accept");

    int num1, num2, ans, i1, i2, i3, i4, choice;

    while (1)
    {
    P:    bzero(buffer, 256);
        n = write(newsockfd, "\nHello there, what can I do for you?", strlen("\nHello there, what can I do for you?"));
        if (n < 0)
            error("ERROR writing to socket");
        bzero(buffer, 256);
        n = read(newsockfd, buffer, 255);
        if (n < 0)
            error("ERROR reading from socket");
        printf("Client: %s\n", buffer);
        i1 = strncmp("Time", buffer, 4);
        if (i1 == 0)
        {
            time_t mytime = time(NULL);
            char *time_str = ctime(&mytime);
            time_str[strlen(time_str) - 1] = '\0';
            n = write(newsockfd, time_str, strlen(time_str)); //Printing Time
            if (n < 0)
                error("ERROR writing to socket");
        }
        i2 = strncmp("Date", buffer, 4);
        if (i2 == 0)
        {
            time_t t = time(NULL);
            struct tm *tm = localtime(&t);
            char s[64];
            assert(strftime(s, sizeof(s), "%c", tm));
            n = write(newsockfd, s, strlen(s)); //Printing Date
            if (n < 0)
                error("ERROR writing to socket");
        }
        int i3 = strncmp("Calc", buffer, 4);
        if (i3 == 0)
        {
        S:
            n = write(newsockfd, "Enter Number 1 : ", strlen("Enter Number 1")); //Ask for Number 1
            if (n < 0)
                error("ERROR writing to socket");
            read(newsockfd, &num1, sizeof(int)); //Read No 1
            printf("Client - Number 1 is : %d\n", num1);

            n = write(newsockfd, "Enter Number 2 : ", strlen("Enter Number 2")); //Ask for Number 2
            if (n < 0)
                error("ERROR writing to socket");
            read(newsockfd, &num2, sizeof(int)); //Read Number 2
            printf("Client - Number 2 is : %d\n", num2);

            n = write(newsockfd, "Enter your choice : \n1.Addition\n2.Subtraction\n3.Multiplication\n4.Division\n5.Exit\n", strlen("Enter your choice : \n1.Addition\n2.Subtraction\n3.Multiplication\n4.Division\n5.Exit\n")); //Ask for choice
            if (n < 0)
                error("ERROR writing to socket");
            read(newsockfd, &choice, sizeof(int)); //Read choice
            printf("Client - Choice is : %d\n", choice);

            switch (choice)
            {
            case 1:
                ans = num1 + num2;
                break;
            case 2:
                ans = num1 - num2;
                break;
            case 3:
                ans = num1 * num2;
                break;
            case 4:
                ans = num1 / num2;
                break;
            case 5:
                goto Q;
                break;
            }

            write(newsockfd, &ans, sizeof(int));
            if (choice != 5)
                goto P;
        }
        int i4 = strncmp("Bye", buffer, 3);
        if (i4 == 0)
            break;
    }
Q: close(sockfd);
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
