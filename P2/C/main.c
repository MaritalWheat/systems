#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

int
main(int argc, char *argv[])
{
	printf("1: %d\n", (int) getpid());
	int rc = fork();
	printf("2: %d [rc:%d]\n", (int) getpid(), rc);
	
	if (rc == 0) {
		//child
		
		char *argv[4]; //stack allocated
		argv[0] = strdup("/bin/ls");
		argv[1] = strdup("-l");
		argv[2] = NULL;
		
		execvp(argv[0], argv);
		printf("Exec Failed\n");
		printf("I am a child.\n");
	} else if (rc > 0) {
		//parent
		(void) wait(NULL);
		printf("I am a parent.\n");
	} else {
		printf("Error in calling fork().\n");
	}
	
	return 0;
}