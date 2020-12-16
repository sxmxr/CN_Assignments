#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>

// constant values
#define data_s 512
#define f_size 56
#define b_size 1024
#define max_try 3
#define round_trip_time 500000

// packet types
#define INIT '0'
#define DATA '1'
#define ACK '2'

// function definitions
void error (char *e);
void mult(char buffer[b_size], char *type, char filename[f_size], long *filesize, int *mode, long *seq_num, char data[data_s]);
void demult(char buffer[b_size], char *type, char filename[f_size], long *filesize, int *mode, long *seq_num, char data[data_s]);
void stop_and_wait();
void gobackn(long N);

// global variables
int port;
char *hostname;;
int mode;
char *filename;
long filesize;
char * filebuffer;
char buffer[b_size];
char recv_buffer[b_size];
int phase = 0;
long random_packet;

int mode_exist = 0;
int port_exist = 0;
int hostname_exist = 0;
int filename_exist = 0;
int test_exist = 0;
int test_case = 0;

int main(int argc, char** argv){

  FILE * file;
  size_t res;
  int i;

  // parse command line input
  for (i=0; i<argc; i++){ // mode
    if(strcmp(argv[i],"-m")==0){
      mode = atoi(argv[i+1]);
      mode_exist = 1;
    }
    else if(strcmp(argv[i],"-p")==0){ // port
      port = atoi(argv[i+1]);
      port_exist = 1;
    }
    else if(strcmp(argv[i],"-h")==0){ // hostname
      hostname = argv[i+1];
      hostname_exist = 1;
    }
    else if(strcmp(argv[i],"-f")==0){ // filename
      filename = argv[i+1];
      filename_exist = 1;
    }
    else if(strcmp(argv[i],"-t")==0){ // test case
      test_exist = 1;
      test_case = atoi(argv[i+1]);
    }
  }

  if(!mode_exist || !port_exist || !filename_exist ||!hostname_exist){
    printf("\tUsage:\n\
          [-m mode] [-p port] [-f filename] [-h hostname] <-t test>\n\
          [required] <optional>\n");
    exit(1);
  }

  // open file and get the size of the file
  file = fopen(filename,"rb");
  if(file == NULL)
    error("Cannot open file!");
  fseek (file , 0 , SEEK_END);
  filesize = ftell (file);
  rewind (file);

  // store the file content in a buffer
  filebuffer = (char*) malloc (sizeof(char)*filesize);
  if(filebuffer == NULL)
    error("Cannot create file buffer!");

  if(fread(filebuffer,1,filesize,file) != filesize)
    error("Cannot read file");

  fclose(file);

  // begin transmission
  if (mode == 1) // stop and wait
    stop_and_wait();
  else if(mode > 1) // go-back-n with windows size N=mode
    gobackn(mode);

  return 0;
}

