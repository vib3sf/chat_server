#include <ncurses.h>
#include <stdio.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

#define START_BUF_SIZE 4
#define OUTBUFSIZE 1024

static int select_loop(int sd);

static int client_init(char *ip_addr, long port);

int main(int argc, char **argv)
{
	char *ip_addr, *endptr;
	long port;
	int sd, flags;

	if(argc != 3) 
	{
		fprintf(stderr, "Usage: client <ip addr> <port>");
		return 1;
	}

	ip_addr = argv[1];
	port = strtol(argv[2], &endptr, 10);
    if(!*argv[1] || *endptr) 
	{
        fprintf(stderr, "Invalid port number\n");
        return 2;
	}

	sd = client_init(ip_addr, port);
	flags = fcntl(sd, F_GETFL);
	fcntl(sd, F_SETFL, flags | O_NONBLOCK);
	flags = fcntl(0, F_GETFL);
	fcntl(0, F_SETFL, flags | O_NONBLOCK);

	return select_loop(sd);
}

static int select_loop(int sd)
{
	fd_set readfds;
	int ok, rc, i;
	char buf_out[OUTBUFSIZE], buf_read[1024];

	for(;;)
	{
		FD_ZERO(&readfds);

		FD_SET(sd, &readfds);
		FD_SET(0, &readfds);	/* stdin */

		ok = select(sd + 1, &readfds, NULL, NULL, NULL);

		if(ok == -1)
		{
			perror("select");
			return 1;
		}

		if(FD_ISSET(sd, &readfds))
		{
			rc = read(sd, buf_out, OUTBUFSIZE);
			for(i = 0; i < rc; i++)
				putchar(buf_out[i]);
			fflush(stdout);
		}

		if(FD_ISSET(0, &readfds))
		{
			rc = read(0, buf_read, 1024);
			write(sd, buf_read, rc);
		}
	}
	return 0;
}

static int client_init(char *ip_addr, long port)
{
	struct sockaddr_in addr;
	int sd = socket(AF_INET, SOCK_STREAM, 0), ok;

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	ok = inet_aton(ip_addr, &addr.sin_addr);
	if(!ok) 
	{
		fprintf(stderr, "Wrong ip address\n");
		exit(4);
	}

	ok = connect(sd, (struct sockaddr *) &addr, sizeof(addr));

	if(ok == -1) 
	{
		fprintf(stderr, "Failed connect\n");
		perror("connect");
		exit(5);
	}

	return sd;
}
