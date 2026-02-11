# BlazeNeuro Desktop Environment - Makefile

PREFIX ?= /usr/local
CC = gcc
CFLAGS = -Wall -Wextra -O2
PKG_GTK = $(shell pkg-config --cflags --libs gtk+-3.0 gdk-3.0)
PKG_VTE = $(shell pkg-config --cflags --libs vte-2.91)
PKG_X11 = $(shell pkg-config --cflags --libs x11)

BINDIR = $(PREFIX)/bin
SHAREDIR = $(PREFIX)/share/blazeneuro

all: blazeneuro-wm blazeneuro-dock blazeneuro-topbar \
     blazeneuro-terminal blazeneuro-files blazeneuro-launcher \
     blazeneuro-settings blazeneuro-notes

blazeneuro-wm: src/wm/wm.c
	$(CC) $(CFLAGS) -o $@ $< $(PKG_X11)

blazeneuro-dock: src/dock/dock.c
	$(CC) $(CFLAGS) -o $@ $< $(PKG_GTK) -lX11

blazeneuro-topbar: src/topbar/topbar.c
	$(CC) $(CFLAGS) -o $@ $< $(PKG_GTK) -lX11

blazeneuro-terminal: src/terminal/terminal.c
	$(CC) $(CFLAGS) -o $@ $< $(PKG_GTK) $(PKG_VTE)

blazeneuro-files: src/files/files.c
	$(CC) $(CFLAGS) -o $@ $< $(PKG_GTK)

blazeneuro-launcher: src/launcher/launcher.c
	$(CC) $(CFLAGS) -o $@ $< $(PKG_GTK)

blazeneuro-settings: src/settings/settings.c
	$(CC) $(CFLAGS) -o $@ $< $(PKG_GTK)

blazeneuro-notes: src/notes/notes.c
	$(CC) $(CFLAGS) -o $@ $< $(PKG_GTK)

install: all
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 blazeneuro-wm $(DESTDIR)$(BINDIR)/
	install -m 755 blazeneuro-dock $(DESTDIR)$(BINDIR)/
	install -m 755 blazeneuro-topbar $(DESTDIR)$(BINDIR)/
	install -m 755 blazeneuro-terminal $(DESTDIR)$(BINDIR)/
	install -m 755 blazeneuro-files $(DESTDIR)$(BINDIR)/
	install -m 755 blazeneuro-launcher $(DESTDIR)$(BINDIR)/
	install -m 755 blazeneuro-settings $(DESTDIR)$(BINDIR)/
	install -m 755 blazeneuro-notes $(DESTDIR)$(BINDIR)/
	install -d $(DESTDIR)$(SHAREDIR)
	install -m 644 theme/blazeneuro.css $(DESTDIR)$(SHAREDIR)/
	install -m 644 config/picom.conf $(DESTDIR)$(SHAREDIR)/
	install -m 755 config/autostart.sh $(DESTDIR)$(BINDIR)/blazeneuro-session
	install -d $(DESTDIR)$(SHAREDIR)/assets
	cp -r assets/* $(DESTDIR)$(SHAREDIR)/assets/ 2>/dev/null || true
	# Install Xsession entry
	install -d $(DESTDIR)/usr/share/xsessions
	install -m 644 config/blazeneuro.desktop $(DESTDIR)/usr/share/xsessions/

clean:
	rm -f blazeneuro-wm blazeneuro-dock blazeneuro-topbar \
	      blazeneuro-terminal blazeneuro-files blazeneuro-launcher \
	      blazeneuro-settings blazeneuro-notes

.PHONY: all install clean
