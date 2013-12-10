ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src conf

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = glamor-egl.pc glamor.pc

.PHONY: ChangeLog INSTALL

INSTALL:
	$(INSTALL_CMD)

ChangeLog:
	$(CHANGELOG_CMD)

dist-hook: ChangeLog INSTALL
