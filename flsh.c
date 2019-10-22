#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define FLSH_RL_BUFSIZE 1024
#define FLSH_TOK_BUFSIZE 64
#define FLSH_TOK_DELIM " \t\r\n\a"

void flsh_loop(void);
char *flsh_read_line(void);
char **flsh_split_line(char *);
int flsh_execute(char **);

int main(int argc, char **argv)
{
	// Load config files.
	
	// Run command loop.
	flsh_loop();

	// Shutdown.
	return EXIT_SUCCESS;
}

void flsh_loop(void)
{
	char *line;
	char **args;
	int status;

	do {
		printf("> ");
		line = flsh_read_line();
		args = flsh_split_line(line);
		status = flsh_execute(args);

		free(line);
		free(args);
	} while (status);
}

char *flsh_read_line(void)
{
	int bufsize = FLSH_RL_BUFSIZE;
	int position = 0;
	char *buffer = malloc(sizeof(char) * bufsize);
	char *buffer_tmp;
	int c;

	if (!buffer) {
		fprintf(stderr, "flsh: allocation error\n");
		exit(EXIT_FAILURE);
	}

	while (1) {
		// Read a character.
		c = getchar();

		// If hit EOF, replace it with a null character
		// and return.
		if (c == EOF || c == '\n') {
			buffer[position] = '\0';
			return buffer;
		} else {
			buffer[position] = c;
		}
		position++;

		// If have exceeded the buffer, reallocate.
		if (position >= bufsize) {
			bufsize += FLSH_RL_BUFSIZE;
			buffer_tmp = realloc(buffer, bufsize * sizeof(char));
			if (buffer_tmp != NULL) {
				buffer = buffer_tmp;
			} else {
				fprintf(stderr, "flsh: allocation error\n");
				exit(EXIT_FAILURE);
			}
		}
	}
}

char **flsh_split_line(char *line)
{
	int bufsize = FLSH_TOK_BUFSIZE;
	int position = 0;
	char **tokens = malloc(bufsize * sizeof(char*));
	char **tokens_tmp;
	char *token;

	if (!tokens) {
		fprintf(stderr, "flsh: allocation error\n");
		exit(EXIT_FAILURE);
	}
	
	token = strtok(line, FLSH_TOK_DELIM);
	while (token != NULL) {
		tokens[position] = token;
		position++;

		if (position >= bufsize) {
			bufsize += FLSH_TOK_BUFSIZE;
			tokens_tmp = realloc(tokens, bufsize * sizeof(char*));
			if (tokens_tmp != NULL) {
				tokens = tokens_tmp;
			} else {
				fprintf(stderr, "flsh: allocation error\n");
				exit(EXIT_FAILURE);
			}
		}

		token = strtok(NULL, FLSH_TOK_DELIM);
	}
	tokens[position] = NULL;
	return tokens;
}

int flsh_launch(char **args)
{
	pid_t pid;
	int status;

	pid = fork();
	if (pid == 0) {
		// Child process.
		if (execvp(args[0], args) == -1) {
			perror("flsh");
		}
		exit(EXIT_FAILURE);
	} else if (pid < 0) {
		// Error forking.
		perror("flsh");
	} else {
		// Parent process.
		do {
			waitpid(pid, &status, WUNTRACED);
		} while (!WIFEXITED(status) && !WIFSIGNALED(status));
	}

	return 1;
}

/*
 * Function declarations for builtin shell commands:
 */

int flsh_cd(char **args);
int flsh_help(char **args);
int flsh_exit(char **args);

/*
 * List of builtin commands, followed by their corresponding functions.
 */

char *builtin_str[] = {
	"cd",
	"help",
	"exit"
};

int (*builtin_func[]) (char **) = {
	&flsh_cd,
	&flsh_help,
	&flsh_exit
};

int flsh_num_builtins()
{
	return sizeof(builtin_str) / sizeof(char *);
}

/*
 * Builtin function implementations.
 */

int flsh_cd(char **args)
{
	if (args[1] == NULL) {
		fprintf(stderr, "flsh: expected argument to \"cd\"\n");
	} else {
		if (chdir(args[1]) != 0) {
			perror("flsh");
		}
	}
	return 1;
}

int flsh_help(char **args)
{
	int i;
	printf("Bruno's FLSH\n");
	printf("Type program names and arguments, and hit enter.\n");
	printf("The following are built in:\n");

	for (i = 0; i < flsh_num_builtins(); i++) {
		printf("  %s\n", builtin_str[i]);
	}
	
	printf("Use the man command for information on other programs.\n");
	return 1;
}

int flsh_exit(char **args)
{
	return 0;
}

int flsh_execute(char **args)
{
	int i;

	if (args[0] == NULL) {
		// An empty command was entered.
		return 1;
	}

	for (i = 0; i < flsh_num_builtins(); i++) {
		if (strcmp(args[0], builtin_str[i]) == 0) {
			return (*builtin_func[i])(args);
		}
	}

	return flsh_launch(args);
}
