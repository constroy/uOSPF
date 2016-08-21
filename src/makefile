CC = gcc
CFLAGS = -g -Wall -lpthread
SRC = $(wildcard *.c)
HEAD = $(wildcard *.h)
OBJ = $(SRC:.c=.o)

uospf: $(HEAD) $(OBJ)
	$(CC) $(CFLAGS) $(LIBS) -o $@ $(OBJ)

$(OBJ): $(HEAD) $(SRC)
	$(CC) -c $(CFLAGS) $(SRC)

.PHONY: clean

clean:
	rm -f *.o *.gch uospf
