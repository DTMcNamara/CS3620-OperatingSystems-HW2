#define _GNU_SOURCE //this is needed to be able to use execvpe 
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <fcntl.h>

typedef struct {
  char* binary_path;
  char* stdin;
  char* stdout;
  char* stderr;
  char* arguments;
  char* extra_environment;
  short use_path;
  short copy_environment;
  short niceness;
  short wait;
  unsigned int timeout;
} command;

//function prototypes
void print_parsed_command(command);
short parse_command(command*, char*);
int hex2dec(int value);

//global variables here

short getlinedup(FILE* fp, char** value){
  char* line = NULL;
  size_t n = 0;
  //get one line
  int ret = getline(&line, &n, fp);

  if(ret == -1){
    //the file ended
    return 0;
  }
  //remove \n at the end
  line[strcspn(line, "\n")] = '\0';
  //duplicate the string and set value
  *value = strdup(line);
  free(line);

  return 1;
}

//parse a command_file and set a corresponding command data structure
short parse_command(command* parsed_command, char* cmdfile){
  FILE* fp = fopen(cmdfile, "rb");
  if(!fp){
    //the file does not exist
    return 0;
  }

  char* value;
  short ret;
  int intvalue;

  ret = getlinedup(fp,&value);
  if(!ret){
    fclose(fp); return 0;
  }
  parsed_command->binary_path = value;

  ret = getlinedup(fp,&value);
  if(!ret){
    fclose(fp); return 0;
  }
  parsed_command->stdin = value;

  ret = getlinedup(fp,&value);
  if(!ret){
    fclose(fp); return 0;
  }
  parsed_command->stdout = value;

  ret = getlinedup(fp,&value);
  if(!ret){
    fclose(fp); return 0;
  }
  parsed_command->stderr = value;

  ret = getlinedup(fp,&value);
  if(!ret){
    fclose(fp); return 0;
  }
  parsed_command->arguments = value;

  ret = getlinedup(fp,&value);
  if(!ret){
    fclose(fp); return 0;
  }
  parsed_command->extra_environment = value;

  ret = getlinedup(fp,&value);
  if(!ret){
    fclose(fp); return 0;
  }
  intvalue = atoi(value);
  if(intvalue != 0 && intvalue != 1){
    fclose(fp); return 0;
  }
  parsed_command->use_path = (short)intvalue;

  ret = getlinedup(fp,&value);
  if(!ret){
    fclose(fp); return 0;
  }
  intvalue = atoi(value);
  if(intvalue != 0 && intvalue != 1){
    fclose(fp); return 0;
  }
  parsed_command->copy_environment = (short)intvalue;

  ret = getlinedup(fp,&value);
  if(!ret){
    fclose(fp); return 0;
  }
  intvalue = atoi(value);
  if(intvalue <-20 || intvalue >19){
    fclose(fp); return 0;
  }
  parsed_command->niceness = (short)intvalue;

  ret = getlinedup(fp,&value);
  if(!ret){
    fclose(fp); return 0;
  }
  intvalue = atoi(value);
  if(intvalue != 0 && intvalue != 1){
    fclose(fp); return 0;
  }
  parsed_command->wait = (short)intvalue;

  ret = getlinedup(fp,&value);
  if(!ret){
    fclose(fp); return 0;
  }
  intvalue = atoi(value);
  if(intvalue < 0){
    fclose(fp); return 0;
  }
  parsed_command->timeout = (unsigned int)intvalue;

  return 1;
}

//useful for debugging
void print_parsed_command(command parsed_command){
  printf("----------\n");
  printf("binary_path: %s\n", parsed_command.binary_path);
  printf("stdin: %s\n", parsed_command.stdin);
  printf("stdout: %s\n", parsed_command.stdout);
  printf("stderr: %s\n", parsed_command.stderr);
  printf("arguments: %s\n", parsed_command.arguments);
  printf("extra_environment: %s\n", parsed_command.extra_environment);
  printf("use_path: %d\n", parsed_command.use_path);
  printf("copy_environment: %d\n", parsed_command.copy_environment);
  printf("niceness: %d\n", parsed_command.niceness);
  printf("wait: %d\n", parsed_command.wait);
  printf("timeout: %d\n", parsed_command.timeout);
}

void free_command(command cmd){
  free(cmd.binary_path);
  free(cmd.stdin);
  free(cmd.stdout);
  free(cmd.stderr);
  free(cmd.arguments);
  free(cmd.extra_environment);
}

