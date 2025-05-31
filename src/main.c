#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <dirent.h>

#define SIZE 1028

void exec_echo(char *in);
void parse_args(char *input, char *args[], int *arg_count);
int exec_pwd(char *cwd);

void exec_echo(char *in)
{
    //skip 5 index
	char *text = in + 5;
    
    bool inside_quotes = false;
    char quote_char = 0;
    bool space_needed = false;
	bool backslash = false;
    while (*text != '\0')
	{
        if (!backslash && !inside_quotes && (*text == '\'' || *text == '\"'))
		{
            //start of quote
            inside_quotes = true;
            quote_char = *text;
            text++;
            continue;
        }

        if (!backslash && inside_quotes && *text == quote_char)
		{
            //end of quote
            inside_quotes = false;
            text++;
            space_needed = true;
            continue;
        }

		if (*text == '\\' && !escape)
		{
			// \ to next \ baclslash
			backslash = true;
			text++;
			continue;
		}

        if (!inside_quotes && *text == ' ' && !backslash)
		{
            //spaces between arguments
            if (space_needed)
			{
                printf(" ");
                space_needed = false;
            }
            //skip extra spaces
            while (*text == ' ') text++;
            continue;
        }

        // Print the character
        printf("%c", *text);
        text++;
        space_needed = true;
		backskash = false;
    }
    printf("\n");
}


void parse_args(char *input, char *args[], int *arg_count)
{
    *arg_count = 0;
    bool inside_quotes = false;
    char quote_char = 0;//for '/"
    char *start = input;
    
    while (*input)
	{
        if (!inside_quotes && (*input == '\'' || *input == '\"')) 
		{
            //start of quote
            inside_quotes = true;
            quote_char = *input;
            start = input + 1;  
		} 
        else if (inside_quotes && *input == quote_char) 
		{
            //end of quote
            inside_quotes = false;
            args[(*arg_count)++] = start;
            *input = '\0';  
			start = input + 1;//for next argument
        }
        else if (!inside_quotes && *input == ' ')
		{
            //argument separator for outside quote argument
            if (start != input)
			{
                args[(*arg_count)++] = start;
                *input = '\0';  
            }
            start = input + 1;//for next argument
        }
        input++;
    }
    
    //if input didnot end with space
    if (start != input)
	{
        args[(*arg_count)++] = start;
    }
    args[*arg_count] = NULL;//if not null it might crash
}

int exec_pwd(char *cwd)
{
    getcwd(cwd, SIZE);
    printf("%s\n", cwd);
	return 0;
}

int main(int argc, char *argv[])
{
    // Flush after every printf
    setbuf(stdout, NULL);
    char command[100];
    char shell_cmd[][100] = {"echo", "exit", "type","history","pwd","cd"};
    int n = 6;//number of commands

    while(1)
	{
        printf("$ ");
        char input[SIZE];
        fgets(input, SIZE, stdin);
        input[strcspn(input, "\n")] = '\0';
        char curr_wrk_dir[SIZE];//current directory variable
        int found_command = 0;

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
            exec_echo(input);
            continue;
        }

        //check for pwd
        if(strcmp(command, "pwd") == 0)
		{
            exec_pwd(curr_wrk_dir);    
            continue;
        }
        
        //check for cd
        if(strcmp(command, "cd") == 0)
		{
            char *cd_p = input + 3;
            while (*cd_p == ' ') cd_p++;
            
			//~ for home directory
            if(strcmp(cd_p, "~") == 0)
			{
                chdir(getenv("HOME"));
            } 
			else
			{
                if(chdir(cd_p) == -1)
                    printf("cd: %s: No such file or directory\n", cd_p);
            }
            found_command = 1;
            continue;
        }

        //check for type commands
        if(strcmp(command,"type") == 0)
		{
            //extract command
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
                    char *dir = strtok(path_copy, ":");
                    int found_path = 0;

                    while(dir != NULL)
					{
                        //construct proper path with separator
                        char full_path[512];
                        snprintf(full_path, sizeof(full_path), "%s/%s", dir, type_command);
                        //check if executable
                        if(access(full_path, X_OK) == 0)
						{
                            printf("%s is %s\n", type_command, full_path);
                            found_path = 1;
                            break;
                        }
                        dir = strtok(NULL, ":");
                    }
                    //free strdup()
                    free(path_copy);

                    if(!found_path)
                        printf("%s: not found\n", type_command);
                }
                else
                    printf("%s: not found\n", type_command);
            }
            continue;
        }

        //external commands
		char *args[10];
        int arg_count = 0;
        
        if(!found_command)
		{
            parse_args(input, args, &arg_count);
            
            //check for executable in PATH
            char *exe_path = NULL;
            char *path = getenv("PATH");
            if(path != NULL)
			{
                char *path_copy = strdup(path);
                char *direc = strtok(path_copy, ":");
                
                while(direc != NULL)
				{
                    char full_path[512];
                    snprintf(full_path, sizeof(full_path), "%s/%s", direc, args[0]);
                    //check if executable
                    if(access(full_path, X_OK) == 0)
					{
						//store path
                        exe_path = strdup(full_path);
                        break;
                    }
                    direc = strtok(NULL, ":");
                }
                free(path_copy);    
            }

            if(exe_path != NULL)
			{
				//create child
                pid_t pid = fork();  
				if(pid == 0)
				{
                    //child process executes program
                    execv(exe_path, args);
                    perror("execv");                      
                    exit(1);
                }
                else if(pid > 0)
				{
                    //parent process wait for child
                    wait(NULL);
                }
                else
                    perror("fork()");  
                free(exe_path); 
            }
            else
                printf("%s: command not found\n", args[0]);
        }
    }
    return 0;
}
