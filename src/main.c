#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main(int argc, char *argv[]) {
  // Flush after every printf
  setbuf(stdout, NULL);
   char check[100];
   char out[100];
while(1)
{
  // Uncomment this block to pass the first stage
  printf("$ ");

  // Wait for user input
  char input[100];
  fgets(input, 100, stdin);

 //remove the newline character fromthe input if it exists
  input[strcspn(input, "\n")] = '\0';

  
  //check if exit is there
  if(strcmp(input,"exit 0") == 0)
  return 0;

//check for echo
	strncpy(check, input, 4);
	int echo_start = 5;
	if (strcmp(check,"echo") == 0)
		{
			printf("%s\n", input + echo_start);
		}
	else
	printf("%s: command not found\n", input);
}

  return 0;
}
