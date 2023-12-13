CC = clang
CFLAGS = -Wall -g

SERVER_SRC = $(wildcard server/*.c)
CLIENT_SRC = $(wildcard client/*.c)

all: chat_server chat_client

chat_server: $(SERVER_SRC)
	$(CC) $(CFLAGS) $^ -o $@

chat_client: $(CLIENT_SRC)
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm chat_server chat_client
