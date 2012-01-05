#  Copyright 2011 Zhigang Gong.
#
#  Permission is hereby granted, free of charge, to any person obtaining a
#  copy of this software and associated documentation files (the "Software"),
#  to deal in the Software without restriction, including without limitation
#  on the rights to use, copy, modify, merge, publish, distribute, sub
#  license, and/or sell copies of the Software, and to permit persons to whom
#  the Software is furnished to do so, subject to the following conditions:
#
#  The above copyright notice and this permission notice (including the next
#  paragraph) shall be included in all copies or substantial portions of the
#  Software.
#
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#  FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
#  ADAM JACKSON BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
#  IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
#  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#
# Process this file with autoconf to produce a configure script


# Initialize Autoconf
AC_PREREQ([2.63])
AC_INIT([glamor-egl], 
        [1.0.0],
        [https://bugs.freedesktop.org/enter_bug.cgi?product=glamor], 
        [glamor-egl])

AC_CONFIG_SRCDIR([Makefile.am])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])

# Initialize Automake
AM_INIT_AUTOMAKE([foreign dist-bzip2])
AM_MAINTAINER_MODE

# Initialize libtool
AC_PROG_LIBTOOL
LT_PREREQ([2.2])
LT_INIT([disable-static])

# Initialize X.Org macros 1.8 or later for MAN_SUBSTS set by XORG_MANPAGE_SECTIONS
m4_ifndef([XORG_MACROS_VERSION], 
          [m4_fatal([must install xorg-macros 1.8 or later before running autoconf/autogen])])
XORG_MACROS_VERSION(1.8)
XORG_DEFAULT_OPTIONS

# Checks for programs.
# AC_PROG_LN_S

# Obtain compiler/linker options for dependencies

PKG_PROG_PKG_CONFIG

PKG_CHECK_MODULES(XORG, [xorg-server >= 1.10])
PKG_CHECK_MODULES(DRI2, [dri2proto >= 2.6])

LIBDRM="libdrm >= 2.4.23"
LIBGL="gl >= 7.1.0"
LIBPIXMAN="pixman-1 >= 0.21.8"
LIBEGL="egl"
LIBGLESV2="glesv2"
LIBGBM="gbm"

# Define a configure option for an alternate input module directory
AC_ARG_WITH(xorg-module-dir,
            AC_HELP_STRING([--with-xorg-module-dir=DIR],
                           [Default xorg module directory [[default=$libdir/xorg/modules]]]),
            [moduledir="$withval"],
            [moduledir="$libdir/xorg/modules"])

AC_SUBST([moduledir])

# X Server SDK location is required to install evdev header files
# This location is also relayed in the xorg-evdev.pc file
sdkdir=`$PKG_CONFIG --variable=sdkdir xorg-server`
AC_SUBST([sdkdir])

AC_MSG_CHECKING([whether to include GLAMOR_GLES2 support])
AC_ARG_ENABLE(glamor-gles2,         AS_HELP_STRING([--enable-glamor-gles2], [Build glamor based on gles2 (default: no)]), [GLAMOR_GLES2="$enableval"], [GLAMOR_GLES2=no])
AC_MSG_RESULT([$GLAMOR_GLES2])

AM_CONDITIONAL([GLAMOR_GLES2], [test "x$GLAMOR_GLES2" = xyes])

if test "x$GLAMOR_GLES2" = xyes; then
   AC_DEFINE(GLAMOR_GLES2,1,[Build glamor over GLES2])
   PKG_CHECK_MODULES(GLESV2, $LIBGLESV2)
   REQUIRED_LIBS="$REQUIRED_LIBS $LIBGLESV2"
   GLAMOR_GL_CFLAGS="$GLESV2_CFLAGS -DGLAMOR_GLES2"
else 
   AC_DEFINE(GLAMOR_GL,1,[Build glamor over GL])
   PKG_CHECK_MODULES(GL, $LIBGL)
   REQUIRED_LIBS="$REQUIRED_LIBS $LIBGL"
   GLAMOR_GL_CFLAGS="$GL_CFLAGS -DGLAMOR_GL"
fi

AC_SUBST([GLAMOR_GL_CFLAGS])

PKG_CHECK_MODULES([LIBDRM], $LIBDRM)
PKG_CHECK_MODULES(EGL, $LIBEGL, [EGL=yes], [EGL=no])
AM_CONDITIONAL([EGL], [test "x$EGL" = xyes])

if test "x$EGL = xyes"; then
   PKG_CHECK_MODULES(GBM, $LIBGBM)
fi

AC_OUTPUT([Makefile
           glamor-egl.pc
           glamor.pc
           src/Makefile])