void stop_and_wait(){
  int sock;
  struct sockaddr_in receiver_address;
  struct hostent *receiver;
  long seq_num;
  char type;
  char data[data_s];
  char FNAME[f_size];
  int tries;
  int go;
  int to_status;
  struct timeval timeout;
  int microsecond = round_trip_time;
  int sent = 0;
  int sent_data;
  socklen_t len;
  fd_set fdset;
  long total_packets;

  // create socket
  sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0)
    error("Cannot open socket!");

  // get receiver info
  receiver = gethostbyname(hostname);
  if (receiver == NULL)
    error("Receiver cannot be found!");

  // build receiver address
  memset(&receiver_address, 0, sizeof(receiver_address));
  receiver_address.sin_family = AF_INET;
  memcpy(&receiver_address.sin_addr, receiver->h_addr, sizeof(receiver_address.sin_addr));
  receiver_address.sin_port = htons(port);

  // create INIT packet
  type = INIT;
  bzero(FNAME,sizeof(FNAME));
  memcpy(FNAME,&(*filename),strlen(filename));
  seq_num = 0;
  bzero(data,data_s);
  mult(buffer,&type,FNAME,&filesize,&mode,&seq_num,data);

  // calculate total number of data packets
  total_packets = (long)ceil((double)filesize/(double)data_s);

  len = sizeof(receiver_address);

  // set timeout
  FD_ZERO (&fdset);
  FD_SET  (sock, &fdset);
  timeout.tv_sec = 0;
  timeout.tv_usec = round_trip_time;
  tries = 0;
  go = 0;

  // send INIT packet up to max_try times until an ACK is received
  while(tries < max_try){
    tries++;
    sent_data = sendto(sock, buffer, b_size, 0,(struct sockaddr *) &receiver_address, len);
    if( sent_data < 0)
      error("Cannot send package!");
    printf("-> INIT\n");
    if(test_case == 1 && phase == 0){ // test case 1
      to_status = 0;
      phase = 1;
    }
    else
      to_status = select(sock+1,&fdset,NULL,NULL,&timeout);
    if(to_status < 0) // error
      error("Select error");
    else if(to_status == 0){ // timeout
      printf("TIMEOUT-%d FOR INIT\n",tries);
      timeout.tv_usec = (tries+1) * round_trip_time; // try again with higher timeout
    }else{ // success
      go = 1;
      break;
    }
  }

  // terminate connection if there is no progress after max_try tries
  if(!go)
    error("Sender time out...\n");

  // receive packet from receiver
  if(recvfrom(sock, recv_buffer, b_size, 0,(struct sockaddr *) &receiver_address, &len) < 0)
    error("No response!\n");

  // get the packet content
  demult(recv_buffer,&type,FNAME,&filesize,&mode,&seq_num,data);

  // sender only accepts packets of type ACK
  if(type != ACK)
    error("Unknown response!\n");

  printf("<- ACK INIT\n");

  phase = 0;
  srand(time(NULL));
  random_packet = rand()%total_packets+1;

  // send the file
  while(sent < filesize){
    // divide the file into chunks
    type = DATA;
    seq_num++;
    bzero(data,data_s);
    memcpy(data,filebuffer+sent,data_s);

    // create the DATA packet
    mult(buffer,&type,FNAME,&filesize,&mode,&seq_num,data);

    //set timeout
    timeout.tv_sec = 0;
    timeout.tv_usec = round_trip_time;
    tries = 0;
    go = 0;

    // send DATA packet up to max_try times until an ACK is received
    while(tries < max_try){
      FD_ZERO (&fdset);
      FD_SET  (sock, &fdset);
      tries++;
      if(test_case == 2 && phase == 0 && seq_num == random_packet){
        phase = 1;
        receiver_address.sin_port = htons(port-1);
        sent_data = sendto(sock, buffer, b_size, 0,(struct sockaddr *) &receiver_address, len);
        if( sent_data < 0)
          error("Cannot send package!");
        printf("-> PACKET %ld\n",seq_num);
      }
      else if(test_case == 3 && seq_num == random_packet){
        receiver_address.sin_port = htons(port-1);
        sent_data = sendto(sock, buffer, b_size, 0,(struct sockaddr *) &receiver_address, len);
        if( sent_data < 0)
          error("Cannot send package!");
        printf("-> PACKET %ld\n",seq_num);
      }else{
        receiver_address.sin_port = htons(port);
        sent_data = sendto(sock, buffer, b_size, 0,(struct sockaddr *) &receiver_address, len);
        if( sent_data < 0)
          error("Cannot send package!");
        printf("-> PACKET %ld\n",seq_num);
      }
      to_status = select(sock+1,&fdset,NULL,NULL,&timeout);
      if(to_status < 0) // error
        error("Select error");
      else if(to_status == 0){ // timeout
        printf("TIMEOUT-%d FOR PACKET %ld\n",tries,seq_num);
        timeout.tv_usec = (tries+1) * round_trip_time; // try again with higher timeout
      }else{ // success
        go = 1;
        break;
      }
    }

    if(go==1){ // if we receive a packet
      sent = sent + sizeof(data); // increment data chunk pointer
      if(recvfrom(sock, recv_buffer, b_size, 0,(struct sockaddr *) &receiver_address, &len) < 0)
        error("No response!");

      // get packet contents
      demult(recv_buffer,&type,FNAME,&filesize,&mode,&seq_num,data);

      if(type != ACK)
        error("Unknown response!");

      printf("<- ACK %ld\n",seq_num);
    }
    else // terminate connection if there is no progress after max_try tries
      error("Sender timeout...\n");
  }
  close(sock);
  printf("Transmission complete\n");
}

