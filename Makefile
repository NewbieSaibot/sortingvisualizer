# Makefile

CC = gcc
CFLAGS = -O2 -Wall -Wextra -pthread
LDFLAGS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
EXEC = main
SRC = $(wildcard *.c)
H = $(wildcard *.h)
OBJ = $(SRC:.c=.o)

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(EXEC) $(LDFLAGS)

%.o: %.c $(H)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ)

fclean:
	rm -f $(EXEC)

re: fclean all

