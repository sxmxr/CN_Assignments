#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>

void usage(void);
int main(int argc, char **argv){
	FILE *src, *src2;
	char c;
	char x;

	src = fopen("Sample_Input","r");
	src2 = fopen("Sample_Output","w");
	while((x = getopt(argc, argv, "i:o:")) != -1){
		switch(x){
			case 'i':
			src = fopen(optarg , "r");
			break;

			case 'o':
			src2 = fopen(optarg , "w");
			break;
			exit(1);
			}
	}

	if(src == NULL){
		printf("File not Found\n");
		exit(EXIT_FAILURE);
	}
	if(src2 == NULL){
		printf("File not Found\n");
		exit(EXIT_FAILURE);
	}
	
	while((c = fgetc(src)) != EOF){
		fputc(c,src2);
	}
fclose(src);
fclose(src2);
return 0;
}

void usage(void){
printf("Usage:\n");
printf(" -i <Input>\n");
printf(" -o <Output>\n");
exit(8);
}
