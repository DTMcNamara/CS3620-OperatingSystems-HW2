#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]){

    int argv_count = 0;
    while(argv[++argv_count] != NULL);
    for(int i =0; i<argv_count;i++){
        printf("%s\n",argv[i]);
    }
}
