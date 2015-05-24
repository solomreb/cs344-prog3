//Adapted from http://stephen-brennan.com/2015/01/16/write-a-shell-in-c/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

void printargs(char **args){
	int i;
	for (i=0; i<5; i++){
		printf("args[%d] = %s\n", i, args[i]);
	}
}

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

//returns index at which c can be found in args, if not found returns -1
int sh_find_char(char **args, char *c){
	int i;
	for (i=0; args[i]!= NULL; i++){
		if (strncmp(args[i], c, 1) == 0){
			//printf("%s found at args[%d]\n", c, i);
			return i;
		}
	}
	return -1;
}

//returns specialChars[input, output, background]
void sh_redirect(char **args){
	int out = -1;
	int in = -1;
	int output = 0;
	int input = 0;
	
	in = sh_find_char(args, "<");
	out = sh_find_char(args, ">");

	if (in >= 0){ //input
		input = open(args[in + 1], O_RDONLY);
		if (input < 0) {
			fprintf(stderr, "smallsh: cannot open %s for input\n", args[1]);
			exit(EXIT_FAILURE);
		}
		dup2(input, 0); 		  // replace standard input with input file
		args[in] = args[in+1] = NULL;
	}
	if (out >= 0){ //output
		output = creat(args[out + 1], 0666); //file to use as out is arg following ">"
		if (output < 0) {
			fprintf(stderr, "%s: no such file or directory\n", args[1]);
			exit(EXIT_FAILURE);
		}
		dup2(output, 1);			//replace standard output with outputfile	
		args[out] = args[out+1] = NULL;
	
	}
	return;
}

int sh_cd(char **args)
{
  if (args[1] == NULL) {
    chdir(getenv ("HOME"));
  } else {
    if (chdir(args[1]) != 0) {
      fprintf(stderr, "error: cd\n");
    }
  }
  return 1;
}


/*int sh_status(char **args){
	

}*/

int sh_execute(char **args){
	pid_t pid = -5;
	pid_t wpid = -5;
	int status, exitstatus, bg, comment;
	
	bg = sh_find_char(args, "&");
	comment = sh_find_char(args, "#");
	
	if (args[0] == NULL || comment >= 0){ //ignore null inputs and comments
			return 1;
	}
	if (strcmp(args[0], "cd") == 0) {
		sh_cd(args);
		return 1;
	}
	if (strcmp(args[0], "exit") == 0) {
		return 0;
	}
	
	/*if (strcmp(args[0], "status") == 0) {
		sh_status(args);
		return 1;
	}*/
	
	pid = fork(); 	//create copy of curr process
	
	if (pid == 0){ 	//child process. execute command
		
		sh_redirect(args); //do any necessary redirection
		
		if (bg >= 0) {
			args[bg] = NULL;
		}
		if (execvp(args[0], args) == -1) {	//exec only returns if error
			fprintf(stderr,"Error executing command\n");
			exit(EXIT_FAILURE);
		}
		
	} 
	else if (pid > 0){ 	//parent process
		if (bg >= 0) {	//child is background process. do not wait for it
			while (!WIFEXITED(status) && !WIFSIGNALED(status)) {	
				wpid = waitpid(pid, &status, WNOHANG | WUNTRACED); //WNOHANG = don't wait
			}
		}
		else {
			while (!WIFEXITED(status) && !WIFSIGNALED(status)) {	//wait until child process is exited or killed
				wpid = waitpid(pid, &status, WUNTRACED);
			}
		}
		exitstatus = WEXITSTATUS(status);
	}	
	else { 	//error forking
		fprintf(stderr, "Error forking parent process\n");
		exit(EXIT_FAILURE);
	}
	return 1;
}

int main(int argc, char **argv){
	
	usage(argc, argv);

	char *line;
	char **args;
	int status;

  	
	do {
		printf(": ");
		fflush(stdout);
		line = sh_get_line();
		args = sh_parse_line(line);
		status = sh_execute(args);
		free (line);
		free (args);
		
	} while (status == 1);
	
	return 0;
}
