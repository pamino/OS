#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 9000

int try_(int r, char * err) {if (r == -1) { perror(err); exit(1); } return r; }

static inline void die(const char* msg)
{
	perror(msg);
	exit(-1);
}

int main()
{
	struct sockaddr_in srv_addr, cli_addr;
	int sockopt = 1;
	socklen_t sad_sz = sizeof(struct sockaddr_in);
	int sfd, cfd;
	ssize_t bytes;
	char buf[256];

	srv_addr.sin_family = AF_INET;
	srv_addr.sin_port = htons(PORT);
	srv_addr.sin_addr.s_addr = INADDR_ANY;

	sfd = try_(socket(AF_INET, SOCK_STREAM, 0), "Couldn't open the socket");

	setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, (char*)&sockopt, sizeof(sockopt));

	try_(bind(sfd, (struct sockaddr*)&srv_addr, sad_sz), "Couldn't bind socket");

	try_(listen(sfd, 1), "Couldn't listen to the socket");
	while(1) {
		cfd = try_(accept(sfd, (struct sockaddr *)&cli_addr, &sad_sz), "Couldn't accept incoming connection");
		while ((bytes = read(cfd, buf, sizeof(buf))) != 0)
		{
			if (bytes < 0)
				die("Couldn't receive message");

			if (write(cfd, buf, bytes) < 0)
				die("Couldn't send message");
		}

		close(cfd);
		close(sfd);
	}
	return 0;
}