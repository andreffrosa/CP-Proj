CC=gcc
CFLAGS=-g -std=gnu11 -Wall -Werror -fcilkplus
LDFLAGS=-lcilkrts

S=debug.c main.c patterns.c unit.c prefix_sum.c
O=$(patsubst %.c,%.o,$(S))

TARGET=main
all: $(TARGET)

main: $(O)
	$(CC) -o $@ $^ $(LDFLAGS)
	
tester:
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ tester.c patterns.c prefix_sum.c

clean:
	rm -f $(TARGET) $(O) tester

debug.o: debug.c debug.h
main.o: main.c unit.h debug.h
patterns.o: patterns.c patterns.h prefix_sum.h
unit.o: unit.c patterns.h debug.h unit.h
tester.o: tester.c patterns.h prefix_sum.h
prefix_sum.o: prefix_sum.c prefix_sum.h

