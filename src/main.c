#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <dirent.h>
#include <fcntl.h>//file flag
#include <sys/stat.h>//file permissions
#include <sys/types.h>
#include <readline/readline.h>
#include <readline/history.h>


#define SIZE 512

//builtin commands for autocompletion
const char *builtin[] = {"echo", "exit", "type","history","pwd","cd", NULL};

char *command_generator(const char *text, int state);
char **command_completion(const char *text, int start, int end);
bool exec_redirect(const char *in);
void exec_echo(char *in);
void parse_args(char *input, char *args[], int *arg_count);
void exec_pwd(char *cwd);

//generate matching builtin command suggestion
char *command_generator(const char *text, int state)
{
	static int index = 0, len;
	static DIR *dir = NULL;
	static struct dirent *entry;
	static char *path_copy = NULL;
	static char *path_token;

	if (!state)
	{
		index = 0;
		len = strlen(text);

		path_tokens = NULL;
		
		//prepare PATH directory
		if (path_copy) 
			free(path_copy);
		path_copy = strdup(getenv("PATH"));
		path_token = strtok(path_copy, ":");
	}

	//search for builtin match
	while ((name = builtin[index++]))
	{
		//match little command
		if (strncmp(name, text, len) == 0)
		{
			char *match = malloc(strlen(name) + 2);
			sprintf(match, "%s", name);
			return match;
		}
	}
	 // Search through PATH directories
    while (1)
    {
        if (!dir)
        {
            if (!path_token) break; // no more directories
            dir = opendir(path_token);
            path_token = strtok(NULL, ":");
        }

        if (!dir) continue;

        while ((entry = readdir(dir)) != NULL)
        {
            if (strncmp(entry->d_name, text, len) == 0)
            {
                // Construct full path
                char full_path[SIZE];
                snprintf(full_path, sizeof(full_path), "%s/%s", path_token, entry->d_name);
                if (access(full_path, X_OK) == 0)
                {
                    char *match = malloc(strlen(entry->d_name) + 2);
                    sprintf(match, "%s ", entry->d_name);
                    closedir(dir);
                    dir = NULL;
                    return match;
                }
            }
        }
        closedir(dir);
		dir = NULL;
    }

    if (path_copy) 
	{
        free(path_copy);
        path_copy = NULL;
    }
	return NULL;
}

char **command_completion(const char *text, int start, int end)
{
	//prevent filename completion
	rl_attempted_completion_over = 1;
	//return completed name to builtin_generator
	return rl_completion_matches(text, command_generator);
}


bool exec_redirect(const char *in)
{
	const char *ops[] = {"1>>","2>>",">>","2>","1>",">"};
	const int fds[] = {STDOUT_FILENO, STDERR_FILENO, STDOUT_FILENO, STDERR_FILENO, STDOUT_FILENO, STDOUT_FILENO};
	const bool appends[] = {true, true, true, false, false, false};
	
	char *redirect = NULL;
	int redirect_fd = STDOUT_FILENO;
	bool append_mode = false;
	int op_index = -1;

	//check redirection operator
    for (int i = 0; i < 6; i++)
	{
		redirect = strstr(in, ops[i]);
		if (redirect)
		{
			redirect_fd = fds[i];
			append_mode = appends[i];
			op_index = i;
			break;
		}
	}
	if(!redirect)
		return true;
	
	//get filename
	char *filename = redirect + strlen(ops[op_index]);

	while(*filename == ' ' )
		filename++;
	
	//check if directory exist
	char *last_slash = strrchr(filename, '/');
    if (last_slash != NULL)
	{
        char dir_path[SIZE];
        strncpy(dir_path, filename, last_slash - filename);
        dir_path[last_slash - filename] = '\0';

       	if (access(dir_path, F_OK) != 0)
			return false;
    }

	int flags = O_WRONLY|O_CREAT;
	flags |= append_mode ? O_APPEND : O_TRUNC;
	int fd = open(filename, flags, 0644);
	if (fd < 0)
		return false;
	
	//redirect the appropiate stream
	dup2(fd, redirect_fd);
	close(fd);

	return true;
}


void exec_echo(char *in)
{
    //skip 5 index
	char *text = in + 5;
    bool space_needed = false;
	bool backslash = false;
	bool in_sq = false;
	bool in_dq = false;

    while (*text != '\0')
	{
		if (!backslash)
		{
			//for double quote
            if (*text == '\"' && !in_sq)
			{  
                in_dq = !in_dq;
                text++;
                continue;
            }

			//for single quote
            if (*text == '\'' && !in_dq) 
			{
				in_sq = !in_sq;
                text++;
                continue;
            }
		}
        
		//for \ inside ' '
		if (*text == '\\' && !in_sq && !backslash)
		{
			backslash = true;
			text++;
			continue;
		}
        
        if (!in_sq && !in_dq && *text == ' ' && !backslash)
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

        printf("%c", *text);
        text++;
        space_needed = true;
		backslash = false;
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

void exec_pwd(char *cwd)
{
    getcwd(cwd, SIZE);
    printf("%s\n", cwd);
}



int main(int argc, char *argv[])
{
    // Flush after every printf
    setbuf(stdout, NULL);
    char command[100];
    char shell_cmd[][100] = {"echo", "exit", "type","history","pwd","cd"};
    int n = 6;//number of commands
	char input[SIZE];
    
	while(1)
	{
		//register autocompletion function
		rl_attempted_completion_function = command_completion;

		//replace fget
		char *line = readline("$ ");
		
		//even if empty prompt again
		if (!line || strlen(line) == 0)
		{
			free(line);
			continue;
		}

		//for UP arrow 
		add_history(line);

		//copy to input buffer and free readline buffer
		strncpy(input, line, SIZE - 1);
		free(line);

        input[strcspn(input, "\n")] = '\0';
        char curr_wrk_dir[SIZE];//current directory variable
        int found_command = 0;
#if 0
        //continue even if empty
        if(strlen(input) == 0)
            continue;
#endif
        //extract command from input
        sscanf(input, "%s", command);

        //exit 0
        if(strcmp(command,"exit") == 0)
            return 0;
        	
		//check for redirection (> & 1>)
		
		// Check for output redirection (> or 1> or 2>)
		if (strstr(input, ">") != NULL)	
		{
			// Fork a child process to handle redirection
			pid_t pid = fork();
			if (pid == 0)
			{
				char *args[512];
				int args_count = 0;
				char cmd_copy[SIZE];
				strcpy(cmd_copy, input);
				
				//(1) perform redirection with ful input
				if (!exec_redirect(cmd_copy))
					_exit(1);
								
				//find redirection operator in input
				char *redirect;
				const char *ops[] = {"1>>","2>>",">>","2>","1>",">"};
				for (int i = 0; i < 6; i++)
				{
					redirect = strstr(input, ops[i]);
					if(redirect)
					{
						*redirect = '\0';
						break;
					}
				}
		
				parse_args(input, args, &args_count);
				
				//run input command
				execvp(args[0], args);

				perror(args[0]);
				//if exec fails
				_exit(1);
			}
			//wiat for child
			waitpid(pid, NULL, 0);
			continue;
		}


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
