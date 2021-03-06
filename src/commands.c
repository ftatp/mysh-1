#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <pwd.h>
#include <pthread.h>

#include "commands.h"
#include "built_in.h"

#define SERVER_PATH "tpf_unix_sock.server"
#define CLIENT_PATH "tpf_unix_sock.client"
#define SHMSZ 27


static struct built_in_command built_in_commands[] = {
	{ "cd", do_cd, validate_cd_argv },
//	{ "pwd", do_pwd, validate_pwd_argv },
	{ "fg", do_fg, validate_fg_argv }
};

enum success_code{ SUCCESS, CDEXEERROR, CDVARERROR, BLANKCOM, EXITCOM, FORKERROR, COMNOTEXIST };

int client_sock;
int tempoutfd;

void* get_input_from_pre_com(void* p_com){//client side
    int rc, len;
    struct sockaddr_un server_sockaddr; 
    struct sockaddr_un client_sockaddr; 
    char buf[256];
    char received_buf[256];

	memset(&server_sockaddr, 0, sizeof(struct sockaddr_un));
    memset(&client_sockaddr, 0, sizeof(struct sockaddr_un));
     
    /**************************************/
    /* Create a UNIX domain stream socket */
    /**************************************/
    client_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (client_sock == -1) {
        printf("SOCKET ERROR = 1\n");
        exit(1);
    }

    /***************************************/
    /* Set up the UNIX sockaddr structure  */
    /* by using AF_UNIX for the family and */
    /* giving it a filepath to bind to.    */
    /*                                     */
    /* Unlink the file so the bind will    */
    /* succeed, then bind to that file.    */
    /***************************************/
    client_sockaddr.sun_family = AF_UNIX;   
    strcpy(client_sockaddr.sun_path, CLIENT_PATH); 
    len = sizeof(client_sockaddr);
    
    unlink(CLIENT_PATH);

    rc = bind(client_sock, (struct sockaddr *) &client_sockaddr, len);
    if (rc == -1){
        printf("BIND ERROR: 2\n");
        close(client_sock);
        exit(1);
    }
        
    /***************************************/
    /* Set up the UNIX sockaddr structure  */
    /* for the server socket and connect   */
    /* to it.                              */
    /***************************************/
    server_sockaddr.sun_family = AF_UNIX;
    strcpy(server_sockaddr.sun_path, SERVER_PATH);

    
	while((rc = connect(client_sock, (struct sockaddr *) &server_sockaddr, len)) == -1){
        //printf("CONNECT ERROR = 3\n");
        //close(client_sock);
        //exit(1);
    }
  
	struct single_command* com = (struct single_command*)p_com;
	
	tempoutfd = dup(STDOUT_FILENO);
	dup2(client_sock, STDOUT_FILENO);

	int ret = execute(com);
	
	close(client_sock);
	dup2(tempoutfd, STDOUT_FILENO);
    
	switch(ret){
		case SUCCESS://success
			break;
		case CDEXEERROR:
			fprintf(stderr, "Error occurs: Not able to do cd\n");
			break;
		case CDVARERROR:
			fprintf(stderr, "Error occurs: not able to validate\n");
		case BLANKCOM:
			break;
		case EXITCOM:
			break;
		case FORKERROR:
			break;
		case COMNOTEXIST:
			printf("Command does not exists\n");
			break;
		default:
			break;
	}
   
	// Close the socket and exit.
	pthread_exit(NULL);
}



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


//pthread_t client_thread[32];
//int com_idx = 0;

int evaluate_command(int n_commands, struct single_command (*commands)[512])
{
	struct single_command* com = (*commands);	
	
//	com_idx = 0;
	
	int ret = evaluate(n_commands, com);

	if(ret == 1)
		return 1;

	return 0;
}


int evaluate(int n_commands, struct single_command* com){
	int ret = 0;
	
	if(n_commands == 1){
		ret = execute(com);
		if(ret == EXITCOM)
			return 1;
		else
			return 0;
	}
	else{
		int server_sock = setup_server();
		int client_sock;
    	struct sockaddr_un client_sock_addr; 
		int len;
		pthread_t client_thread;

		len = sizeof(struct sockaddr_un);
		memset(&client_sock_addr, 0, len);

		pthread_create(&client_thread, NULL, get_input_from_pre_com, com);
		
		client_sock = accept(server_sock, (struct sockaddr *) &client_sock_addr, &len);
		if (client_sock == -1){
			printf("ACCEPT ERROR: 4\n");
			close(server_sock);
			close(client_sock);
			exit(1);
		}   
		pthread_join(client_thread, NULL);

		int tempinfd = dup(STDIN_FILENO);
		dup2(client_sock, STDIN_FILENO);

		ret = evaluate(n_commands - 1, com + 1);
		if(ret == EXITCOM)
			return 1;

		//if(ret) return
		close(client_sock);
		dup2(tempinfd, STDIN_FILENO);
	}
}

