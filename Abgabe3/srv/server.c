#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <unistd.h>
#include <limits.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "../lib/array.h"

#define PORT 9000

void debug() {
	static int i = 0;
	++i;
	printf( "pos %i\n", i);
}

int try_(int r, char *err) {
	if (r == -1) {
		perror(err);
		exit(1);
	}
	return r;
}

static inline void die(const char* msg)
{
	perror(msg);
	exit(-1);
}

void delete(char** ppArr, int pos) {
	for (int i = pos; i < arrayLen(ppArr) - 1; ++i) {
		ppArr[i] = ppArr[i + 1];
	}
	arrayRelease(ppArr[arrayLen(ppArr) - 1]);
	char* pDiscard = arrayPop(ppArr); ++pDiscard;
}

// ----- relate ----
void relate(char* pOut, const char* pBase, const char* pOff) {
	// 1: skip the shared prefix
	int shared = 0;
	for (; pOff[shared] == pBase[shared] && pOff[shared]; ++shared);
	if (pOff[shared] == pBase[shared]) {
		memcpy(pOut, "./", 3); 
		return;
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

// ----- split ----
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

// ----- translateDir ----
void translateDir(const char* pArguments, char** ppDir, char* pRet) {
	strcpy(pRet, *ppDir);
	if (pArguments[0] != '/') strcat(pRet, "/");
	strcat(pRet, pArguments);

	char** ppDirs;
	arrayInit(ppDirs);
	split('/', pRet, ppDirs);

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
	release(&ppDirs);
}

// ----- changeDir ----
void changeDir(const char* pArguments,char** ppDir) {
	char path[PATH_MAX];
	translateDir(pArguments, ppDir, path);
	if (!chdir(&path[1])) exit(-1);
	strcpy(*ppDir, path);
}

// ----- makeDir ----
void makeDir(const char* pArguments, char** ppDir, char* pRet) {
	char path[PATH_MAX];
	translateDir(pArguments, ppDir, path);
	try_(mkdir(path, S_IRWXU | S_IRWXG), "couldn't write to file)";
	strcpy(pRet, path);
}

// ----- threadFunc ----
void* threadFunc(void* arg) {
	int* connectionFd = arg;

	ssize_t bytes;
	char buf[256];
	char pDir[PATH_MAX];
	char pStartDir[PATH_MAX];

	dup2(*connectionFd, 0);
	dup2(*connectionFd, 1);

	while ((bytes = read(0, buf, sizeof(buf))) != 0){

		{
			char pRelPath[PATH_MAX];
			relate(pRelPath, pStartDir, pDir);
			printf("%s>", pRelPath);
		}

		char** ppInput;
		arrayInit(ppInput);
		split(' ', buf, ppInput);
		if (!strcmp(ppInput[0],"cd")) {
			changeDir(ppInput[1], &pDir);
		}
		else if (!strcmp(ppInput[0],"put")) {
			char pPath[PATH_MAX];
			makeDir(ppInput[1], &pDir, pPath);
			int fd = try_(open(pPath, O_APPEND), "couldn't open file");
			try_(write(*connectionFd, ".", 1), "Couldn't send message");
			while (1) {
				int smallBuf[2];
				int byte = read (0, smallBuf, 1);
				if (byte != 1) 
					return NULL;
				if (smallBuf[0] == -1)
					break;
				write(fd, smallBuf, 1);
			}
		}
		else if (!strcmp(ppInput[0],"get")) {
			get(ppInput[1]);
		}
		int stop[2] = {-1};
		try_(write(*connectionFd, stop, 1), "Couldn't send message");
	}

	try_(close(*connectionFd), "could not close socket");
	free(arg);
	return NULL;
}

int main()
{
	struct sockaddr_in srv_addr, cli_addr;
	int sockopt = 1;
	socklen_t sad_sz = sizeof(struct sockaddr_in);
	int socketFd;

	srv_addr.sin_family = AF_INET;
	srv_addr.sin_port = htons(PORT);
	srv_addr.sin_addr.s_addr = INADDR_ANY;

	socketFd = try_(socket(AF_INET, SOCK_STREAM, 0), "Couldn't open the socket");

	setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR, (char*)&sockopt, sizeof(sockopt));

	try_(bind(socketFd, (struct sockaddr*)&srv_addr, sad_sz), "Couldn't bind socket");
	try_(listen(socketFd, 1), "Couldn't listen to the socket");
	while(1) {
		int* connectionFd = (int*) malloc(sizeof(int));
		*connectionFd = try_(accept(socketFd, (struct sockaddr *)&cli_addr, &sad_sz), "Couldn't accept incoming connection");
		pthread_t threadId;
		int ret = pthread_create(&threadId, NULL, threadFunc, (void*) connectionFd);
		pthread_join(threadId, NULL); //temp
		if (ret != 0) { 
			printf("Error from pthread: %d\n", ret); 
			exit(1); 
		}
	}
	try_(close(socketFd), "could not close socket");
	return 0;
}