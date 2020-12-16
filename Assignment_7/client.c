#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>

#define data_S 512
#define b_size 1024
#define fname_size 56
#define r_timeout 5

#define INIT '0'
#define DATA '1'
#define ACK '2'

void error (char *e);
void mult(char buffer[b_size], char *type, char filename[fname_size], long *filesize, int *mode, long *seq_num, char data[data_S]);
void demult(char buffer[b_size], char *type, char filename[fname_size], long *filesize, int *mode, long *seq_num, char data[data_S]);
void merge(const int num_pack, const char* filename);

const char *prefix = "packet";

int main(int argc, char **argv) {
  int sock;
  int port;
  int optval = 1;
  char *hostname;
  struct in_addr **addr_list;
  struct sockaddr_in sender_address;
  struct sockaddr_in receiver_address;
  struct hostent *sender;
  long seq_num;
  char type;
  char filename[fname_size];
  char packetname[fname_size];
  long filesize;
  int mode;
  int sender_mode;
  char data[data_S];
  char buffer[b_size];
  char recv_buffer[b_size];
  socklen_t len;
  int received = 0;
  FILE *file;
  int recv_seq_num;
  long total_packets;
  long req_num = 1;
  struct timeval timeout;
  fd_set fdset;
  int to_status;
  int mode_exist = 0;
  int port_exist = 0;
  int hostname_exist = 0;
  int test_exist = 0;
  int test_case = 0;
  int phase = 0;
  int i;

  // parse command line input
  for (i=0; i<argc; i++){
    if(strcmp(argv[i],"-m")==0){
      mode = atoi(argv[i+1]);
      mode_exist = 1;
    }
    else if(strcmp(argv[i],"-p")==0){
      port = atoi(argv[i+1]);
      port_exist = 1;
    }
    else if(strcmp(argv[i],"-h")==0){
      hostname = argv[i+1];
      hostname_exist = 1;
    }
    else if(strcmp(argv[i],"-t")==0){
      test_exist = 1;
      test_case = atoi(argv[i+1]);
    }
  }

  if(!mode_exist || !port_exist){
    printf("\tUsage:\n\
          [-m mode] [-p port] <-h hostname> <-t test>\n\
          [required] <optional>\n");
    exit(1);
  }

  // create socket
  sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0)
    error("Cannot open socket!");

  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));

  // get receiver info
  if(hostname_exist){
    sender = gethostbyname(hostname);
    if (sender == NULL)
      error("Sender cannot be found!");
  }

  // build receiver address
  memset(&receiver_address, 0, sizeof(receiver_address));
  receiver_address.sin_family = AF_INET;
  receiver_address.sin_addr.s_addr = INADDR_ANY;
  receiver_address.sin_port = htons(port);

  // bind to a port
  if (bind(sock, (struct sockaddr *) &receiver_address, sizeof(receiver_address)) < 0)
    error("ERROR on binding");

  // set timeout
  FD_ZERO (&fdset);
  FD_SET  (sock, &fdset);
  timeout.tv_sec = 2*r_timeout;
  timeout.tv_usec = 0;

  to_status = select(sock+1,&fdset,NULL,NULL,&timeout);
  if(to_status < 0) // error
    error("Select error");
  else if(to_status == 0){ // receiver was idle
    error("Receiver time out...");
  }

  len = sizeof(sender_address);

  // receive packet
  if(recvfrom(sock, recv_buffer, b_size, 0,(struct sockaddr *) &sender_address, &len) < 0)
    error("No init\n");

  // get packet contents
  demult(recv_buffer,&type,filename,&filesize,&sender_mode,&seq_num,data);

  // check for mode mismatch
  if(mode != sender_mode)
    error("Incompatible modes!");

  // check if the sender is the designated host given from the command line
  if(hostname_exist){
    sender = gethostbyaddr((const char *)&sender_address.sin_addr.s_addr, sizeof(sender_address.sin_addr.s_addr), AF_INET);
    if(strcmp(sender->h_name,hostname) != 0)
      error("Unexpected sender");
  }

  // check packet type
  if (type != INIT)
    error("No INIT message!");

  // calculate total number of data packets
  total_packets = (long)ceil((double)filesize/(double)data_S);

  printf("<- INIT\n");

  // create and send ACK message for INIT
  type = ACK;
  if(mode == 1)
    mult(buffer,&type,filename,&filesize,&sender_mode,&seq_num,data);
  else
    mult(buffer,&type,filename,&filesize,&sender_mode,&req_num,data);
  if(sendto(sock, buffer, b_size, 0,(struct sockaddr *) &sender_address, len) < 0)
    error("Cannot send package!");

  printf("-> ACK INIT\n");

  // main loop
  if (mode == 1){ // stop and wait
    recv_seq_num = 1;
    while(received < filesize){ // when there is still packets to receive
      to_status = select(sock+1,&fdset,NULL,NULL,&timeout);
      if(to_status < 0) // error
        error("Select error");
      else if(to_status == 0){ // channel was idle for too long
        error("Receiver time out...");
      }

      // receive packet
      bzero(data,data_S);
      int x = recvfrom(sock, recv_buffer, b_size, 0,(struct sockaddr *) &sender_address, &len);
      if( x < 0)
        error("Cannot receive packet");

      // get packet contents
      demult(recv_buffer,&type,filename,&filesize,&sender_mode,&seq_num,data);

      // if the received packet is the expected packet
      if(seq_num == recv_seq_num){
        recv_seq_num++;
        received = received + sizeof(data);
        printf("<- PACKET %ld\n",seq_num);

        // write packet to disk
        char tmp[data_S+1];
        memcpy(tmp,data,data_S);
        tmp[data_S] = '\0';
        strcpy(packetname,prefix);
        sprintf(packetname+strlen(prefix),"%ld",seq_num);
        file = fopen(packetname,"wb");
        fwrite(tmp,1,sizeof(tmp),file);
        fclose(file);

        // create and send ACK for the received DATA packet
        type = ACK;
        mult(buffer,&type,filename,&filesize,&sender_mode,&seq_num,data);
        if(sendto(sock, buffer, b_size, 0,(struct sockaddr *) &sender_address, len) < 0)
          error("Cannot send package!");
        printf("-> ACK %ld\n",seq_num);
      }
    }
    printf("Transmission complete\n");
    close(sock);

    // combine data packets to re-create the original file
    pid_t pid = getpid();
    bzero(packetname,fname_size);
    strcpy(packetname,filename);
    sprintf(packetname+strlen(filename),"%d",pid);
    merge(seq_num,packetname);
  }
  else if(mode > 1){ // go-back-n with windows size N=mode
    while(req_num <= total_packets){ // when there is still packets to receive
      FD_ZERO (&fdset);
      FD_SET  (sock, &fdset);
      to_status = select(sock+1,&fdset,NULL,NULL,&timeout);
      if(to_status < 0) // error
        error("Select error");
      else if(to_status == 0){ // channel was idle for too long
        error("Receiver time out...");
      }

      // receive packet
      bzero(data,data_S);
      int x = recvfrom(sock, recv_buffer, b_size, 0,(struct sockaddr *) &sender_address, &len);
      if( x < 0)
        error("Cannot receive packet");

      // get packet content
      demult(recv_buffer,&type,filename,&filesize,&sender_mode,&seq_num,data);

      if(seq_num == req_num){ // expected packet
        req_num++;
        received = received + sizeof(data);
        printf("<- PACKET %ld\n",seq_num);

        // write packet to disk
        char tmp[data_S+1];
        memcpy(tmp,data,data_S);
        tmp[data_S] = '\0';
        strcpy(packetname,prefix);
        sprintf(packetname+strlen(prefix),"%ld",seq_num);
        file = fopen(packetname,"wb");
        fwrite(tmp,1,strlen(tmp),file);
        fclose(file);
      }
      // send ACK for the unreceived packet with smallest seq num
      if(!(test_case == 4 && req_num != total_packets + 1 && req_num % 2 == 1)){ // test case 4
        type = ACK;
        mult(buffer,&type,filename,&filesize,&sender_mode,&req_num,data);
        if(sendto(sock, buffer, b_size, 0,(struct sockaddr *) &sender_address, len) < 0)
          error("Cannot send package!");
        printf("-> REQUEST %ld\n",req_num);
      }
    }
    printf("Transmission complete\n");
    close(sock);

    // combine data packets to re-create the original file
    pid_t pid = getpid();
    bzero(packetname,fname_size);
    strcpy(packetname,filename);
    sprintf(packetname+strlen(filename),"%d",pid);
    merge(seq_num,packetname);
  }
}

