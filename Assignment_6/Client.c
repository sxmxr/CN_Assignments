
/*
Samarth Shah
AU1841145
*/

// client code for UDP socket programming 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <unistd.h> 
#include <netdb.h>
#include <ctype.h>

#define IP_PROTOCOL 0 
#define NET_BUF_SIZE 512
#define cipherKey 'S' 
#define sendrecvflag 0 

// function to clear buffer 
void clearBuf(char* b) 
{ 
	int i; 
	for (i = 0; i < NET_BUF_SIZE; i++) 
		b[i] = '\0'; 
} 

// function for decryption 
char Cipher(char ch) 
{ 
	return ch ^ cipherKey; 
} 

// function to receive file 
int recvFile(char* buf, int s) 
{ 
	int i; 
	char ch; 
	for (i = 0; i < s; i++) { 
		ch = buf[i]; 
		ch = Cipher(ch); 
		if (ch == EOF) 
			return 1; 
		else
			printf("%c", ch); 
	} 
	return 0; 
} 

// driver code 
int main(int argc, char *argv[]) 
{ 
	
	char IP_ADDRESS[20]; 
	int sockfd, nBytes,PORT_NO; 
	
	FILE* fp; 
	if(argc >= 2) {
		strcpy(IP_ADDRESS, argv[1]);
		PORT_NO = atoi(argv[2]);
	}
	struct sockaddr_in addr_con; 
	int addrlen = sizeof(addr_con); 
	addr_con.sin_family = AF_INET; 
	addr_con.sin_port = htons(PORT_NO); 
	addr_con.sin_addr.s_addr = inet_addr(IP_ADDRESS);
	char net_buf[NET_BUF_SIZE]; 
	
	// socket() 
	sockfd = socket(AF_INET, SOCK_DGRAM, 
					IP_PROTOCOL); 
	
	if (sockfd < 0) 
		printf("\nfile descriptor not received!!\n"); 
	else
		printf("\nfile descriptor %d received\n", sockfd); 

	while (1) { 
		printf("\nPlease enter file name to receive:\n"); 
		scanf("%s", net_buf); 
		sendto(sockfd, net_buf, NET_BUF_SIZE, 
			sendrecvflag, (struct sockaddr*)&addr_con, 
			addrlen); 

		printf("\n---------Data Received---------\n"); 

		while (1) { 
			// receive 
			clearBuf(net_buf); 
			nBytes = recvfrom(sockfd, net_buf, NET_BUF_SIZE, 
							sendrecvflag, (struct sockaddr*)&addr_con, 
							&addrlen); 

			// process 
			if (recvFile(net_buf, NET_BUF_SIZE)) { 
				break; 
			printf("\n-------------------------------\n"); 
			return 0; 
			} 
		} 
		
	} 
	
} 

//Output
/*
sxmxr@samarth:~/CNAssignment/Assignment_5$ gcc -o client Client.c
sxmxr@samarth:~/CNAssignment/Assignment_5$ ./client 127.0.0.1 8000

file descriptor 3 received

Please enter file name to receive:
Dummy.txt

---------Data Received---------
*/