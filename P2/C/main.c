#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>


int
main(int argc, char *argv[])
{
	char* home = getenv("HOME");
	while (1) {
		//hold 512 chars
		char str[512];
		char* path = "/bin/";
		
		
		printf("mysh> ");
		fgets(str, 512, stdin);
		
		//how many args can we handle?
		char *argv[4];
		
		char tmp[512];
		char *ptr;
		
		//check for built in cases
		strcpy(tmp, str);
		ptr = strtok(tmp, " \n\0\t");
		//check for exit
		if (strcmp(ptr, "exit") == 0) {
			exit(0);
		//check for cd
		} else if (strcmp(ptr, "cd") == 0) {
			char path[512];
			char *pathPtr;
			pathPtr  = path;
			strcpy(pathPtr, home);
			strcpy(tmp, str);
			ptr = strtok(NULL, " \n\0\t");
			if (ptr != NULL) {
				if (getcwd(pathPtr, 512) == NULL) {
					printf("Error: cwd!\n");
				}
				strcat(pathPtr, ptr);
			}
			if (chdir(pathPtr) != 0) {
				printf("Error changing directory!\n");
			}
			pathPtr = NULL;
		} else if (strcmp(ptr, "pwd") == 0) {
			char cwd[512];
			char *cwdPtr;
			cwdPtr = cwd;
			if (getcwd(cwdPtr, 512) == NULL) {
				printf("Error: cwd!\n");
			}
			printf("%s\n", cwdPtr);
		} else {
			strcpy(tmp, path);
			strcat(tmp, str);
			
			ptr = strtok(tmp, " \n\0\t");
			int argIndex = 1;
			argv[0] = ptr;
			
			while (ptr != NULL) {
				ptr = strtok(NULL, " \n\0\t");
				argv[argIndex] = ptr;
				argIndex++;
			}
			
			argv[argIndex] = NULL;
			
			//fork
			int rc = fork();
			
			if (rc == 0) {
			execvp(argv[0], argv);
			
			}
			else if (rc > 0) {
				(void) wait(NULL);
				printf("I am a parent.\n");
			}
		}
	}
	return 0;
}