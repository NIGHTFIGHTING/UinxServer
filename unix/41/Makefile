.PHONY:clean
CC=g++
CFLAGS=-Wall -g
ALL=pool
all:$(ALL)
OBJS=threadpool.o main.o condition.o
.c.o:
	$(CC) $(CFLAGS) -c $<

pool:$(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ -lpthread -lrt

clean:
	rm -f $(ALL) *.o
