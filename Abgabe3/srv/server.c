#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <limits.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "../lib/array.h"

#define PORT 9000

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

void* threadFunc(void* arg) {
	int* connectionFd = arg;

	ssize_t bytes;
	char buf[256];
	char pDir[PATH_MAX];
	char pStartDir[PATH_MAX];

	dup2(*connectionFd, 0);
	dup2(*connectionFd, 1);

	// {
	// 	char pRelPath[PATH_MAX];
	// 	relate(pRelPath, pStartDir, pDir);
	// 	printf("%s>", pRelPath);
	// }
	
	while ((bytes = read(*connectionFd, buf, sizeof(buf))) != 0){	
		// for(int i = 0; i< bytes; ++i) {

		// }
		buf[bytes] = '\0';
		printf(buf);
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
	//while(1) {
		int* connectionFd = (int*) malloc(sizeof(int));
		*connectionFd = try_(accept(socketFd, (struct sockaddr *)&cli_addr, &sad_sz), "Couldn't accept incoming connection");
		pthread_t threadId;
		int ret = pthread_create(&threadId, NULL, threadFunc, (void*) connectionFd);
		pthread_join(threadId, NULL);
		if (ret != 0) { 
			printf("Error from pthread: %d\n", ret); 
			exit(1); 
		}
	//}
	try_(close(socketFd), "could not close socket");
	return 0;
}