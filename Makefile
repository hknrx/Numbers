CC=gcc
CFLAGS=-O2

.PHONY: all
all: NumbersTest NumbersBenchmark GameDemo

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

NumbersTest: NumbersTest.c NumbersLibrary.o RandomLibrary.o
	$(CC) -o $@ $^ $(CFLAGS)

NumbersBenchmark: NumbersBenchmark.c NumbersLibrary.c RandomLibrary.o
	$(CC) -o $@ $^ $(CFLAGS) -lpthread -DDISABLE_COMPLEXITY

GameDemo: GameDemo.c NumbersLibrary.o RandomLibrary.o
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean
clean:
	rm -f NumbersTest NumbersBenchmark GameDemo *.o
