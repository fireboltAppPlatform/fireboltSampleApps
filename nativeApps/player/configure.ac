#
# If not stated otherwise in this file or this component's Licenses.txt file the
# following copyright and licenses apply:
#
# Copyright 2016 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

dnl AC_PREREQ([2.65])
AC_INIT(westeros, 1.0.0, jeff_wannamaker@cable.comcast.com,westeros)
AC_CONFIG_SRCDIR([])
AC_CONFIG_HEADERS([cfg/config.h])
AC_CONFIG_MACRO_DIR([cfg])
AC_CONFIG_AUX_DIR([cfg])
AC_CONFIG_SUBDIRS([essos])
AM_INIT_AUTOMAKE([foreign no-dist-gzip dist-bzip2 1.9])
LT_INIT

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_CXX

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
AC_HEADER_STDBOOL

ENABLE_WESTEROS_PLAYER=" "
EGL_DETECTED=" "
GLESV2_DETECTED=" "
GLEW_DETECTED=" "
WAYLAND_EGL_DETECTED=" "
ENABLE_BREAKPAD=" "

# Checks for library functions.
#Add the subdirectories to be considered for building.
SUBDIRS=" "
#to setup m4 macros
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])],
[AM_DEFAULT_VERBOSITY=1
AC_SUBST(AM_DEFAULT_VERBOSITY)])

PKG_CHECK_MODULES([GLIB],[glib-2.0 >= 2.22.5])
PKG_CHECK_MODULES([GTHREAD],[gthread-2.0 >= 2.38.2])
PKG_CHECK_MODULES([WAYLAND_CLIENT],[wayland-client >= 1.6.0])
PKG_CHECK_MODULES([WAYLAND_SERVER],[wayland-server >= 1.6.0])
PKG_CHECK_MODULES([WAYLAND_EGL],[wayland-egl >= 0.0],[WAYLAND_EGL_DETECTED=true],[WAYLAND_EGL_DETECTED=false])
PKG_CHECK_MODULES([XKBCOMMON],[xkbcommon >= 0.4])
PKG_CHECK_MODULES([EGL],[egl >= 0.0],[EGL_DETECTED=true],[EGL_DETECTED=false])
PKG_CHECK_MODULES([GLESV2],[glesv2 >= 0.0],[GLESV2_DETECTED=true],[GLESV2_DETECTED=false])

AM_CONDITIONAL([HAVE_WAYLAND_EGL], [test x$WAYLAND_EGL_DETECTED = xtrue])              
AM_CONDITIONAL([HAVE_EGL], [test x$EGL_DETECTED = xtrue])              
AM_CONDITIONAL([HAVE_GLESV2], [test x$GLESV2_DETECTED = xtrue])              

AC_ARG_ENABLE([player],
              AS_HELP_STRING([--enable-player],[enable westeros_player (default is no)]),
              [
                case "${enableval}" in
                 yes) ENABLE_WESTEROS_PLAYER=true ;;
                  no) AC_MSG_ERROR([westeros_player is disabled]) ;;
                   *) AC_MSG_ERROR([bad value ${enableval} for --enable-player]) ;;
                esac
              ],
              [echo "westeros_player is disabled"])
AM_CONDITIONAL([ENABLE_WESTEROS_PLAYER], [test x$ENABLE_WESTEROS_PLAYER = xtrue])              

AC_ARG_ENABLE([breakpad],
              AS_HELP_STRING([--enable-breakpad],[enable breakpad support (default is no)]),
              [
                case "${enableval}" in
                 yes) ENABLE_BREAKPAD=true;
		      PKG_CHECK_MODULES([BREAKPAD], [breakpad-client >= 0.0]) ;;
                  no) AC_MSG_ERROR([breakpad is disabled ]) ;;
                   *) AC_MSG_ERROR([bad value ${enableval} for --enable-breakpad]) ;;
                esac
              ],
              [echo "breakpad is disabled"])
AM_CONDITIONAL([ENABLE_BREAKPAD], [test x$ENABLE_BREAKPAD = xtrue])


GST_MAJORMINOR=1.0
PKG_CHECK_MODULES([GST], [gstreamer-1.0 >= 1.0], have_gst1="yes", have_gst1="no")

AS_IF([test "x$have_gst1" = "xyes"], [
  PKG_CHECK_MODULES([GSTBASE], [gstreamer-base-1.0 >= 1.0])
  AC_DEFINE(USE_GST1, 1, [Build with GStreamer 1.x])
], [])

AS_IF([test "x$enable_gstreamer0" != "xyes" -a "x$have_gst1" != "xyes"], [
   AC_MSG_ERROR([Could not find GStreamer 1.x dependencies:

$GST_PKG_ERRORS])], [])

WAYLANDLIB="-lwayland-server"
AC_SUBST(WAYLANDLIB)

AC_CONFIG_FILES([Makefile])
AC_SUBST(WAYLAND_CFLAGS)
AC_SUBST(GUPNP_VERSION)
AC_SUBST(ENABLE_WESTEROS_PLAYER)
AC_OUTPUT