int execute(struct single_command* com){
	
	struct passwd* pw = getpwuid(getuid());
	char* homepath = pw->pw_dir;
	int return_code = 0;
	char path[256] = {0};

	for(int i = 0; i < com->argc; i++){
		memset(path, 0, sizeof(path));
		strcpy(path, com->argv[i]);

		if(path[0] == '~' && (path[1] == '\0' || path[1] == '/')){
			char* temp_arg = malloc(sizeof(char) * strlen(path));
			if(path[1] == '/')
				strcpy(temp_arg, path + 2);

			strcpy(path, homepath);
			path[strlen(homepath)] = '/';
			strcpy((path + strlen(homepath) + 1), temp_arg);

			//printf("new arg: %s\n", com->argv[i]);
			free(temp_arg);
			memset(com->argv[i], 0, sizeof(char) * strlen(com->argv[i]));
			strcpy(com->argv[i], path);
		}
	}

	int built_in_pos = is_built_in_command(com->argv[0]);
	if (built_in_pos != -1) {
		if (built_in_commands[built_in_pos].command_validate(com->argc, com->argv)) {
			if (built_in_commands[built_in_pos].command_do(com->argc, com->argv) != 0) {
				return_code = CDEXEERROR;
			}
		} 
		else {
			return_code = CDVARERROR;
			//fprintf(stderr, "%s: Invalid arguments\n", com->argv[0]);
			//return -1;
		}
	} 
	else if (strcmp(com->argv[0], "") == 0) {
		return_code = BLANKCOM;
		//return 0;
	} 
	else if (strcmp(com->argv[0], "exit") == 0) {
		return_code = EXITCOM;
		//return 1;
	} 
	else {
		pid_t pid_for_exe;
		pid_for_exe = fork();

		if(pid_for_exe < 0){
			return_code = FORKERROR;
			//return -1;
			//fprintf(stderr, "Fork Failed");
		}
		else if(pid_for_exe == 0){

			char path[32][128] = {
				"/usr/local/bin/",
				"/usr/bin/",
				"/bin/",
				"/usr/sbin/",
				"/sbin/"
			};

			for(int j = 0; j < 6; j++)
			{
				char* command_path = (char *)malloc(strlen(path[j]) * sizeof(char) + strlen(com->argv[0]));
				strcpy(command_path, path[j]);
				strcat(command_path, com->argv[0]);
				execv(command_path, com->argv);
				free(command_path);
			}
			return_code = COMNOTEXIST;
		
			close(client_sock);
			dup2(tempoutfd, STDOUT_FILENO);

			fprintf(stderr, "Command not found\n");
			
			tempoutfd = dup(STDOUT_FILENO);
			dup2(client_sock, STDOUT_FILENO);
			
			exit(0);
		}
		else{
			//for background process, need to be modified to not wait when & comes in the end of the command
			wait(NULL);
			//printf("Child for exe complete\n");
		}

	}

	return return_code;
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


int setup_server(){
    int server_sock, len, rc;
    int bytes_rec = 0;
    struct sockaddr_un server_sockaddr;
    //struct sockaddr_un client_sockaddr;     

    int backlog = 10;
    memset(&server_sockaddr, 0, sizeof(struct sockaddr_un));
    //memset(&client_sockaddr, 0, sizeof(struct sockaddr_un));
    //memset(buf, 0, 256);                
    
    // Create a UNIX domain stream socket 
    server_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_sock == -1){
        printf("SOCKET ERROR: 1\n");
        exit(1);
    }
    
    /* Set up the UNIX sockaddr structure by using AF_UNIX for the family and giving it a filepath to bind to.    
       Unlink the file so the bind will succeed, then bind to that file.*/
    
	server_sockaddr.sun_family = AF_UNIX;
    strcpy(server_sockaddr.sun_path, SERVER_PATH); 
    len = sizeof(server_sockaddr);
    
    unlink(SERVER_PATH);
    rc = bind(server_sock, (struct sockaddr *) &server_sockaddr, len);
    if (rc == -1){
        printf("BIND ERROR: 2\n");
        close(server_sock);
        exit(1);
    }
    
    /*********************************/
    /* Listen for any client sockets */
    /*********************************/
    rc = listen(server_sock, backlog);
    if (rc == -1){ 
        printf("LISTEN ERROR: 3\n");
        close(server_sock);
        exit(1);
    }

	return server_sock;
}
