#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>

#include "commands.h"
#include "built_in.h"

static struct built_in_command built_in_commands[] = {
  { "cd", do_cd, validate_cd_argv },
  { "pwd", do_pwd, validate_pwd_argv },
  { "fg", do_fg, validate_fg_argv }
};

static int is_built_in_command(const char* command_name)
{
  static const int n_built_in_commands = sizeof(built_in_commands) / sizeof(built_in_commands[0]);

  for (int i = 0; i < n_built_in_commands; ++i) {
    if (strcmp(command_name, built_in_commands[i].command_name) == 0) {
      return i;
    }
  }

  return -1; // Not found
}

void *t_function_server(void* data);
//void path_resolution(int n_commands, struct single_command (*commands)[512]);
/*
 * Description: Currently this function only handles single built_in commands. You should modify this structure to launch process and offer pipeline functionality.
 */
int evaluate_command(int n_commands, struct single_command (*commands)[512])
{
  if (n_commands > 0) {
    struct single_command* com = (*commands);
    struct stat buf;
    int status;
    char buffer[1024];
    	
    assert(com->argc != 0);

   // path_resolution(n_commands, commands);

    stat(com->argv[0], &buf);
	
    int built_in_pos = is_built_in_command(com->argv[0]);
    if (built_in_pos != -1) {
      if (built_in_commands[built_in_pos].command_validate(com->argc, com->argv)) {
        if (built_in_commands[built_in_pos].command_do(com->argc, com->argv) != 0) {
          fprintf(stderr, "%s: Error occurs\n", com->argv[0]);
        }
      } else {
        fprintf(stderr, "%s: Invalid arguments\n", com->argv[0]);
        return -1;
      }
    } else if (strcmp(com->argv[0], "") == 0) {
      return 0;
    } else if (strcmp(com->argv[0], "exit") == 0) {
      return 1;
    } else if (n_commands > 1){
		pid_t pid;
		pthread_t p_thread;
		int client_sock;
		struct sockaddr_un server_addr;

		pthread_create(&p_thread, NULL, t_function_server, (void*)com);
	
		client_sock = socket(AF_UNIX, SOCK_STREAM, 0);
		memset(&server_addr, 0, sizeof(server_addr));
		server_addr.sun_family = AF_UNIX;
		strcpy(server_addr.sun_path, "/tmp/mysh-1");
		
		while(connect(client_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)));
		
		pid = fork();
		
		if(pid == 0){
			dup2(client_sock, 0);
			execv(com[1].argv[0], com[1].argv);
		}
		else if(pid > 0){
			wait(&status);
		}
	} else if(S_ISREG(buf.st_mode)){
      pid_t pid;
      bool background = false;

	  if(strcmp(com->argv[(com->argc)-1], "&") == 0){
		  background = true;
		  com->argv[(com->argc)-1] = NULL;
	  }

      pid = fork();
	  if(pid == 0){
          execv(com->argv[0], com->argv);
      }
      else if(pid > 0){
		  if(background == false){
			  wait(&status);
			  return 0;
		  }
		  printf("%d\n", pid);
      }      
    }
	else {
      fprintf(stderr, "%s: command not found\n", com->argv[0]);
      return -1;
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

void *t_function_server(void *data) {
	int server_sock, client_sock;
	int client_addr_size;
	int status;
	pid_t pid;

	struct sockaddr_un server_addr;
	struct sockaddr_un client_addr;

	if(access("/tmp/mysh-1", F_OK) == 0)
		unlink("/tmp/mysh-1");

	server_sock = socket(AF_UNIX, SOCK_STREAM, 0);
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sun_family = AF_UNIX;
	strcpy(server_addr.sun_path, "/tmp/mysh-1");

	bind (server_sock, (struct sockaddr*)&server_addr,sizeof(server_addr));

	while(1){
		if(listen(server_sock,0) == -1){
			printf("listen failed\n");
			exit(1);
		}

		client_addr_size = sizeof(client_addr);
		client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_addr_size);

		if(client_sock == -1){
			printf("accept failed\n");
			exit(1);
		}
		pid = fork();
		if(pid == 0){
			dup2(client_sock, 1);
			execv((*(struct single_command *)data).argv[0], (*(struct single_command*)data).argv);
		}
		if(pid > 0){
			wait(&status);
		}
		close(client_sock);
	}
}

/*void path_resolution(int n_commands, struct single_command (*commands)[512]){
	char *abs_path[5] = {"/usr/local/bin/", "/usr/bin/", "/bin/", "/usr/sbin/", "/sbin/"};
	char *tmp;

	for(int i =0; i < n_commands; i++){
	    if(commands[i]->argv[0][0] != '/'){
		if(!strcmp(commands[i]->argv[0], "ls")){
		    tmp = (char *)realloc(tmp, sizeof(commands[i]->argv[0])+sizeof(abs_path[2]));
		    strcpy(tmp, abs_path[2]);
		    strcat(tmp, commands[i]->argv[0]);
		    commands[i]->argv[0] = (char *)realloc(commands[i]->argv[0], sizeof(tmp));
		    strcpy(commands[i]->argv[0], tmp);
		}
		if(!strcmp(commands[i]->argv[0], "cat")){
		    tmp = (char *)realloc(tmp, sizeof(commands[i]->argv[0])+sizeof(abs_path[2]));
		    strcpy(tmp, abs_path[2]);
		    strcat(tmp, commands[i]->argv[0]);
		    commands[i]->argv[0] = (char *)realloc(commands[i]->argv[0], sizeof(tmp));
		    strcpy(commands[i]->argv[0], tmp);
		}
		if(!strcmp(commands[i]->argv[0], "vim")){
		    tmp = (char *)realloc(tmp, sizeof(commands[i]->argv[0])+sizeof(abs_path[1]));
		    strcpy(tmp, abs_path[1]);
		    strcat(tmp, commands[i]->argv[0]);
		    commands[i]->argv[0] = (char *)realloc(commands[i]->argv[0], sizeof(tmp));
		    strcpy(commands[i]->argv[0], tmp);
		}
		if(!strcmp(commands[i]->argv[0], "grep")){
		    tmp = (char *)realloc(tmp, sizeof(commands[i]->argv[0])+sizeof(abs_path[2]));
		    strcpy(tmp, abs_path[2]);
		    strcat(tmp, commands[i]->argv[0]);
		    commands[i]->argv[0] = (char *)realloc(commands[i]->argv[0], sizeof(tmp));
		    strcpy(commands[i]->argv[0], tmp);
		} 
	    }
	}
	

}*/
