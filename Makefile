# TLS Examples Makefile
CC					?= $(CROSS_COMPILE)gcc
WOLFSSL_INSTALL_DIR ?= /usr/local
CFLAGS				+= -Wall -I$(WOLFSSL_INSTALL_DIR)/include -I$(LIBTEEC_INSTALL_DIR)/include
LIBS				+= -L$(WOLFSSL_INSTALL_DIR)/lib -L$(LIBTEEC_INSTALL_DIR)/lib -lm
LDFLAGS				+= -Wl,--hash-style=gnu

# option variables
DYN_LIB         = -lwolfssl -lteec
DEBUG_FLAGS     = -g -DDEBUG
OPTIMIZE        = -Os
DEPS            =

# Options
#CFLAGS+=$(OPTIMIZE)
LIBS+=$(DYN_LIB)

# build targets
TARGETS = client-tls server-tls

.PHONY: clean all debug install

# Default target
all: $(TARGETS)

# Debug build
debug: CFLAGS+=$(DEBUG_FLAGS)
debug: all

# Source files
COMMON_SRCS = include/common/transmission.c include/common/challenge.c
CLIENT_SRCS = client-tls.c $(COMMON_SRCS)
SERVER_SRCS = server-tls.c $(COMMON_SRCS)

client-tls: $(CLIENT_SRCS)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(LIBS)

server-tls: $(SERVER_SRCS)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(LIBS)

clean:
	rm -f $(TARGETS)

# Install the binaries into /usr/bin
install: $(TARGETS)
	install -d $(DESTDIR)/usr/bin
	install -m 0755 $(TARGETS) $(DESTDIR)/usr/bin/
