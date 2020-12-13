#include <stdio.h>
#include<time.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include<stdint.h>
int main(){
pid_t child_a,child_b;
child_a=fork();
if(child_a==0){
	printf("Child 1 :pid=%d\n",getpid());
	time_t t = time(NULL);
  	struct tm tm = *localtime(&t);
  	printf("Date Time (From Child 1) : %d-%02d-%02d %02d:%02d:%02d\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
}
else{
	child_b=fork();
	if(child_b==0){
		printf("Child 2 :pid=%d\n",getpid());
		system("traceroute www.google.com");
		printf("Child 2 : Work completed. \n" );
		      }
	else{
		printf("Parent :pid=%d\n",getpid());
	    }
    }
}
