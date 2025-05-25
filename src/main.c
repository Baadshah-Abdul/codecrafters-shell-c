#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
  // Flush after every printf
	setbuf(stdout, NULL);
	char command[100];
	char shell_cmd[][100] = {"echo", "exit", "type"};
	int n = 3;

while(1)
{
	printf("$ ");
	char input[100];
	fgets(input, 100, stdin);
	input[strcspn(input, "\n")] = '\0';

	//continue even if empty
	if(strlen(input) == 0)
		continue;

	//extract command from input
	sscanf(input, "%s", command);

	//exit 0
	if(strcmp(command,"exit") == 0)
	return 0;

	//check for echo
	if(strcmp(command,"echo") == 0)
		{
			printf("%s\n", input + 5);
			continue;
		}

	//check for type commands
	if(strcmp(command,"type") == 0)
	{
		//extract command for path
		char type_command[100] = {};
		char *command_ptr = input + 5;
		while(*command_ptr == ' ')
			command_ptr++;
		sscanf(command_ptr, "%s", type_command);
		
		//check for builtin
		int found_type = 0;
		for(int i = 0; i < n; i++)
		{
			if(strcmp(type_command, shell_cmd[i]) == 0)
			{
				printf("%s is a shell builtin\n", type_command);
				found_type = 1;
				break;
			}
		}
 
	//check PATH if not builtin
		if(!found_type)
		{
			char *path = getenv("PATH");
			if(path != NULL)
			{
				char *path_copy = strdup(path);
				char *direc = strtok(path_copy, ":");
				int found_path = 0;

				while(direc != NULL)
				{
					//construct proper path with separator
					char full_path[512];
					snprintf(full_path, sizeof(full_path), "%s/%s", direc, type_command);
					//check if executable
					if (access(full_path, X_OK) == 0)
					{
						printf("%s is %s\n", type_command, full_path);
						found_path = 1;
						break;
					}
					direc = strtok(NULL, ":");
				}
				//free space from strdup()
				free(path_copy);

				if(!found_path)
					printf("%s: not found\n", type_command);
			}
			else
				printf("%s: not found\n", type_command);
		}
		continue;
	}

    //Handle external command
	char *args[10];
	int arg_count = 0;
	char *token = strtok(input, " ");

	//extract arguments
	while (token != NULL && arg_count < 10)
		{
			args[arg_count++] = token;
			token = strtok(NULL, " ");
		}
	args[arg_count] = NULL; //NULL terminate argument list
		
	//check for executable in PATH
	char *exe_path = NULL;
        char *path = getenv("PATH");
        if(path != NULL) {
            char *path_copy = strdup(path);
            char *direc = strtok(path_copy, ":");
            
            while(direc != NULL) {
                char full_path[512];
                snprintf(full_path, sizeof(full_path), "%s/%s", direc, args[0]);
				//check if executable
                if(access(full_path, X_OK) == 0) {                      exe_path = strdup(full_path);   // Store found path
                    break;
                }
                direc = strtok(NULL, ":");
            }
            free(path_copy);	
			}

			if(exe_path != NULL) 
			{
				pid_t pid = fork();  // Create child process
				if(pid == 0) 
				{
                // Child process - execute program
					execv(exe_path, args);
					perror("execv");  					exit(1);
				}
				else if(pid > 0) {
					// Parent process - wait for child
					wait(NULL);
				}
				else
					perror("fork");  
                free(exe_path); 
			}
			else
				printf("%s: command not found \n", args[0]);
}

			return 0;
}
