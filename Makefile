CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2
LDLIBS ?= -pthread -lrt

.PHONY: all clean run

all: campuscompute

campuscompute: CampusCompute.c
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

run: campuscompute
	./campuscompute all

clean:
	rm -f campuscompute campuscompute.log
