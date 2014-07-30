EXECUTABLES = auctioneer robotrader 
INCLUDES = -I../include/
CFLAGS = -g $(INCLUDES) 
CC = gcc
LIBDIR = ../lib
LIBS = $(LIBDIR)/libfdr.a #$(LIBDIR)/communicate.o

all: $(EXECUTABLES)

clean:
	rm -f core *.o a.out simple* result* $(EXECUTABLES)

.SUFFIXES: .c .o
.c.o:
	$(CC) $(CFLAGS) -c $*.c

auctioneer: auctioneer.o func.o communicate.o
	$(CC) $(CFLAGS) -o auctioneer auctioneer.o communicate.o func.o -lpthread -lm $(LIBS)

robotrader: robotrader.o func.o reason.o parse.o util.o and_or_tree.o communicate.o
	$(CC) $(CFLAGS) -o robotrader communicate.o func.o reason.o parse.o util.o and_or_tree.o robotrader.o -lpthread -lm $(LIBS)
