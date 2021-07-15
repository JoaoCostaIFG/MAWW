LDFLAGS = -lX11 -lImlib2
CC = gcc
CFLAGS = -O2 -Wall -Wextra -Wpedantic
BIN = maww
SRC = maww.c

$(BIN) : $(SRC) Makefile
	$(CC) $(CFLAGS) $(SRC) $(LDFLAGS) -o $(BIN)

debug: $(SRC) Makefile
	$(CC) -g $(CFLAGS) $(SRC) $(LDFLAGS) -o $(BIN)

install: $(BIN)
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp -f $(BIN) $(DESTDIR)$(PREFIX)/bin
	chmod 755 $(DESTDIR)$(PREFIX)/bin/$(BIN)

uninstall:
	rm $(DESTDIR)$(PREFIX)/bin/$(BIN)

clean:
	rm -f $(BIN)

pkgname = maww

dist: clean
	rm -rf $(pkgname)-$(VERSION)
	mkdir -p $(pkgname)-$(VERSION)
	cp Makefile maww.c $(pkgname)-$(VERSION)
	tar --zstd -cf $(pkgname)-$(VERSION).pkg.tar.zst $(pkgname)-$(VERSION)
	rm -rf $(pkgname)-$(VERSION)
	updpkgsums
	makepkg --printsrcinfo > .SRCINFO
	
