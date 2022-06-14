
#include "../lib/lib.h"

#define PORT 9000

// ----- changeDir ----
void changeDir(const char* pArgument,char** ppDir) {
	char path[PATH_MAX];
	translateDir(pArgument, ppDir, path);
	if (!chdir(&path[1])) 
		exit(-1);
	strcpy(*ppDir, path);
}

// ----- makeDir ----
void makeDir(const char* pArgument, char** ppDir, char* pRet) {
	char path[PATH_MAX];
	translateDir(pArgument, ppDir, path);
	try_(mkdir(path, S_IRWXU | S_IRWXG), "couldn't write to file");
	strcpy(pRet, path);
}

// ----- threadFunc ----
void* threadFunc(void* arg) {
	int* connectionFd = arg;

	debug();
	ssize_t bytes;
	char buf[256];
	char pDir[PATH_MAX];
	char pStartDir[PATH_MAX];
	int stop[2] = {-1};

	getcwd(pStartDir,sizeof(pStartDir));
	strcpy(pDir, pStartDir);
	debug();
	dup2(*connectionFd, 0);
	dup2(*connectionFd, 1);

	while ((bytes = read(0, buf, sizeof(buf))) != 0){

		{
			char pRelPath[PATH_MAX];
			relate(pRelPath, pStartDir, pDir);
			write(1, pRelPath, sizeof(pRelPath));
			write(1, ">", 1);
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
			//get(ppInput[1]);
		}
		try_(write(*connectionFd, ppInput[0], arrayLen(ppInput[0])), "Couldn't send message");
		try_(write(*connectionFd, " ", arrayLen(ppInput)), "Couldn't send message");
		if(arrayLen(ppInput[1]) != 0) 
			try_(write(*connectionFd, ppInput[1], arrayLen(ppInput[1])), "Couldn't send message");
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