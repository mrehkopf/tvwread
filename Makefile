CC := gcc

CFLAGS = -Wall -Wstrict-prototypes -Werror

BIN = tvwread

all: $(BIN)

tvwread: tvwread.o tvw.o
	$(CC) $(CFLAGS) $^ --output $@

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@