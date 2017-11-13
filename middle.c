#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main(){
	char buf[256];
	char bufm[256] = "middle";

	fgets(buf, 256, stdin);

	buf[strlen(buf) - 1] = 0;
	puts(strcat(buf, bufm));

	return 0;
}
