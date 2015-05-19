// based on http://stephen-brennan.com/2015/01/16/write-a-shell-in-c/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
//#include <unistd.h>

char *lsh_read_line(void){
	char *line = NULL;
	int ch;
	ssize_t bufleng = 0; //force getline to allocate buffer
	
	getline(&line, &bufleng, stdin);
	return line;
}

#define TOK_DELIM " \t\r\n\a"

char **lsh_parse_line(char *line){
	int bufleng = 64;
	int pos = 0;
	char **tokens = malloc(bufleng * sizeof(char*)); //array of pointers
	char *token;
	
	if (!tokens) {
		fprintf(stderr, "lsh: allocation error\n");
		exit(EXIT_FAILURE);	
	}
	
	token = strtok(line, TOK_DELIM);	//scan for tokens starting at line and continuing until delimiter found
	for (;token != NULL; pos++) {
		tokens[pos] = token;
		
		if (pos >= bufleng) {
			bufleng = bufleng + 64;		//if buffer too small, dynamically expand it
			tokens = realloc(tokens, bufleng * sizeof(char*));
			if (!tokens) {
				fprintf(stderr, "lsh: allocation error\n");
				exit (EXIT_FAILURE);
			}
		}
		token = strtok(NULL, TOK_DELIM);	//continue scanning for tokens where previous call left off	
	}
	tokens[pos] = NULL;	//array of pointers ends with null char
	return tokens;
}

int lsh_exec(char **args){
	pid_t pid;
	pid_t wpid;
	int status;
	
	pid = fork();	//copy of current process made
	if (pid == 0) {	n	//child process
		if (execvp(args[0], args) == -1) {	
			perror("lsh");
		}
		exit(EXIT_FAILURE);
	}
	else if (pid<0) {	//fork had an error
		perror("lsh");
	}
	else {	//fork executed successfully. parent process
		while (!WIFEXITED(status) && !WIFSIGNALED(status)) {	//wait until child process is exited or killed
			wpid = waitpid(pid, &status, WUNTRACED);
		}
	}
	
	return 1;
}

void lsh_loop(void){
	char *line;
	char **args;
	int status;
	
	do{
		printf(": ");
		line = lsh_read_line();
		args = lsh_parse_line(line);
		status = lsh_exec(args);
		
		free(line);
		free(args);
	} while (status);

}

int main(int argc, char **argv){
	lsh_loop();
	return EXIT_SUCCESS;
}