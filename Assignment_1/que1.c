#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>

void usage(void);
int main(int argc, char **argv){
int num = 2;
char disp = '*';
char c;
while ((c = getopt(argc,argv, "n:d:")) != -1)
{
switch(c){
case 'n':
num = atoi(optarg);
break;
case 'd':
disp = optarg[0];
break;
exit(1);
	}
}

int x = 0;
for(x = 0;x<num;x++){
	printf("%c\n", disp);
	}
return 0;
}
void usage(void){
printf("Usage:\n");
printf(" -n <repetitions>\n");
printf(" -d <char to display>\n");
exit(8);
}
