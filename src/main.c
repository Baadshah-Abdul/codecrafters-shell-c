#include <stdio.h>
#include <stdlib.h>
#include <string.h>


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
		char *type_command = input + 5;
		char *space = strchr(type_command, ' ');
		//NULl terminate the command at the first space
		if(space != NULL)
			*space = '\0';
		
		int found = 0;
		for(int i = 0; i < n; i++)
		{
			if(strcmp(type_command, shell_cmd[i]) == 0)
			{
				printf("%s is a shell builtin\n", type_command);
				found = 1;
				break;
			}
		}
		if(!found)
		printf("%s: not found\n", type_command);
		continue;
	}

	printf("%s: not found\n", input);
}

  return 0;
}
