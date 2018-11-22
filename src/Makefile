CC=gcc
CFLAGS=-g -std=gnu11 -Wall -Werror -fcilkplus
LDFLAGS=-lcilkrts

S=debug.c main.c patterns.c unit.c tester.c
O=$(patsubst %.c,%.o,$(S))

TARGET=main
all: $(TARGET) tester

main: $(O)
	$(CC) -o $@ $^ $(LDFLAGS)
	
tester: tester.o patterns.o
	$(CC) -o $@ $^ tester.c $(LDFLAGS)

clean:
	rm -f $(TARGET) $(O)

debug.o: debug.c debug.h
main.o: main.c unit.h debug.h
patterns.o: patterns.c patterns.h
unit.o: unit.c patterns.h debug.h unit.h
tester.o: tester.c patterns.h