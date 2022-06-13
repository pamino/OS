#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "../lib/array.h"
#include "../lib/lib.h"

#define PORT 9000

static char** _ppIn;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

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

void input() {
	pthread_mutex_lock(&mutex);
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
	pthread_mutex_unlock(&mutex);
}

// ----- threadFunc ----
void* threadIn(void* arg) {
	int* pConnectionFd = arg;
	input();
	pthread_mutex_lock(&mutex);
	write (arg, _ppIn[0], arrayLen(_ppIn[0]));
	write (arg, " ", 1);
	write (arg, _ppIn[1], arrayLen(_ppIn[1]));
	if (!strcmp("put"), _ppIn[0]) {

	}
	pthread_mutex_unlock(&mutex);
	return NULL;
}

int main()
{
	struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_port = htons(PORT),
		.sin_addr.s_addr = inet_addr("127.0.0.1")
	};
	char buf[256];
	int connectionFd;
	
	if ((connectionFd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		die("Couldn't open the socket");
	
	if (connect(connectionFd, (struct sockaddr*) &addr, sizeof(addr)) < 0)
		die("Couldn't connect to socket");
	
	arrayInit(_ppIn);

	for (int i = 0; i < 5; ++i)
	{
		pthread_t threadId;
		pthread_create(&threadId, NULL, threadIn, &connectionFd);
		if (write(connectionFd, "Ping", 4) < 0)
			die("Couldn't send message");
		
		printf("[send] Ping\n");
		
		if (read(connectionFd, buf, sizeof(buf)) < 0)
			die("Couldn't receive message");
		
		printf("[recv] %s\n", buf);
	}
	if (write(connectionFd, "\0", 4) < 0)
			die("Couldn't send message");
	close(connectionFd);
	return 0;
}