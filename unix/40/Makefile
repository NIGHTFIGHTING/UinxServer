.PHONY:clean
CC=g++
CFLAGS=-Wall -g
ALL=main
all:$(ALL)
OBJS=produce_consume.o
.c.o:
	$(CC) $(CFLAGS) -c $<

main:$(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ -lpthread -lrt

clean:
	rm -f $(ALL) *.o
