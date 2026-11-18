CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2
LDLIBS ?= -pthread -lrt

.PHONY: all clean run

all: campuscompute

SOURCES = main.c scheduler.c logger.c ipc.c banker.c workers.c
OBJECTS = $(SOURCES:.c=.o)

campuscompute: $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS) $(LDLIBS)

%.o: %.c campuscompute.h
	$(CC) $(CFLAGS) -c $< -o $@

run: campuscompute
	./campuscompute all

clean:
	rm -f $(OBJECTS) campuscompute campuscompute.log
