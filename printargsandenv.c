#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[], char *env[]){

    int argv_count = 0;
    int env_count = 0;
    while(argv[++argv_count] != NULL);
    while(env[++env_count] != NULL);
    for(int i =0; i<argv_count;i++){
        printf("%s\n",argv[i]);
    }
    for(int i =0; i<env_count;i++){
        printf("%s\n",env[i]);
    }
}
