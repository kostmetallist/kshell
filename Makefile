CC = gcc
CFLAGS = -g -Wall -ansi -pedantic
OBJFILES = $(patsubst %.c, %.o, $(wildcard *.c))
EXECUTABLES = OUT

all: OUT

OUT: $(OBJFILES)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c %.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(EXECUTABLES) $(OBJFILES)
