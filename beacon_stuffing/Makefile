CC = gcc
CFLAGS = -I.
DEPS = headers.h
OBJ = main.o passive_scan.o utils.o nl_callbacks.o
LIBNL = $(shell pkg-config --cflags --libs libnl-genl-3.0)

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) $(LIBNL)

all: $(OBJ)
	$(CC) -g -o beacon_stuffing $^ $(LIBNL)

clean:
	rm -f beacon_stuffing
	rm -f *.o
