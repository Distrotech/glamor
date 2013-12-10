AUTOMAKE_OPTIONS=nostdinc
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src conf

aclocaldir = $(datadir)/aclocal

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = glamor-egl.pc glamor.pc

DISTCHECK_CONFIGURE_FLAGS=\
	--with-xkb-path=$(XKB_BASE_DIRECTORY) \
	--with-xkb-bin-directory=$(XKB_BIN_DIRECTORY) \
	--with-xkb-output='$${datadir}/X11/xkb/compiled'

.PHONY: ChangeLog INSTALL

INSTALL:
	$(INSTALL_CMD)

ChangeLog:
	$(CHANGELOG_CMD)

dist-hook: ChangeLog INSTALL

# gross hack
relink: all
	$(AM_V_at)$(MAKE) -C hw relink
