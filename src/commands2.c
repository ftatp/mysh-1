#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
//#include <sys/ipc.h>
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
#define DATA "Hello from server"
#define SHMSZ 27


static struct built_in_command built_in_commands[] = {
	{ "cd", do_cd, validate_cd_argv },
	{ "pwd", do_pwd, validate_pwd_argv },
	{ "fg", do_fg, validate_fg_argv }
};

void* put_output_to_next_com(void* p_sock){//server side
    int server_sock, len, rc;
	int client_sock;
    int bytes_rec = 0;
    struct sockaddr_un server_sockaddr;
    struct sockaddr_un client_sockaddr;     

    char buf[256];
    char sendback_buf[256];

    int backlog = 10;
    memset(&server_sockaddr, 0, sizeof(struct sockaddr_un));
    memset(&client_sockaddr, 0, sizeof(struct sockaddr_un));
    memset(buf, 0, 256);                
    
    /**************************************/
    /* Create a UNIX domain stream socket */
    /**************************************/
    server_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_sock == -1){
        printf("SOCKET ERROR: 1\n");
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
    //printf("socket listening...\n");

	//Make the client to connect
	//while(*shm >= 0);
    
	/*********************************/
    /* Accept an incoming connection */
    /*********************************/

	//printf("Accepting...\n");
    client_sock = accept(server_sock, (struct sockaddr *) &client_sockaddr, &len);
    if (client_sock == -1){
        printf("ACCEPT ERROR: 4\n");
        close(server_sock);
        close(client_sock);
        exit(1);
    }
    
    /****************************************/
    /* Get the name of the connected socket */
    /****************************************/
    len = sizeof(client_sockaddr);
    rc = getpeername(client_sock, (struct sockaddr *) &client_sockaddr, &len);
    if (rc == -1){
        printf("GETPEERNAME ERROR: 5\n");
        close(server_sock);
        close(client_sock);
        exit(1);
    }
    else {
      //  printf("Client socket filepath: %s\n", client_sockaddr.sun_path);
    }

	//int infd;
	dup2(client_sock, STDIN_FILENO);
	//*p_sock = client_sock;

    /************************************/
    /* Read and print the data          */
    /* incoming on the connected socket */
    /************************************/
    //printf("waiting to read...\n");
//    bytes_rec = recv(client_sock, buf, sizeof(buf), 0);
//    if (bytes_rec == -1){
//        printf("RECV ERROR: 6\n");
//        close(server_sock);
//        close(client_sock);
//        exit(1);
//    }
//    else {
//        //printf("DATA RECEIVED = %s\n", buf);
//    }
//    
//	//strcpy(additional_com, buf);
//
//    /******************************************/
//    /* Send data back to the connected socket */
//    /******************************************/
//    strcpy(sendback_buf, buf);      
//    //printf("Sending data...\n");
//    rc = send(client_sock, sendback_buf, strlen(sendback_buf), 0);
//    if (rc == -1) {
//        printf("SEND ERROR: 7");
//        close(server_sock);
//        close(client_sock);
//        exit(1);
//    }   
//    else {
//		//strcpy(ary, sendback_buf);
//        //printf("Data sent!\n");
//    }
//    
    /******************************/
    /* Close the sockets and exit */
    /******************************/
    close(server_sock);
    close(client_sock);

//	return;
}


/////////////////////////////////////////////////////////////////////////////////////////////

void* get_input_from_pre_com(void* p_com){//client side
    int client_sock, rc, len;
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
	int tempoutfd = dup(STDOUT_FILENO);
	dup2(client_sock, STDOUT_FILENO);

	execute(com);
	
	close(client_sock);
	dup2(tempoutfd, STDOUT_FILENO);
    
    // Close the socket and exit.
    close(client_sock);
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

int com_idx = 0;


int evaluate_command(int n_commands, struct single_command (*commands)[512])
{
	struct single_command* com = (*commands);	
	int tempfd;
	int fd; 	
	pid_t pid_for_pipe;
	int needs_pipe = 0;
	int ret = 0;
	int len;

	pthread_t client_thread[32];

	if(n_commands < 2){
		ret = execute(com);
		if(ret == 1)
			return 1;
	}
	else{
		int server_sock = setup_server();
		int client_sock;
    	struct sockaddr_un client_sock_addr; 
		//char send_buf[256];
		pthread_t client_thread[32];

		len = sizeof(struct sockaddr_un);
		memset(&client_sock_addr, 0, len);

		pthread_create(&client_thread[com_idx], NULL, get_input_from_pre_com, com + com_idx);
		
		client_sock = accept(server_sock, (struct sockaddr *) &client_sock_addr, &len);
		if (client_sock == -1){
			printf("ACCEPT ERROR: 4\n");
			close(server_sock);
			close(client_sock);
			exit(1);
		}   
		pthread_join(client_thread[com_idx++], NULL);

		int tempinfd = dup(STDIN_FILENO);
		dup2(client_sock, STDIN_FILENO);
		execute(com + com_idx);
		close(client_sock);
		dup2(tempinfd, STDIN_FILENO);
	}


//	evaluate();
	return 0;
}


void evalute(){


}

int execute(struct single_command* com){
	
	struct passwd* pw = getpwuid(getuid());
	char* homepath = pw->pw_dir;
	
	//printf("home: %s\n", homepath);
	
	for(int i = 0; i < com->argc; i++){
		if(com->argv[i][0] == '~' && ((com->argv[i][1] == '\0') || (com->argv[i][1] == '/'))){
			char* temp_arg = malloc(sizeof(char) * strlen(com->argv[i]));
			if(com->argv[i][1] == '\0')
				strcpy(temp_arg, com->argv[i] + 1);
			else
				strcpy(temp_arg, com->argv[i] + 2);

			strcpy(com->argv[i], homepath);
			com->argv[i][strlen(homepath)] = '/';
			strcpy((com->argv[i] + strlen(homepath) + 1), temp_arg); 

			//printf("new arg: %s\n", com->argv[i]);
			free(temp_arg);
		}
	}

	int built_in_pos = is_built_in_command(com->argv[0]);
	if (built_in_pos != -1) {
		if (built_in_commands[built_in_pos].command_validate(com->argc, com->argv)) {
			if (built_in_commands[built_in_pos].command_do(com->argc, com->argv) != 0) {
				fprintf(stderr, "%s: Error occurs\n", com->argv[0]);
			}
		} 
		else {
			fprintf(stderr, "%s: Invalid arguments\n", com->argv[0]);
			return -1;
		}
	} 
	else if (strcmp(com->argv[0], "") == 0) {
		return 0;
	} 
	else if (strcmp(com->argv[0], "exit") == 0) {
		return 1;
	} 
	else {
		pid_t pid_for_exe;
		pid_for_exe = fork();

		if(pid_for_exe < 0){
			return -1;
			fprintf(stderr, "Fork Failed");
		}
		else if(pid_for_exe == 0){

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
				char* command_path = (char *)malloc(strlen(path[j]) * sizeof(char) + strlen(com->argv[0]));
				strcpy(command_path, path[j]);
				strcat(command_path, com->argv[0]);
				execv(command_path, com->argv);
				free(command_path);
			}
			fprintf(stderr, "Command not found\n");
			exit(0);
		}
		else{
			//for background process, need to be modified to not wait when & comes in the end of the command
			wait(NULL);
			//printf("Child for exe complete\n");
		}

	}
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
