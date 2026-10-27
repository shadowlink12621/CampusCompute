CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2
LDLIBS ?= -pthread -lrt

.PHONY: all clean
all: campuscompute

campuscompute: CampusCompute.c
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

clean:
	rm -f campuscompute campuscompute.log
