
all: s.out c.out

s.out: server.c
	gcc server.c -o s.out

c.out: client.c
	gcc client.c -o c.out