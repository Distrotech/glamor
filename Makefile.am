AUTOMAKE_OPTIONS=nostdinc
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src conf

aclocaldir = $(datadir)/aclocal

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = glamor-egl.pc glamor.pc

EXTRA_DIST = glamor-egl.pc.in glamor.pc.in autogen.sh

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

DIST_SUBDIRS = src conf

# gross hack
relink: all
	$(AM_V_at)$(MAKE) -C hw relink
