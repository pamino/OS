#include <stdio.h> 
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <pthread.h>
#include "array.h"

static char** in_;
static char oldDir_[PATH_MAX];
static char dir_[PATH_MAX];
int child_;

void release (char*** arr) {
	for(int i = 0; i < arrayLen(*arr); ++i) arrayRelease((*arr)[i]);
	arrayClear(*arr);
}

void debug() {
	static int i = 0;
	++i;
	printf( "pos %i\n", i);
}

void input() {
	release(&in_);
	int i = 0;
	char* newElement;
	arrayInit(newElement);
	arrayPush(in_) = newElement;
	while(1) {
		char temp = getchar();
		if(temp == ' ') {
			char* newElement;
			arrayInit(newElement);
			arrayPush(in_) = newElement;
			arrayPush(in_[i]) = '\0';
			++i;
		}
		else if(temp == '\n') {
			arrayPush(in_[i]) = '\0';
			return;
		}
		else arrayPush(in_[i]) = temp;
	}
} 

void split(const char delimiter,const char* str, char** ret) {
	char* newElement;
	arrayInit(newElement);
	arrayPush(ret) = newElement;
	int pos = 0;
	for(int i=1; str[i]; ++i) {
		if(str[i] == delimiter) {
			arrayPush(ret[pos]) = '\0';
			char* newElement;
			arrayInit(newElement);
			arrayPush(ret) = newElement;
			++pos;
		}
		else {
			arrayPush(ret[pos]) = str[i];
		}
	}
	arrayPush(ret[pos]) = '\0';
}

void delete(char** arr, int pos) {
	for(int i = pos; i < arrayLen(arr)-1; ++i) {
		arr[i] = arr[i+1];
	}
	arrayRelease(arr[arrayLen(arr)-1]);
	char* discard = arrayPop(arr);++discard;
}

void translateDir(char** dirs, char* ret) {
	for(int i = 0; i<arrayLen(dirs); ++i) {
		if(!strcmp(dirs[i], "..")) {
			if(i >= 1) {
				delete(dirs, i);
				delete(dirs, i-1);
			}
		}
	}
	strcpy(ret, "");
	strcat(ret, "/");
	for(int i = 0; i < arrayLen(dirs); ++i) {
		char temp[arrayLen(dirs[i])+2];
		strcpy(temp, dirs[i]);
		if(i != arrayLen(dirs)-1) strcat(temp, "/");
		strcat(ret,temp);
	}

}

void* w8Pid(void* child) {

	int status;
	waitpid(*(int*)(child), &status, 0); 
	if(! WIFEXITED(status)) {
		printf("[%i] finished with status code %i!\n", *(int*)(child), status);
	}
	else printf("[%i] finished normally!\n", *(int*)(child));
	return NULL;
}

enum {
	NONE,
	READ,
	WRITE
};

void signalhandler(int i) {
	printf("[%i] TERMINATED!", child_);
	kill(child_, SIGINT);
}

void execute(const char* command, int start, int end, bool wait, int fd[2], int hasPipe ) {
	// acts on globael variable _in; arguments "begin" at start and end at "end"
	char** arguments;
	arrayInit(arguments);
	for(int i = start; i < end; ++i) {
		arrayPush(arguments) = in_[i];
	}

	if (!strcmp(command, "cd")) {
		if (arguments[0][0] == '-') {
			if (!chdir(&oldDir_[1])) exit(-1);
			char temp[PATH_MAX];
			strcpy(temp, dir_);
			strcpy(dir_,oldDir_);
			strcpy(oldDir_,temp);
			return;
		}
		strcpy(oldDir_, dir_);
		char path[PATH_MAX];
		strcpy(path, dir_);
		if(arguments[0][0] != '/') strcat(path,"/");
		strcat(path, arguments[0]);
		char** dirs;
		arrayInit(dirs);
		split('/', path, dirs);
		translateDir(dirs, path);
		if (!chdir(&path[1])) exit(-1);
		strcpy(dir_, path);
		release(&dirs);
		return;
	}

	if (!strcmp(command, "wait")){
		int status;
		char** end = NULL;
		int pid = strtol(arguments[0], end , 10);
		child_ = pid;
		signal(SIGINT, signalhandler);
		waitpid(pid, &status, 0);
		signal(SIGINT, SIG_DFL);
		return;
	}

	int child = fork();
	if (child == 0) {
		if (hasPipe == READ) {
			dup2(fd[0], STDIN_FILENO);
			close(fd[1]);
		}
		else if (hasPipe == WRITE) {
			dup2(fd[1], STDOUT_FILENO);
			close(fd[0]);
		}
		else {
			close(fd[0]);
			close(fd[1]);
		}
		char* argumentsTemp[arrayLen(command)+2];
		argumentsTemp[0] = "shell";
		for(int i = 0; i < arrayLen(command); ++i) argumentsTemp[i+1] = arguments[i];
		char command2[100];
		strcat(command2, "./");
		strcat(command2, command);
		if (execvp(command, argumentsTemp) == -1) execvp(command2, argumentsTemp);
		printf("execution of %s failed!\n", command);
		exit(-1);
	}

	int status;
	if (wait) waitpid(child, &status, 0);
	else {
		printf("\n[%i]\n ", child);
		pthread_t thread;
		pthread_create(&thread, NULL, &w8Pid, (void*)&child);
		//child process continues, is that a problem?
	}
}

int main(void) {
	arrayInit(in_);
	char *command = in_[0];
	if (!getcwd(dir_, sizeof(dir_))) exit(-1);
	strcpy(oldDir_, dir_);
	while (printf("\n%s> ", dir_), input(),  command = in_[0], strcmp(command, "exit"))
	{
		if (strcmp(command, "")) {
			int usesPipe = 0;
			int begin = 1;
			int end = 1;
			int hasPipe = NONE;
			bool wait = true;
			
			for (int i = 0; i < arrayLen(in_); ++i) {
				if (in_[i][0] == '|') {
					usesPipe = i;
					end = i;
					hasPipe = WRITE;
				}
			}

			if (!usesPipe) 
				end = arrayLen(in_);

			int fd[2];
			if(pipe(fd) < 0)
				exit(-1);

			if (in_[end - 1][0] == '&') {
				--end;
				wait = false;
			}
			execute(command, begin, end, wait, fd, hasPipe);
			if(usesPipe) {
				if(strcmp(in_[usesPipe+1], "")) {
					wait = true;
					command = in_[usesPipe+1];
					end = arrayLen(in_);
					if (in_[end - 1][0] == '&') {
						--end;
						wait = false;
					}
					execute(command, usesPipe+2, end, wait, fd, READ);
					close(fd[0]);
					close(fd[1]);
				}
			}
		}
	}
	release(&in_);
}
