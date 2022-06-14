
#include "../lib/array.h"
#include "../lib/lib.h"
#include <stdbool.h>

#define PORT 9000

static char* _pFile;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static bool threadActive = false;

void lock(bool active) {
	pthread_mutex_lock(&mutex);
	threadActive = active;
	pthread_mutex_unlock(&mutex);
}

void input(char*** ppIn) {
	release(ppIn);
	int i = 0;
	char* pNewEle;
	arrayInit(pNewEle);
	arrayPush(*ppIn) = pNewEle;
	while (1) {
		char temp = getchar();
		if (temp == ' ') {
			char* pNewStr;
			arrayInit(pNewStr);
			arrayPush(*ppIn) = pNewStr;
			arrayPush(*ppIn[i]) = '\0';
			++i;
		}
		else if (temp == '\n') {
			arrayPush(*ppIn[i]) = '\0';
			return;
		}
		else arrayPush(*ppIn[i]) = temp;
	}
}

// ----- threadFunc ----
void* threadIn(void* arg) {
	lock(true);
	int* pConnectionFd = arg;
	char pDir[PATH_MAX];
	char** ppIn;
	arrayInit(ppIn);
	getcwd(pDir,sizeof(pDir));

	while (1) {
		input(&ppIn);
		write(*pConnectionFd, ppIn[0], arrayLen(ppIn[0]));
		write(*pConnectionFd, " ", 1);
		write(*pConnectionFd, ppIn[1], arrayLen(ppIn[1]));

		char buf[256];

		if (!strcmp("exit", ppIn[0])) {
			lock(false);
			return NULL;
		}
		else if (!strcmp("put", ppIn[0])) {
			char pPath[PATH_MAX];
			translateDir(ppIn[1], &pDir, pPath);
			int fd = try_(open(pPath, O_RDONLY), "couldn't open file");
			while (read(fd, &buf, sizeof(buf)))
			{
				write(*pConnectionFd, &buf, sizeof(buf));
			}
			buf[0] = -1;
			write(*pConnectionFd, &buf, 1);
		}
		else if (!strcmp("get", ppIn[0])) {
		}
	}
	lock(false);
	return NULL;
}

int main()
{
	struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_port = htons(PORT),
		.sin_addr.s_addr = inet_addr("127.0.0.1")
	};
	int connectionFd;

	if ((connectionFd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		die("Couldn't open the socket");

	if (connect(connectionFd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
		die("Couldn't connect to socket");

	arrayInit(_pFile);

	pthread_t threadId;

	int smallBuf[2];
	debug();
	while (read(connectionFd, &smallBuf, 1) != 0) {
		pthread_mutex_lock(&mutex);
		if (!threadActive) {
			pthread_create(&threadId, NULL, threadIn, &connectionFd);
		}
		pthread_mutex_unlock(&mutex);
		if (smallBuf[0] < 0) {
			while (read(connectionFd, &smallBuf, 1)) {
				if (smallBuf[0] < 0)
					break;
				else
					arrayPush(_pFile) = smallBuf[0];
			}
		}
		else {
			putchar(smallBuf[0]);
		}
	}
	close(connectionFd);
	return 0;
}