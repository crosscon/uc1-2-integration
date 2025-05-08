# TLS Examples Makefile
CC					?= $(CROSS_COMPILE)gcc
WOLFSSL_INSTALL_DIR ?= /usr/local
LIBTEEC_INSTALL_DIR ?= #FIXME for building in yocto
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

# build template
%: %.c
	$(CC) -o $@ $(DEPS) $< $(CFLAGS) $(LDFLAGS) $(LIBS)

clean:
	rm -f $(TARGETS)

# Install the binaries into /usr/bin
install: $(TARGETS)
	install -d $(DESTDIR)/usr/bin
	install -m 0755 $(TARGETS) $(DESTDIR)/usr/bin/
	install -d $(DESTDIR)/etc/mtls
	install -m 0644 certs/* $(DESTDIR)/etc/mtls/
