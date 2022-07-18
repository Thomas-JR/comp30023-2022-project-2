CC=gcc
CFLAGS=-Wall -g
EXE=server
OBJ=server.o tools.o

$(EXE): $(OBJ)
	$(CC) $(CFLAGS) -o $(EXE) $^ -lpthread

%.o: %.c %.h
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	rm -f *.o $(EXE)

format:
	clang-format -i *.c *.h