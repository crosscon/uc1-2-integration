# Makefile
CC ?= $(CROSS_COMPILE)gcc
CFLAGS += -Wall
LDFLAGS += $(shell pkg-config --libs wolfssl)
CFLAGS += $(shell pkg-config --cflags wolfssl)

SRC = main.c
BIN = mtls

all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(BIN) $(LDFLAGS)

clean:
	rm -f $(BIN)

install:
	install -d $(DESTDIR)/usr/bin
	install -m 0755 mtls $(DESTDIR)/usr/bin
