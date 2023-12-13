#include "list.h"
#include "server.h"

#include <errno.h>
#include <stdio.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

static char *get_user_action_alert(enum user_actions a, char *user)
{
	char *alert, *msg;
	switch(a)
	{
		case connection_action:
			msg = "connected\n";
			break;
		case disconnection_action:
			msg = "disconnected\n";
	}

	alert = malloc(strlen(user) + strlen(msg));
	sprintf(alert, "%s %s", user, msg);
	return alert;
}

static void session_send_string(struct session *sess, char *buf)
{
	write(sess->sockfd, buf, strlen(buf));
}

static void create_session(int ls, struct sockaddr *addr, struct list *sess_list)
{
	struct session *sess;
	socklen_t socklen = sizeof(*addr);
	int fd = accept(ls, addr, &socklen);
	int flags = fcntl(fd, F_GETFL);
	fcntl(fd, F_SETFL, flags | O_NONBLOCK);

	sess = malloc(sizeof(struct session));
	sess->sockfd = fd;
	sess->state = start_state;
	session_send_string(sess, "Type your name: ");

	list_append(sess, sess_list);
}

static void send_alert_to_chat(char *alert, struct list *sess_list)
{
	struct node *sess_node;
	for(sess_node = sess_list->first;
			sess_node; sess_node = sess_node->next)
	{
		struct session *sess = (struct session *)sess_node->data;
		write(sess->sockfd, alert, strlen(alert));	
	}
}

static void send_message_to_chat(struct session *sess_sender, struct list *sess_list)
{
	struct node *sess_node;
	int msg_size = sess_sender->size_name + sess_sender->buf_used + 2;
	char *msg = malloc(msg_size * sizeof(char));
	sess_sender->buf[sess_sender->buf_used] = '\0';
	sprintf(msg, "%s: %s", sess_sender->name, sess_sender->buf);

	for(sess_node = sess_list->first;
			sess_node; sess_node = sess_node->next)
	{
		struct session *sess = (struct session *)sess_node->data;
		if(sess != sess_sender)
			write(sess->sockfd, msg, msg_size);
	}
	free(msg);
}
static void session_fsm_step(struct session *sess, struct list *sess_list)
{
	switch(sess->state)
	{
		case start_state:
			sess->size_name = sess->buf_used - 1;
			sess->name = malloc(sess->buf_used * sizeof(char));
			memcpy(sess->name, sess->buf, sess->size_name);
			sess->name[sess->size_name - 1] = '\0';

			sess->state = chat_state;
			send_alert_to_chat(get_user_action_alert(connection_action, 
						sess->name), sess_list);
			break;
		case chat_state:
			send_message_to_chat(sess, sess_list);
	}
}

static void remove_session(struct node **sess_node, struct list *sess_list)
{
	struct node *tmp;	
	struct session *sess = (struct session *)(*sess_node)->data;

	close(sess->sockfd);

	tmp = (*sess_node)->next;
	send_alert_to_chat(
			get_user_action_alert(disconnection_action, sess->name),
			sess_list);

	list_remove(*sess_node, sess_list);
	*sess_node = tmp;
}

static void fdset_handle(struct server *server)
{
	struct node *sess_node;
	for(sess_node = server->sess_list.first;
			sess_node;)
	{
		struct session *sess = (struct session *)sess_node->data;

		if(FD_ISSET(sess->sockfd, &server->readfds))
		{
			sess->buf_used = read(sess->sockfd, sess->buf, INBUFSIZE);
			if(sess->buf_used == 0)
			{
				remove_session(&sess_node, &server->sess_list);
				continue;
			}
			else
				session_fsm_step(sess, &server->sess_list);

		} 
		sess_node = sess_node->next;
	}
}

static void fdset_check(struct server *server)
{
	struct node *sess_node;
	for(sess_node = server->sess_list.first;
			sess_node; sess_node = sess_node->next)
	{
		struct session *session = (struct session *)sess_node->data;
		FD_SET(session->sockfd, &server->readfds);
		if(session->sockfd > server->max_d)
			server->max_d = session->sockfd;
	}
}

static void session_callback_free(struct session *sess)
{
	free(sess->name);
	free(sess);
}

static int main_loop(struct server *server)
{
	int ok;
	struct sockaddr_in addr;
	list_init(&server->sess_list, (void (*)(void *))session_callback_free);

	for(;;)
	{
		server->max_d = server->ls;

		FD_ZERO(&server->readfds);
		FD_SET(server->ls, &server->readfds);

		fdset_check(server);

		ok = select(server->max_d + 1, &server->readfds, NULL, NULL, NULL);
		printf("select: %d\n", ok);

		if(ok == -1)
		{
			perror("select");
			return 1;
		}

		if(FD_ISSET(server->ls, &server->readfds))
		{
			printf("create session\n");
			create_session(server->ls, (struct sockaddr *) &addr, &server->sess_list);
		}

		fdset_handle(server);
	}
	return 0;
}

static void sockaddr_init(struct sockaddr_in *addr, int port)
{
	addr->sin_port = htons(port);
	addr->sin_family = AF_INET;
	addr->sin_addr.s_addr = htonl(INADDR_ANY);
}


static int server_init(struct server *server, int port)
{
	struct sockaddr_in addr;
	int flags, opt, ok;

	server->ls = socket(AF_INET, SOCK_STREAM, 0);
	flags = fcntl(server->ls, F_GETFL), opt = 1;
	fcntl(server->ls, F_SETFL, flags | O_NONBLOCK);

	setsockopt(server->ls, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	if(server->ls == -1)
	{
		perror("ls");
		return 3;
	}

	sockaddr_init(&addr, port);
	ok = bind(server->ls, (struct sockaddr*)&addr, sizeof(addr));

	if(ok == -1)
	{
		perror("bind");
		return 4;
	}
	
	signal(SIGPIPE, SIG_IGN);
	listen(server->ls, LISTEN_QUEUE_LEN);
	return 0;

}

int main(int argc, char **argv)
{
	struct server server;	
	char *endptr;
	long port;

	if(argc != 2)
	{
		fprintf(stderr, "Usage: chat_server <port>\n");
		return 1;
	}

	port = strtol(argv[1], &endptr, 10);
    if(!*argv[1] || *endptr) {
        fprintf(stderr, "Invalid port number\n");
        return 2;
    }
	
	if (server_init(&server, port))
		return 3;

	return main_loop(&server);
}
