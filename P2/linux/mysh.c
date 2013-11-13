#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>


int
main(int argc, char *argv[])
{
	FILE *fd;
	char error_message[30] = "An error has occurred\n";
	char cmdLine[516];
	char *cmdLinePtr;
	char *home = getenv("HOME");
	
	// check for batch mode
	int batch = 0;
	if (argv[1] != NULL) {
		batch = 1;
		fd = fopen(argv[1], "r");
		if (fd == NULL) {
			// error opening file
			write(STDERR_FILENO, error_message, strlen(error_message));
			exit(1);
		}
		cmdLinePtr = cmdLine;
		if (argv[2] != NULL) {
			write(STDERR_FILENO, error_message, strlen(error_message));
			exit(1);
		}
	}
	
	
	while (1) {
		//hold 512 chars
		char str[516];
		
		if (batch == 1) {
			if (fgets(cmdLine, 516, fd) != NULL) {
				cmdLinePtr = strtok(cmdLinePtr, "\n\0\t");
				strcat(cmdLinePtr, "\n");
				write(STDOUT_FILENO, cmdLinePtr, strlen(cmdLinePtr));
				strcpy(str, cmdLinePtr);
			} else {
				strcpy(str, "exit");
			}
		} else {
			write(STDOUT_FILENO, "mysh> ", strlen("mysh> "));
			fgets(str, 516, stdin);
			if (feof(stdin)) {
      			exit(0);
    		}
		}
		
		
		//how many args can we handle?
		char *argv[4];
		
		char tmp[516];
		char *ptr;
		
		int size = strlen(str);
		
		//check for built in cases
		strcpy(tmp, str);
		ptr = strtok(tmp, " \n\0\t");
		//check for exit
		if (size >= 512 && (char)str[512] != '\n') {
			//printf("EOL");
			write(STDERR_FILENO, error_message, strlen(error_message));
		}
		else if (strcmp(ptr, "exit") == 0) {
			//check for bad args
			ptr = strtok(NULL, " \n\0\t");
			if (ptr != NULL) {
				write(STDERR_FILENO, error_message, strlen(error_message));
			}
			exit(0);
		//check for cd
		} else if (strcmp(ptr, "cd") == 0) {
			char path[516];
			char *pathPtr;
			pathPtr  = path;
			strcpy(pathPtr, home);
			strcpy(tmp, str);
			ptr = strtok(NULL, " \n\0\t");
			if (ptr != NULL) {
				if (chdir(ptr) != 0) {
					write(STDERR_FILENO, error_message, strlen(error_message));
				}
			} else {
				if (chdir(pathPtr) != 0) {
					write(STDERR_FILENO, error_message, strlen(error_message));
				}
			}
			pathPtr = NULL;
		//check for pwd
		} else if (strcmp(ptr, "pwd") == 0) {
			ptr = strtok(NULL, " \n\0\t");
			if (ptr != NULL) {
				write(STDERR_FILENO, error_message, strlen(error_message));
			} else {
				char cwd[516];
				char *cwdPtr;
				cwdPtr = cwd;
				if (getcwd(cwdPtr, 516) == NULL) {
					//check for error
				}
				strcat(cwdPtr, "\n");
				write(STDOUT_FILENO, cwdPtr, strlen(cwdPtr));
			}
		//check for wait
		} else if (strcmp(ptr, "wait") == 0) {
			ptr = strtok(NULL, " \n\0\t");
			if (ptr != NULL) {
				write(STDERR_FILENO, error_message, strlen(error_message));
			} else {
				int pid;
				int stat;
				while ( (pid = waitpid(-1, &stat, WNOHANG)) > 0) {
					//printf("child %d terminated\n", pid);
				}
        	}
		} else {
		    //check for redirection
		    char outFile[516];
		    char *outFilePtr;
		    char prevToken[516];
		    char *prevTokenPtr;
		    
		    outFilePtr = outFile;
			prevTokenPtr = prevToken;
			
			strcpy(tmp, str);
			int redirect = 0;
			
			char subString[516];
			char *subStringPtr;
			subStringPtr = subString;
			
			
			int i;
			int j;
			for (i = 0; i < (sizeof(tmp) / sizeof(char)); i++) {
				//check for redirect
				if ((char)tmp[i] == '>') {
					redirect = 1;
					subString[i] = '\0';
					i++;
					int k = 0;
					for (j = i; j < (sizeof(tmp) / sizeof(char)); j++) {
						outFile[k] = tmp[j];
						k++;
					}
					//make sure to null terminate substring!!
					break;
				}
				subString[i] = tmp[i];
			}
			
			ptr = strtok(outFilePtr, " \n\0\t");
			if (ptr != NULL && redirect == 1) {
				ptr = strtok(NULL, " \n\0\t");
				if (ptr != NULL) {
					//printf("pointer: %s\n", ptr); 
					if (strcmp(ptr, "&") != 0) {
						write(STDERR_FILENO, error_message, strlen(error_message));
					}
				}
			}
			
			//check for background
			strcpy(tmp, str);
			ptr = strtok(tmp, " \n\0\t");
			int background = 0;	
			while (ptr != NULL) {
				strcpy(prevTokenPtr, ptr);
				ptr = strtok(NULL, " \n\0\t");
				if (ptr != NULL) {
					if (strcmp(ptr, "&") == 0 && background == 0) {
						background = 1;
					}
				}	
			}
			

			strcpy(tmp, subString);
			
			ptr = strtok(tmp, " \n\0\t");
			int argIndex = 1;
			argv[0] = ptr;
			
			while (ptr != NULL) {
				ptr = strtok(NULL, " \n\0\t");
				if (ptr != NULL) {
					argv[argIndex] = ptr;
					argIndex++;
				}
			}
			
			argv[argIndex] = NULL;
			
			//fork
			int rc = fork();
			
			if (rc == 0) {
			//handle redirect
			int redirectFileCheck = 0;
			if (redirect == 1) {
				printf("Redirect detected.\n");
				printf("Attempting to write to file: %s\n", outFilePtr);
				close(STDOUT_FILENO);
				if (open(outFilePtr, O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU) == -1) {
                    //error!
					write(STDERR_FILENO, error_message, strlen(error_message));
				} else {
					redirectFileCheck = 1;
				}
			}
			if (redirect == 0 || redirectFileCheck == 1) {
				if (execvp(argv[0], argv) == -1) {
					write(STDERR_FILENO, error_message, strlen(error_message));
				}
			}
			}
			else if (rc > 0) {
				if (background != 1) {
					(void) wait(NULL);
				}
			}
		}
	}
	return 0;
}