//Adapted from http://stephen-brennan.com/2015/01/16/write-a-shell-in-c/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

void usage(int argc, char **argv){
	if (argc != 1){
		fprintf(stderr, "Usage: %s\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	return;
}

char *sh_get_line(){
	char *line = NULL;
   size_t len = 0;
   getline(&line, &len, stdin);
   return line;
}

#define SH_DELIM " \n\r"
#define SH_BUFSIZE 64
char **sh_parse_line(char *line){

	int bufsize = SH_BUFSIZE;
	char **args = malloc(bufsize * sizeof(char*));
	char *token;
	int pos;
	
	token = strtok(line, SH_DELIM);
	
	for (pos=0; token != NULL ;pos++){
		args[pos] = token;
		if (pos >= bufsize){ //grow bufsize dynamically
			bufsize = bufsize * 2;
			args = realloc(args, bufsize * sizeof(char*));
			if (!args){
				fprintf(stderr, "allocation error\n");
				exit(EXIT_FAILURE);
			}
		}
		token = strtok(NULL, SH_DELIM);	//when first param NULL, strtok starts where it left off
	}
	args[pos] = NULL;	//last element of args is NULL char
	return args;
}

void sh_redirect(char **args){
	int input = 0, output = 0, i;
	
	for (i=0; args[i]!= NULL; i++){
		if (strcmp(args[i], "<") == 0){
			input = open(args[i+1], O_RDONLY); //file to use as input is arg following "<"
			dup2(input, 0); 		  // replace standard input with input file
		}
		if (strcmp(args[i], ">") == 0){
			output = open(args[i+1], O_WRONLY); //file to use as out is arg following ">"
			dup2(output, 1);			//replace standard output with outputfile		
		}
	}
	//close(input);
	//close(output);
	return;
}

int sh_execute(char **args){
	pid_t pid = -5;
	pid_t wpid = -5;
	int status, exitstatus;
	if (strcmp(args[0], "#") == 0){
			return 1;
	}
		
	pid = fork(); 	//create copy of curr process
	
	if (pid == 0){ 	//child process. execute command
		sh_redirect(args); 	//check if command involves redirection
		if (execvp(args[0], args) == -1) {	
			fprintf(stderr,"Error executing command\n");
			exit(EXIT_FAILURE);
		}
		
	} 
	else if (pid > 0){ 	//parent process. wait for child

		while (!WIFEXITED(status) && !WIFSIGNALED(status)) {	//wait until child process is exited or killed
			wpid = waitpid(pid, &status, WUNTRACED);
		} 
	}	
	else { 	//error forking
		fprintf(stderr, "Error forking parent process");
		exit(EXIT_FAILURE);
	}
	return 1;
}

int main(int argc, char **argv){
	
	usage(argc, argv);

	char *line;
	char **args;
	int status;
	
	int* numArgs;
	numArgs = 0;

  	
	do {
		printf(": ");
		line = sh_get_line();
		args = sh_parse_line(line);
		status = sh_execute(args);
		free (line);
		free (args);
	} while (status == 1);
	
	return 0;
}
//open //dup2
//0 = stdin
//1 = stdout
//2 = stderr
//open(junk, readonly) = 3; 3 is the next available filedescriptor
//then read from 3
//if -1, then file doesn't exist or other error
//
