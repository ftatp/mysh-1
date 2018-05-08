#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(){
	char buff[1024];
	gets(buff);
	printf("%s\n", strcat(buff, "right!!!!"));
}
