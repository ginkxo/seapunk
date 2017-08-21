#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "seapunk.h"

#define SEAPUNK_BUFFER 1024

int main(int argc, char **argv) {

	seashell();

	return EXIT_SUCCESS;
}

char *seapunk_time(void) {

	/* timestamp function */

	char *times = (char *) malloc(sizeof(char) * 16);

	time_t ltime;
	ltime = time(NULL);
	struct tm *tm;
	tm = localtime(&ltime);
	
	sprintf(times, "%02d:%02d:%02d", tm->tm_hour, tm->tm_min, tm->tm_sec);
	
	return times;
}

void seashell(void) {

	/* 
		primary seapunk shell loop function
		- takes a line
		- breaks the line into arguments
		- executes the arguments, and returns a state value
			- this loop will break when the state value has a completion status
	*/

	char *line;
	char **arguments;
	int state;

	do {

		char currentdir[1024];

		getcwd(currentdir, sizeof(currentdir));

		printf("[seapunk | %s | %s]  > ", currentdir, seapunk_time()); // printed prompt
		line = seapunk_read(); // read a line
		// line = seapunk_read_line(); // alternative with getline()
		arguments = seapunk_split(line); // split the line into arguments
		state = seapunk_do(arguments); // execute the arguments and return a status

		free(line);
		free(arguments);

	} while (state);
}

char *seapunk_read(void) {

	/* reads a line from the prompt

		the main issue: 
		we don't know how much text someone will enter beforehand
		therefore, we can't easily allocate a set amount of space for an input
		thus, we need to use realloc to re-allocate space as necessary
	*/

	int bufsz = SEAPUNK_BUFFER;
	int pos = 0;
	char *buf = malloc(sizeof(char) * bufsz);

	int ch;

	if (!buf) {
		fprintf(stderr, "seapunk: could not allocate memory on heap\n");
		exit(EXIT_FAILURE);	
	}

	
	// main read loop, char by char
	while(1) {

		ch = getchar(); // important - store char as int, to check EOF

		if (ch == EOF || ch == '\n') {
			buf[pos] = '\0';
			return buf; // return string if end

		} else {
			buf[pos] = ch; // add char to string
		}
		pos++; //goto next string pos

		// reallocation if the buffer is exceeded

		if (pos >= bufsz) {

			bufsz += SEAPUNK_BUFFER;
			buf = realloc(buf, bufsz);
			
			if (!buf) {
				fprintf(stderr, "seapunk: could not re-allocate memory on heap\n");
				exit(EXIT_FAILURE);
			}

		}
	}
} 

/*

char *seapunk_read_line(void) {

	char *line = NULL;
	ssize_t = bufsz = 0;
	getline(&line, &bufsize, stdin);
	return line
} 

*/

#define SEAPUNK_TOKEN_BUFFERSZ 64
#define SEAPUNK_TOKEN_DELIMIT " \t\r\n\a"
char **seapunk_split(char *line) {
	
	/* grabs a line and produces an array of tokens 
		we are just going to use whitespace delimiters here
		again we dynamically re-allocate the buffer as necessary
		(i think i might make a sub-routine for this	
	*/
	

	int bufsz, pos;
	bufsz = SEAPUNK_TOKEN_BUFFERSZ;
	pos = 0;

	char **tokens = malloc(bufsz * sizeof(char*));
	char *token;

	if (!tokens) {
		fprintf(stderr, "seapunk: failed to allocate memory for tokens\n");
		exit(EXIT_FAILURE);
	}

	token = strtok(line, SEAPUNK_TOKEN_DELIMIT);
	
	while (token != NULL) {
		
		tokens[pos] = token;
		pos++;

		if (pos >= bufsz) {
			bufsz += SEAPUNK_TOKEN_BUFFERSZ;
			tokens = realloc(tokens, bufsz * sizeof(char*));
			if (!tokens) {
				fprintf(stderr, "seapunk: failed to re-allocate more memory for tokens\n");
				exit(EXIT_FAILURE);
			}
		}

		token = strtok(NULL, SEAPUNK_TOKEN_DELIMIT); // ACCORDING to the man page, first arg now NULL
		// only for whenever we have the same line

	}

	tokens[pos] = NULL; // null terminate the token list
	// each token represents a program, argument, etc etc
	return tokens;
}

int seashell_execute(char **arguments) {

	pid_t pid;
	pid_t wait_pid;

	int status;

	pid = fork();

	if (pid == 0) {
		// child

		if (execvp(arguments[0], arguments) == -1) {
			perror("seapunk");
		}

	} else if (pid < 0) {
		// error

		perror("seapunk");

	} else {
		// parent
		do {
			
			wait_pid = waitpid(pid, &status, WUNTRACED);

		} while (!WIFEXITED(status) && !WIFSIGNALED(status));
	}

	return 1;
}

int seapunk_cd(char **args);
int seapunk_lsbasic(char **args);
int seapunk_help(char **args);
int seapunk_exit(char **args);

// builtins

char *builtins[] = {

	"swimto",
	"splash",
	"help",
	"exit"
};

// corresponding functions to builtins

int (*builtin_funk[]) (char **) = {
	
	&seapunk_cd,
	&seapunk_lsbasic,
	&seapunk_help,
	&seapunk_exit
	
};

int seapunk_numberof_builtins() {

	return sizeof(builtins) / sizeof(char *);
}

// builtin implementation

int seapunk_cd(char **args) {

	if (args[1] == NULL) {
		fprintf(stderr, "seapunk: argument for swimto expected\n");
	} else {
		if (chdir(args[1]) != 0) {
			perror("lsh");
		}
	}

	return 1;

}

int seapunk_lsbasic(char **args) {

	DIR *directory;
	struct dirent *dir;
	
	if (args[1] == NULL) {
		directory = opendir(".");
		
		if (directory) {
			while ((dir = readdir(directory)) != NULL) {
				printf("%s\n", dir->d_name);
			} 
		}

	} else {
		fprintf(stderr, "seapunk: splash requires no arguments");
	}

	return 1;	

}

int seapunk_help(char **args) {

	printf("The Seapunk C Shell, a mini project by ginkxo.\n");
	printf("Implemented commands: \n");
	printf("swimto (change directory)\nsplash (ls)\nhelp\nexit\n ");
	printf(" \n");
	printf("Use man pages for other help\n");
	return 1;

}

int seapunk_exit(char **args) {

	return 0;

}

int seapunk_do(char **args) {

	int i;

	if (args[0] == NULL) {
		return 1;
	}

	for (i = 0; i < seapunk_numberof_builtins(); i++) {
		if (strcmp(args[0], builtins[i]) == 0) {
			return (*builtin_funk[i])(args);
		}

	}

	return seashell_execute(args);
}



