CFLAGS=-std=c11 -g -static
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

dcc: $(OBJS)
	$(CC) -o dcc $(OBJS) $(LDFLAGS)

$(OBJS): dcc.h

test: dcc
	./dcc tests > tmp.s
	gcc -static -o tmp tmp.s
	./tmp

clean:
	rm -f dcc *.o *~ tmp*

.PHONY: test clean
