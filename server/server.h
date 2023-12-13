#ifndef SERVER_H
#define SERVER_H

#define INBUFSIZE 1024
#define LISTEN_QUEUE_LEN 16

#include "list.h"
#include <sys/select.h>
#include <netinet/in.h>

enum fsm_states
{
	start_state,
	chat_state
};

enum user_actions
{
	connection_action,
	disconnection_action
};

struct server
{
	struct list sess_list;
	int ls, res, fd, max_d;
	fd_set readfds;
	struct timeval timeout;
};

struct session
{
	int sockfd;
	char buf[INBUFSIZE];
	int buf_used;
	enum fsm_states state;
	char *name;
	int size_name;
};

struct message
{
	int fd;
	char *author, *content;
	int size_author, size_content;
};

#endif 
