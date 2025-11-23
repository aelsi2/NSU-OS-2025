CC = gcc
LDLIBS = -lsocket -lnsl
CFLAGS = -O2 -Wall

java: proxy.c
	$(LINK.c) -o $@ $(LDLIBS) $^
