CC ?= cc
CFLAGS ?= -O2 -g
CFLAGS += -std=c11 -Wall -Wextra -Werror -pedantic
CPPFLAGS += $(shell pkg-config --cflags x11 xft libpng gio-unix-2.0 gdk-pixbuf-2.0)
LDLIBS += $(shell pkg-config --libs x11 xft libpng gio-unix-2.0 gdk-pixbuf-2.0)

linsoft-native: linsoft-native.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $< $(LDLIBS)

test-native-icons: test-native-icons.c linsoft-native.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $< $(LDLIBS)

clean:
	rm -f linsoft-native test-native-icons

.PHONY: clean