int process_command(command parsed_command, char *env[]){

        if (strlen(parsed_command.stdout) != 0) {
            int fd = open(parsed_command.stdout, O_WRONLY|O_CREAT|O_TRUNC, 0064);
            fflush(stdout);	
            dup2(fd,1);
            close(fd);
        }

        if (strlen(parsed_command.stderr) != 0) {
            int fd = open(parsed_command.stderr, O_WRONLY|O_CREAT|O_TRUNC, 0064);
            fflush(stderr);	
            dup2(fd,2);
            close(fd);
	}

        if (strlen(parsed_command.stdin) != 0) {
            int fd = open(parsed_command.stdin, O_RDONLY, 0064);
            fflush(stdin);	
            dup2(fd,0);
            close(fd);
	}
	
        setpriority(PRIO_PROCESS, 0, parsed_command.niceness);
        char const *hex_str = parsed_command.arguments;
        char const *extra_hex_str = parsed_command.extra_environment;
        size_t str_len = strlen(hex_str);
        size_t extra_str_len = strlen(extra_hex_str);
        size_t len = str_len / 2;
        size_t len_extra = extra_str_len/2;
        char *str = calloc(len + 1, 1);
        char *extra_str = calloc(len + 1,1);
        size_t str_count = 1;
        size_t extra_str_count = 1;

        for (size_t i = 0; i < len; ++i) {
            str[i] = hex2dec(hex_str[i * 2]) * 16 + hex2dec(hex_str[i * 2 + 1]);
            if (!str[i])
                ++str_count;
        }

        for (size_t i = 0; i < len_extra; ++i) {
            extra_str[i] = hex2dec(extra_hex_str[i * 2]) * 16 + hex2dec(extra_hex_str[i * 2 + 1]);
            if (!extra_str[i])
                ++extra_str_count;
        }

        char **split_str = malloc((str_count+1) * sizeof(char*));
        char **extra_split_str = malloc((extra_str_count) * sizeof(char*));
        split_str[0]=parsed_command.binary_path;

        for (size_t i = 1, offset = 0; i < str_count+1; ++i) {
            size_t len = strlen(str + offset);
            split_str[i] = calloc(len + 1, 1);
            strcpy(split_str[i], str + offset);
            offset += len + 1;
        }

        for (size_t i = 0, offset = 0; i < extra_str_count+1; ++i) {
            size_t extra_len = strlen(extra_str + offset);
            extra_split_str[i] = calloc(len_extra + 1, 1);
            strcpy(extra_split_str[i], extra_str + offset);
            offset += len + 1;
        }

        free(str);
        free(extra_str);

        if(parsed_command.use_path == 0 && parsed_command.copy_environment == 0 && parsed_command.timeout == 0){
            execvpe(split_str[0],split_str,env);
        }
        else if(parsed_command.use_path == 1 && parsed_command.copy_environment == 0 && parsed_command.timeout == 0){
            execvpe(split_str[0],split_str,env);
        }
        else if(parsed_command.use_path == 1 && parsed_command.copy_environment == 1 && parsed_command.timeout == 0){
             execvpe(split_str[0],split_str,env);

        }
        else if(parsed_command.use_path == 1 && parsed_command.copy_environment == 0 && parsed_command.timeout != 0){
            int timeout_val = parsed_command.timeout;
            int timeout_length = snprintf(NULL, 0, "%d", timeout_val);
            char *timeout_str = malloc(timeout_length + 1);
            snprintf(timeout_str, timeout_length + 1, "%d", timeout_val);
            char **split_str_wait = malloc((str_count+5) * sizeof(char*));
            int num_args = 0;
            while(split_str[++num_args] != NULL);
            split_str_wait[0] = "/usr/bin/timeout";
            split_str_wait[1] = "--preserve-status";
            split_str_wait[2] = "-k";
            split_str_wait[3] = "1";
            split_str_wait[4] = timeout_str;
            for(int q = 0; q < num_args;q++){
                memcpy(&split_str_wait[5+q], &split_str[q], sizeof(&split_str[q]));
            }
            split_str_wait[5+num_args] = '\0';
            execvpe(split_str_wait[0],split_str_wait,env);
        }

        for (size_t i = 0; i < str_count; ++i)
            free(split_str[i]);
        free(split_str);
	exit(0);
    }


int hex2dec(int value)
{
    if (isdigit(value))
        return value - '0';

    value = toupper(value);

    if ('A' <= value && value <= 'F')
        return 10 + value - 'A';

    return -1;
}

int main(int argc, char *argv[], char* env[]) {

  for(int ncommand=1; ncommand<argc; ncommand++){
    command parsed_command;
    int ret = parse_command(&parsed_command, argv[ncommand]);
    if (!ret){
      printf("command file %s is invalid\n", argv[ncommand]);
      continue;
    }

    /*
    process_command will:
    - get a parsed_command variable
    - create a child process
    - set file redirection, niceness, arguments, envirionment variables, ...
    - call a proper variant of execv
    - print when a child process is created and when any child process is terminated
    - if necessary, wait for the termination of the program
    */

    int pid = fork();
    if (pid ==0){
        printf("New child process started: %u\n",getpid());
	process_command(parsed_command,env);
        fflush(stdout);	

    }
    else if (parsed_command.wait == 1){
        int code;
        pid_t process = waitpid(-1,&code, 0);
        printf("Child process %d terminated with exit code %d\n",process,WEXITSTATUS(code));
    }
    free_command(parsed_command);
    }
  
    //remember to wait for the termination of all the child processes, regardless of the value of parsed_command.wait
    while(1){
        int code;
        pid_t process = waitpid(-1,&code, 0);
        if(process == -1){
            break;
        } 
    printf("Child process %d ended with exit code %d\n",process,WEXITSTATUS(code));
    }
}


