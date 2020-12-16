
/*
Samarth Shah
AU1841145
*/

// server code for UDP socket programming 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <unistd.h> 
#include <netdb.h>

#define IP_PROTOCOL 0  
#define NET_BUF_SIZE 512 
#define cipherKey 'S' 
#define sendrecvflag 0 
#define nofile "File Not Found!" 

// function to clear buffer 
void clearBuf(char* b) 
{ 
	int i; 
	for (i = 0; i < NET_BUF_SIZE; i++) 
		b[i] = '\0'; 
} 

// function to encrypt 
char Cipher(char ch) 
{ 
	return ch ^ cipherKey; 
} 

// function sending file 
int sendFile(FILE* fp, char* buf, int s) 
{ 
	int i, len; 
	if (fp == NULL) { 
		strcpy(buf, nofile); 
		len = strlen(nofile); 
		buf[len] = EOF; 
		for (i = 0; i <= len; i++) 
			buf[i] = Cipher(buf[i]); 
		return 1; 
	} 

	char ch, ch2; 
	for (i = 0; i < s; i++) { 
		ch = fgetc(fp); 
		ch2 = Cipher(ch); 
		buf[i] = ch2; 
		if (ch == EOF) 
			return 1; 
	} 
	return 0; 
} 

// driver code 
int main(int argc, char *argv[]) 
{ 
	if (argc < 2)
    {
        fprintf(stderr, "ERROR, no port provided\n");
        exit(1);
    }
	int PORT_NO = atoi(argv[1]);
	int sockfd, nBytes,len; 
	struct sockaddr_in addr_con; 
	int addrlen = sizeof(addr_con); 
	addr_con.sin_family = AF_INET; 
	addr_con.sin_port = htons(PORT_NO); 
	addr_con.sin_addr.s_addr = INADDR_ANY; 
	char net_buf[NET_BUF_SIZE]; 
	FILE* fp=NULL; 

	// socket() 
	sockfd = socket(AF_INET, SOCK_DGRAM, IP_PROTOCOL); 

	if (sockfd < 0) 
		printf("\nfile descriptor not received!!\n"); 
	else
		printf("\nfile descriptor %d received\n", sockfd); 

	// bind() 
	if (bind(sockfd, (struct sockaddr*)&addr_con, sizeof(addr_con)) == 0) 
		printf("\nSuccessfully binded!\n"); 
	else
		printf("\nBinding Failed!\n"); 

	while (1) { 
		printf("\nWaiting for file name...\n"); 

		// receive file name 
		clearBuf(net_buf); 

		nBytes = recvfrom(sockfd, net_buf, 
						NET_BUF_SIZE, sendrecvflag, 
						(struct sockaddr*)&addr_con, &addrlen); 

		fp = fopen(net_buf, "r"); 
		printf("\nFile Name Received: %s\n", net_buf); 
		if (fp == NULL) 
			printf("\nFile open failed!\n"); 
		else
			printf("\nFile Successfully opened!\n"); 

		int tot_frame,i;
		fseek(fp, 0, SEEK_END);
		long fsize = ftell(fp);
		printf("\nFile Size : %ld\n",fsize); 
		long p=(fsize % 512);
		if ((fsize % NET_BUF_SIZE) != 0)
		{
			tot_frame = (fsize / NET_BUF_SIZE) + 1;
		}		
		else
		{
			tot_frame = (fsize / NET_BUF_SIZE);
		}
		printf("last packets are :%ld\n\n", p); 
		printf("Total number of packets are :%d\n\n", tot_frame);
				    
		fseek(fp, 0, SEEK_SET);
		         
		if(sendto(sockfd,&(tot_frame),sizeof(tot_frame),0,(struct sockaddr*)&addr_con, addrlen)<0)
		    printf("Error in sending frame no:\n");
			    
		if(tot_frame==0 || tot_frame==1)
		{
		      char *string = malloc(fsize + 1);
			      fread(string,1,fsize+1,fp);		          
			      fseek(fp, 0, SEEK_SET);
			      int x=sendto(sockfd,string,fsize+1,0,(struct sockaddr*)&addr_con, addrlen); 
		      printf("%d\n",x);
		}
		else
		{ 
		      for(i=1;i<=tot_frame;i++)
		      {
			              char *string = malloc(NET_BUF_SIZE);
			              len=fread(string,1,NET_BUF_SIZE,fp);	           
			              fseek(fp, 0, SEEK_CUR);
			              int x=sendto(sockfd,string,len,0,(struct sockaddr*)&addr_con,addrlen); 
			              printf("Sent Packet %d\n",i);
		      }
		      
		}
		if (fp != NULL) 
			fclose(fp); 	
		return 0;
} 
} 

//Output:
/*sxmxr@samarth:~/CNAssignment/Assignment_5$ gcc -o server Server.c
sxmxr@samarth:~/CNAssignment/Assignment_5$ ./server 8000

file descriptor 3 received

Successfully binded!

Waiting for file name...

File Name Received: Dummy.txt

File Successfully opened!

File Size : 1136
last packets are :112

Total number of packets are :3

Sent Packet 1
Sent Packet 2
Sent Packet 3
sxmxr@samarth:~/CNAssignment/Assignment_5$*/