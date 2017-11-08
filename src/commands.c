#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include "commands.h"
#include "built_in.h"


static struct built_in_command built_in_commands[] = {
	{ "cd", do_cd, validate_cd_argv },
	{ "pwd", do_pwd, validate_pwd_argv },
	{ "fg", do_fg, validate_fg_argv }
};

static int is_built_in_command(const char* command_name)
{
	static const int n_built_in_commands = sizeof(built_in_commands) / sizeof(built_in_commands[0]);//num of commands

	for (int i = 0; i < n_built_in_commands; ++i) {
		if (strcmp(command_name, built_in_commands[i].command_name) == 0) {
			return i;
		}
	}

	return -1; // Not found
}

/*
* Description: Currently this function only handles single built_in commands. You should modify this structure to launch process and offer pipeline functionality.
*/
int evaluate_command(int n_commands, struct single_command (*commands)[512])
{
	//if (n_commands > 0) {
	struct single_command* com = (*commands);

	for(int i = 0; i < n_commands; i++) {

		assert((com + i)->argc != 0);//if com->argc == 0, abort
		
		outfd = dup(1);
		close(1);

		int built_in_pos = is_built_in_command((com + i)->argv[0]);
		if (built_in_pos != -1) {
			if (built_in_commands[built_in_pos].command_validate((com + i)->argc, (com + i)->argv)) {
				if (built_in_commands[built_in_pos].command_do((com + i)->argc, (com + i)->argv) != 0) {
					// TODO: Implements ~ 
					fprintf(stderr, "%s: Error occurs\n", (com + i)->argv[0]);
				}
			} 
			else {
				fprintf(stderr, "%s: Invalid arguments\n", (com + i)->argv[0]);
				return -1;
			}
		} 
		else if (strcmp((com + i)->argv[0], "") == 0) {
			return 0;
		} 
		else if (strcmp((com + i)->argv[0], "exit") == 0) {
			return 1;
		} 
		else {
			pid_t pid;
			pid = fork();

			if(pid < 0){
				fprintf(stderr, "Fork Failed");
				return -1;
			}
			else if(pid == 0){

				char path[32][128] = {
					"/usr/local/bin/",
					"/usr/bin/",
					"/bin/",
					"/usr/sbin/",
					"/sbin/",
					""
				};

				for(int j = 0; j < 6; j++)
				{
					char* command_path = (char *)malloc(strlen(path[j]) * sizeof(char) + strlen((com + i)->argv[0]));
					strcpy(command_path, path[j]);
					strcat(command_path, (com + i)->argv[0]);
					execv(command_path, (com + i)->argv);
					free(command_path);
				}
				fprintf(stderr, "Command not found\n");
				exit(0);
			}
			else{
				//for background process, need to be modified to not wait when & comes in the end of the command
				wait(NULL);
				printf("Child complete\n");
			}

		}
	}

	return 0;
}

void free_commands(int n_commands, struct single_command (*commands)[512])
{
	for (int i = 0; i < n_commands; ++i) {
		struct single_command *com = (*commands) + i;
		int argc = com->argc;
		char** argv = com->argv;

		for (int j = 0; j < argc; ++j) {
			free(argv[j]);
		}

		free(argv);
	}

	memset((*commands), 0, sizeof(struct single_command) * n_commands);
}