void mult(char buffer[b_size], char *type, char filename[fname_size], long *filesize, int *mode, long *seq_num, char data[data_S]){
  // writes the packet fields into the packet and does conversions if necessary
  long fls = htonl(*filesize);
  int mod = htons(*mode);
  long sn = htonl(*seq_num);
  int i=0;
  memcpy(buffer,type,sizeof(char));
  i = i+sizeof(char);
  memcpy(buffer+i,&(*filename),fname_size);
  i = i+fname_size;
  memcpy(buffer+i,&fls,sizeof(long));
  i = i+sizeof(long);
  memcpy(buffer+i,&mod,sizeof(int));
  i = i+sizeof(int);
  memcpy(buffer+i,&sn,sizeof(long));
  i = i+sizeof(long);
  memcpy(buffer+i,data,data_S);

}

void demult(char buffer[b_size], char *type, char filename[fname_size], long *filesize, int *mode, long *seq_num, char data[data_S]){
  // reads the packet fields from the packet and does conversions if necessary
  int i=0;
  memcpy(type,buffer,sizeof(char));
  i = i+sizeof(char);
  memcpy(&(*filename),buffer+i,fname_size);
  i = i+fname_size;
  memcpy(filesize,buffer+i,sizeof(long));
  i = i+sizeof(long);
  memcpy(mode,buffer+i,sizeof(int));
  i = i+sizeof(int);
  memcpy(seq_num,buffer+i,sizeof(long));
  i = i+sizeof(long);
  memcpy(data,buffer+i,data_S);
  *filesize = ntohl(*filesize);
  *mode = ntohs(*mode);
  *seq_num = ntohl(*seq_num);

}

