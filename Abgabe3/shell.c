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

static char** _ppIn;
static char _pStartDir[PATH_MAX];
static char _pOldDir[PATH_MAX];
static char _Dir[PATH_MAX];
int child_;

void release(char*** pppArr) {
	for (int i = 0; i < arrayLen(*pppArr); ++i) arrayRelease((*pppArr)[i]);
	arrayClear(*pppArr);
}

void debug() {
	static int i = 0;
	++i;
	printf("pos %i\n", i);
}


void relate(char* pOut, const char* pBase, const char* pOff) {
	// 1: skip the shared prefix
	int shared = 0;
	for (; pOff[shared] == pBase[shared] && pOff[shared]; ++shared);
	if (pOff[shared] == pBase[shared]) {
		memcpy(pOut, "./", 3); return;
	}
	const char* ptr = pOff + shared;
	while (*--ptr != '/');

	shared = ptr - pOff + 1;
	int ascend = 0;
	for (const char* ptr = pBase + shared; (ptr = strchr(ptr, '/')); ++ptr, ++ascend);
	ascend *= 3;
	memmove(pOut + ascend, pOff + shared, strlen(pOff) - shared + 1);
	memset(pOut, '.', ascend);
	for (char* ptr = pOut; ascend > 0; ptr += 3, ascend -= 3) ptr[2] = '/';
}

void input() {
	{
		char pRelPath[PATH_MAX];
		relate(pRelPath, _pStartDir, _Dir);
		printf("%s>", pRelPath);
	}

	release(&_ppIn);
	int i = 0;
	char* pNewEle;
	arrayInit(pNewEle);
	arrayPush(_ppIn) = pNewEle;
	while (1) {
		char temp = getchar();
		if (temp == ' ') {
			char* pNewStr;
			arrayInit(pNewStr);
			arrayPush(_ppIn) = pNewStr;
			arrayPush(_ppIn[i]) = '\0';
			++i;
		}
		else if (temp == '\n') {
			arrayPush(_ppIn[i]) = '\0';
			return;
		}
		else arrayPush(_ppIn[i]) = temp;
	}
}

void split(char delimiter, const char* pStr, char** ppRet) {
	char* pNewEle;
	arrayInit(pNewEle);
	arrayPush(ppRet) = pNewEle;
	int pos = 0;
	for (int i = 1; pStr[i]; ++i) {
		if (pStr[i] == delimiter) {
			arrayPush(ppRet[pos]) = '\0';
			char* pNewStr;
			arrayInit(pNewStr);
			arrayPush(ppRet) = pNewStr;
			++pos;
		}
		else {
			arrayPush(ppRet[pos]) = pStr[i];
		}
	}

	arrayPush(ppRet[pos]) = '\0';
}

void delete(char** ppArr, int pos) {
	for (int i = pos; i < arrayLen(ppArr) - 1; ++i) {
		ppArr[i] = ppArr[i + 1];
	}
	arrayRelease(ppArr[arrayLen(ppArr) - 1]);
	char* pDiscard = arrayPop(ppArr); ++pDiscard;
}

void translateDir(char** ppDirs, char* pRet) {
	for (int i = 0; i < arrayLen(ppDirs); ++i) {
		if (!strcmp(ppDirs[i], "..")) {
			if (i >= 1) {
				delete(ppDirs, i);
				delete(ppDirs, i - 1);
			}
		}
	}
	strcpy(pRet, "");
	strcat(pRet, "/");
	for (int i = 0; i < arrayLen(ppDirs); ++i) {
		char pTemp[arrayLen(ppDirs[i]) + 2];
		strcpy(pTemp, ppDirs[i]);
		if (i != arrayLen(ppDirs) - 1) strcat(pTemp, "/");
		strcat(pRet, pTemp);
	}

}

void* w8Pid(void* pChild) {

	int status;
	waitpid(*(int*)(pChild), &status, 0);
	if (!WIFEXITED(status)) {
		printf("[%i] finished with status code %i!\n", *(int*)(pChild), status);
	}
	else printf("[%i] finished normally!\n", *(int*)(pChild));
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

void waiting(const char** ppArguments) {
	int status;
	char** ppEnd = NULL;
	int pid = strtol(*ppArguments, ppEnd, 10);
	child_ = pid;
	signal(SIGINT, signalhandler);
	waitpid(pid, &status, 0);
	signal(SIGINT, SIG_DFL);
	return;
}

void changeDir(const char** ppArguments) {
	if ((*ppArguments)[0] == '-') {
		if (!chdir(&_pOldDir[1])) exit(-1);
		char temp[PATH_MAX];
		strcpy(temp, _Dir);
		strcpy(_Dir, _pOldDir);
		strcpy(_pOldDir, temp);
		return;
	}
	strcpy(_pOldDir, _Dir);
	char path[PATH_MAX];
	strcpy(path, _Dir);
	if ((*ppArguments)[0] != '/') strcat(path, "/");
	strcat(path, (*ppArguments));

	char** ppDirs;
	arrayInit(ppDirs);
	split('/', path, ppDirs);
	translateDir(ppDirs, path);
	if (!chdir(&path[1])) exit(-1);
	strcpy(_Dir, path);
	release(&ppDirs);
}

void execute(const char* pCommand, int start, int end, bool wait, int pFd[2], int hasPipe) {
	// acts on globael variable _in; arguments "begin" at start and end at "end"
	char** arguments;
	arrayInit(arguments);
	for (int i = start; i < end; ++i) {
		arrayPush(arguments) = _ppIn[i];
	}

	if (!strcmp(pCommand, "cd")) {
		changeDir(&arguments[0]);
		return;
	}

	if (!strcmp(pCommand, "wait")) {
		waiting(&arguments[0]);
	}

	int child = fork();
	if (child == 0) {
		if (hasPipe == READ) {
			dup2(pFd[0], STDIN_FILENO);
			close(pFd[1]);
		}
		else if (hasPipe == WRITE) {
			dup2(pFd[1], STDOUT_FILENO);
			close(pFd[0]);
		}
		else {
			close(pFd[0]);
			close(pFd[1]);
		}
		char* ppArgumentsTemp[arrayLen(pCommand) + 2];
		char* pName = "shell";
		ppArgumentsTemp[0] = pName;
		for (int i = 0; i < arrayLen(pCommand); ++i) ppArgumentsTemp[i + 1] = arguments[i];
		char pCommand2[100] = "";
		strcat(pCommand2, "./");
		strcat(pCommand2, pCommand);
		printf("pCommand: %s \n", pCommand);
		execvp(pCommand, ppArgumentsTemp);
		execvp(pCommand2, ppArgumentsTemp);
		printf("execution of %s failed!\n", pCommand);
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
	arrayInit(_ppIn);
	char* pCommand = _ppIn[0];
	if (!getcwd(_Dir, sizeof(_Dir))) exit(-1);
	strcpy(_pOldDir, _Dir);
	strcpy(_pStartDir, _Dir);
	while (input(), pCommand = _ppIn[0], strcmp(pCommand, "exit"))
	{
		if (strcmp(pCommand, "")) {

		}
	}
	release(&_ppIn);
}