void gobackn(long N){
  int sock;
  struct sockaddr_in receiver_address;
  struct hostent *receiver;
  long req_num;
  long seq_num;
  long base = 1;
  long max = N;
  char type;
  char data[data_s];
  char FNAME[f_size];
  int tries;
  int sent = 0;
  long total_packets;
  socklen_t len;
  struct timeval timeout;
  fd_set fdset;
  int to_status;
  int go;
  int sent_data;

  // create socket
  sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0)
    error("Cannot open socket!");

  // get receiver info
  receiver = gethostbyname(hostname);
  if (receiver == NULL)
    error("Receiver cannot be found!");

  // build receiver address
  memset(&receiver_address, 0, sizeof(receiver_address));
  receiver_address.sin_family = AF_INET;
  memcpy(&receiver_address.sin_addr, receiver->h_addr, sizeof(receiver_address.sin_addr));
  receiver_address.sin_port = htons(port);

  // create INIT packet
  type = INIT;
  memcpy(FNAME,&(*filename),strlen(filename));
  seq_num = 1;
  bzero(data,data_s);
  mult(buffer,&type,FNAME,&filesize,&mode,&seq_num,data);

  // calculate total number of data packets
  total_packets = (long)ceil((double)filesize/(double)data_s);

  len = sizeof(receiver_address);

  // set timeout
  FD_ZERO (&fdset);
  FD_SET  (sock, &fdset);
  timeout.tv_sec = 0;
  timeout.tv_usec = round_trip_time;
  tries = 0;
  go = 0;

  // send INIT packet up to max_try times until an ACK is received
  while(tries < max_try){
    tries++;
    sent_data = sendto(sock, buffer, b_size, 0,(struct sockaddr *) &receiver_address, len);
    if( sent_data < 0)
      error("Cannot send package!");
    printf("-> INIT\n");
    to_status = select(sock+1,&fdset,NULL,NULL,&timeout);
    if(to_status < 0) // error
      error("Select error");
    else if(to_status == 0){ // timeout
      printf("TIMEOUT-%d FOR INIT\n",tries);
      timeout.tv_usec = (tries+1) * round_trip_time; // try again with higher timeout
    }else{ // success
      go = 1;
      break;
    }
  }

  // terminate connection if there is no progress after max_try tries
  if(!go)
    error("Sender time out...\n");

  // receive packet
  if(recvfrom(sock, recv_buffer, b_size, 0,(struct sockaddr *) &receiver_address, &len) < 0)
    error("No response!");

  // get packet contents
  demult(recv_buffer,&type,FNAME,&filesize,&mode,&req_num,data);

  if(type != ACK || req_num != 1)
    error("Unknown response!");

  printf("<- ACK INIT\n");

  // set timeout
  FD_ZERO (&fdset);
  FD_SET  (sock, &fdset);
  timeout.tv_sec = 0;
  timeout.tv_usec = round_trip_time;
  tries = 0;

  while(1){ // main loop
    // every window is sent at most max_try times
    if(tries >= max_try)
      error("Connection timeout!");

    // send the window
    max = base+N-1;
    while(seq_num <= total_packets && seq_num <= max){
      memcpy(data,filebuffer+(seq_num-1)*data_s,data_s);
      type = DATA;
      mult(buffer,&type,FNAME,&filesize,&mode,&seq_num,data);

      if(sendto(sock, buffer, b_size, 0,(struct sockaddr *) &receiver_address, len) < 0)
          error("Cannot send package!");

      printf("-> PACKET %ld\n",seq_num);
      seq_num++;
    }

    to_status = select(sock+1,&fdset,NULL,NULL,&timeout);

    if(to_status < 0) // error
      error("Select error");
    else if(to_status == 0){ // timeout
      tries++;
      timeout.tv_usec = (tries+1) * round_trip_time; // try again with higher timeout
      seq_num = base; // send the window again from scratch
      printf("TIMEOUT-%d\n", tries);
    }else{ // we receive a packet

      if(recvfrom(sock, recv_buffer, b_size, 0,(struct sockaddr *) &receiver_address, &len) < 0)
        error("No response!");

      // get packet content
      demult(recv_buffer,&type,FNAME,&filesize,&mode,&req_num,data);
      printf("<- REQUEST %ld\n",req_num);

      // all packets delivered so terminate the connection
      if(req_num == total_packets + 1){
        printf("Transmission complete\n");
        break;
      }

      // slide the window
      if(req_num > base)
        base = req_num;

      // reset tries and timeout value
      tries = 0;
      timeout.tv_usec = round_trip_time;
    }

  }
  close(sock);
}