void merge(const int num_pack, const char* filename){
  // combine small packets into a large file
  char buffer[data_S];
  char packetname[fname_size];
  int i = 1;
  long size;
  FILE* fr;
  FILE* fw = fopen(filename, "wb");
  if(fw == NULL)
    error("Cannot open file");

  while(i<=num_pack){

    bzero(packetname,fname_size);
    strcpy(packetname,prefix);
    sprintf(packetname+strlen(prefix),"%d",i);

    fr = fopen(packetname, "rb");
    if(fr == NULL)
      error("Cannot open file");

    char curr_char;
    while ((curr_char = getc(fr)) != EOF)
      fwrite(&curr_char,1,sizeof(curr_char),fw);

    fclose(fr);
    remove(packetname);
    i++;
  }
  fclose(fw);
}

void error (char *e){
  // print error message and die
  printf("%s\n",e);
  exit(1);
}

//Output :
// ./client -p 1515 -m 1
// <- INIT
// -> ACK INIT
// <- PACKET 1
// -> ACK 1
// <- PACKET 2
// -> ACK 2
// <- PACKET 3
// -> ACK 3
// <- PACKET 4
// -> ACK 4
// <- PACKET 5
// -> ACK 5
// <- PACKET 6
// -> ACK 6
// <- PACKET 7
// -> ACK 7
// <- PACKET 8
// -> ACK 8
// <- PACKET 9
// -> ACK 9
// <- PACKET 10
// -> ACK 10
// <- PACKET 11
// -> ACK 11
// <- PACKET 12
// -> ACK 12
// <- PACKET 13
// -> ACK 13
// <- PACKET 14
// -> ACK 14
// <- PACKET 15
// -> ACK 15
// <- PACKET 16
// -> ACK 16
// <- PACKET 17
// -> ACK 17
// <- PACKET 18
// -> ACK 18
// <- PACKET 19
// -> ACK 19
// <- PACKET 20
// -> ACK 20
// <- PACKET 21
// -> ACK 21
// <- PACKET 22
// -> ACK 22
// <- PACKET 23
// -> ACK 23
// <- PACKET 24
// -> ACK 24
// <- PACKET 25
// -> ACK 25
// <- PACKET 26
// -> ACK 26
// <- PACKET 27
// -> ACK 27
// <- PACKET 28
// -> ACK 28
// <- PACKET 29
// -> ACK 29
// <- PACKET 30
// -> ACK 30
// <- PACKET 31
// -> ACK 31
// <- PACKET 32
// -> ACK 32
// <- PACKET 33
// -> ACK 33
// <- PACKET 34
// -> ACK 34
// <- PACKET 35
// -> ACK 35
// <- PACKET 36
// -> ACK 36
// Transmission complete