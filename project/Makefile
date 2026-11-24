# Crypto Messenger build configuration
# Version: 0.1.0
# Updated: 2026-11-23
# Description: Builds the C HTTP backend.

CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Wpedantic
CPPFLAGS += -Isrc/inc $(shell pkg-config --cflags libmicrohttpd json-c sqlite3 openssl)
LDLIBS += $(shell pkg-config --libs libmicrohttpd json-c sqlite3 openssl)

SOURCES = src/core/main.c src/core/http_server.c src/core/crypto.c src/database/db.c
OBJECTS = $(SOURCES:.c=.o)
TARGET = build/crypto-messenger

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	mkdir -p build
	$(CC) $(CFLAGS) $(OBJECTS) -o $@ $(LDLIBS)

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

#==== End of file Makefile ====
