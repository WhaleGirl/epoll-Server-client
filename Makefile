.PHONY:all
all:epoll_server epoll_client

epoll_server:epoll_server.c
	gcc -o $@ $^
epoll_client:epoll_client.c
	gcc -o $@ $^

.PHONY:clean

clean:
	rm -rf epoll_server epoll_client
