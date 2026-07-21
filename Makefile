CC = clang

CFLAGS = -Wall -Wextra -Iinclude -I/opt/homebrew/opt/cjson/include
LDFLAGS = -L/opt/homebrew/lib -lcurl -lcjson

SRC = $(wildcard src/*.c)

speedtest: $(SRC)
	$(CC) $(CFLAGS) $(SRC) $(LDFLAGS) -o speedtest

clean:
	rm -f speedtest