void mult(char buffer[b_size], char *type, char filename[f_size], long *filesize, int *mode, long *seq_num, char data[data_s]){
  // writes the packet fields into the packet and does conversions if necessary
  long fls = htonl(*filesize);
  int mod = htons(*mode);
  long sn = htonl(*seq_num);
  int i=0;
  memcpy(buffer,type,sizeof(char));
  i = i+sizeof(char);
  memcpy(buffer+i,filename,f_size);
  i = i+f_size;
  memcpy(buffer+i,&fls,sizeof(long));
  i = i+sizeof(long);
  memcpy(buffer+i,&mod,sizeof(int));
  i = i+sizeof(int);
  memcpy(buffer+i,&sn,sizeof(long));
  i = i+sizeof(long);
  memcpy(buffer+i,data,data_s);

}

void demult(char buffer[b_size], char *type, char filename[f_size], long *filesize, int *mode, long *seq_num, char data[data_s]){
  // reads the packet fields from the packet and does conversions if necessary
  int i=0;
  memcpy(type,buffer,sizeof(char));
  i = i+sizeof(char);
  memcpy(filename,buffer+i,f_size);
  i = i+f_size;
  memcpy(filesize,buffer+i,sizeof(long));
  i = i+sizeof(long);
  memcpy(mode,buffer+i,sizeof(int));
  i = i+sizeof(int);
  memcpy(seq_num,buffer+i,sizeof(long));
  i = i+sizeof(long);
  memcpy(data,buffer+i,data_s);
  *filesize = ntohl(*filesize);
  *mode = ntohs(*mode);
  *seq_num = ntohl(*seq_num);

}

void error (char *e){
  printf("%s\n",e);
  exit(1);
}

//Output
// ./server -p 1515 -h 127.0.0.1 -f test.txt -m 1
// -> INIT
// <- ACK INIT
// -> PACKET 1
// <- ACK 1
// -> PACKET 2
// <- ACK 2
// -> PACKET 3
// <- ACK 3
// -> PACKET 4
// <- ACK 4
// -> PACKET 5
// <- ACK 5
// -> PACKET 6
// <- ACK 6
// -> PACKET 7
// <- ACK 7
// -> PACKET 8
// <- ACK 8
// -> PACKET 9
// <- ACK 9
// -> PACKET 10
// <- ACK 10
// -> PACKET 11
// <- ACK 11
// -> PACKET 12
// <- ACK 12
// -> PACKET 13
// <- ACK 13
// -> PACKET 14
// <- ACK 14
// -> PACKET 15
// <- ACK 15
// -> PACKET 16
// <- ACK 16
// -> PACKET 17
// <- ACK 17
// -> PACKET 18
// <- ACK 18
// -> PACKET 19
// <- ACK 19
// -> PACKET 20
// <- ACK 20
// -> PACKET 21
// <- ACK 21
// -> PACKET 22
// <- ACK 22
// -> PACKET 23
// <- ACK 23
// -> PACKET 24
// <- ACK 24
// -> PACKET 25
// <- ACK 25
// -> PACKET 26
// <- ACK 26
// -> PACKET 27
// <- ACK 27
// -> PACKET 28
// <- ACK 28
// -> PACKET 29
// <- ACK 29
// -> PACKET 30
// <- ACK 30
// -> PACKET 31
// <- ACK 31
// -> PACKET 32
// <- ACK 32
// -> PACKET 33
// <- ACK 33
// -> PACKET 34
// <- ACK 34
// -> PACKET 35
// <- ACK 35
// -> PACKET 36
// <- ACK 36
// Transmission complete
