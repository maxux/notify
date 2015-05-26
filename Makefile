include config.mk

SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) -c $(CFLAGS) $*.c

clean:
	rm -fv *.o

mrproper: clean
	rm -fv $(EXEC)
