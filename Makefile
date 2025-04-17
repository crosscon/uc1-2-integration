# TLS Examples Makefile
CC					?= $(CROSS_COMPILE)gcc
WOLFSSL_INSTALL_DIR = /usr/local
CFLAGS				= -Wall -I$(WOLFSSL_INSTALL_DIR)/include $(shell pkg-config --cflags wolfssl)
LIBS				= -L$(WOLFSSL_INSTALL_DIR)/lib -lm $(shell pkg-config --libs wolfssl)

# option variables
DYN_LIB         = -lwolfssl
STATIC_LIB      = $(WOLFSSL_INSTALL_DIR)/lib/libwolfssl.a
DEBUG_FLAGS     = -g -DDEBUG
OPTIMIZE        = -Os
DEPS            =

# Options
#CFLAGS+=$(OPTIMIZE)
#LIBS+=$(STATIC_LIB)
LIBS+=$(DYN_LIB)

# build targets
TARGETS = client-tls server-tls

.PHONY: clean all debug install

# Default target
all: $(TARGETS)

# Debug build
debug: CFLAGS+=$(DEBUG_FLAGS)
debug: all

# build template
%: %.c
	$(CC) -o $@ $(DEPS) $< $(CFLAGS) $(LIBS)

clean:
	rm -f $(TARGETS)

# Install the binaries into /usr/bin
install: $(TARGETS)
	install -d $(DESTDIR)/usr/bin
	install -m 0755 $(TARGETS) $(DESTDIR)/usr/bin/
	install -d $(DESTDIR)/etc/mtls
	install -m 0644 certs/* $(DESTDIR)/etc/mtls/
