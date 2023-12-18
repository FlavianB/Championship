CC = gcc

CFLAGS = -g

LIBS_SERVER = -lsqlite3 -lpthread -lcurl

SRCS_SERVER = server.c sqlite_utils.c djb2.c email_utils.c

SRCS_CLIENT = client.c terminal_utils.c

OBJS_SERVER = $(addprefix build/, $(SRCS_SERVER:.c=.o))

OBJS_CLIENT = $(addprefix build/, $(SRCS_CLIENT:.c=.o))

EXEC_SERVER = build/server
EXEC_CLIENT = build/client  

all: $(EXEC_SERVER) $(EXEC_CLIENT)

$(EXEC_SERVER): $(OBJS_SERVER)
	$(CC) $(CFLAGS) -o $(EXEC_SERVER) $(OBJS_SERVER) $(LIBS_SERVER)

$(EXEC_CLIENT): $(OBJS_CLIENT)
	$(CC) $(CFLAGS) -o $(EXEC_CLIENT) $(OBJS_CLIENT) $(LIBS)

build/%.o: %.c
	mkdir -p build
	$(CC) $(CFLAGS) -c $<  -o $@

clean:
	rm -f $(OBJS_SERVER) $(OBJS_CLIENT) $(EXEC_SERVER) $(EXEC_CLIENT)