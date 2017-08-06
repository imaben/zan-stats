CC=gcc
CFLAGS=-g -D_GNU_SOURCE -D_DEFAULT_SOURCE
LIBS=-lncursesw -lm -lpanelw -lcurl -L/usr/local/Cellar/ncurses/6.0_1/lib/
PROC=zan-stats

all:
	$(CC) $(CFLAGS) -o $(PROC) stats.c $(LIBS)

clean:
	rm -rf zan-stats
