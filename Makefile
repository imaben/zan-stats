CC=gcc
CFLAGS=-g -D_GNU_SOURCE -D_DEFAULT_SOURCE
LIBS=-lncursesw -lm -lpanelw
PROC=zan-stats

all:
	$(CC) $(CFLAGS) -o $(PROC) stats.c $(LIBS)

clean:
	rm -rf zan-stats
