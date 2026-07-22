CC = gcc

CFLAGS = -Wall -Wextra -Iinclude
LDFLAGS = -lcurl -lcjson

SRC = $(wildcard src/*.c)

speedtest: $(SRC)
	$(CC) $(CFLAGS) $(SRC) $(LDFLAGS) -o speedtest

clean:
	rm -f speedtest *.o