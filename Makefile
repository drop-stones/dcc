CFLAGS=-std=c11 -g -static -D_GNU_SOURCE
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

dcc: $(OBJS)
	$(CC) -o dcc $(OBJS) $(LDFLAGS)

$(OBJS): dcc.h

test: dcc
	./test.sh

clean:
	rm -f dcc *.o *~ tmp*

.PHONY: test clean
