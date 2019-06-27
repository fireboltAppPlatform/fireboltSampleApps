##########################################################################
# If not stated otherwise in this file or this component's Licenses.txt
# file the following copyright and licenses apply:
#
# Copyright 2018 RDK Management
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
##########################################################################
#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

dnl AC_PREREQ([2.65])
AC_INIT(aampplayer, 1.0)
AC_CONFIG_SRCDIR([aampplayer.cpp])
AC_CONFIG_HEADERS([cfg/config.h])
AC_CONFIG_MACRO_DIR([cfg])
AC_CONFIG_AUX_DIR([cfg])
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

ENABLE_DIRECT_QAM=" "
ENABLE_IPPV_CLIENT=" "
ENABLE_VOD_SRC=" "
ENABLE_RDK_LOGGER=" "
ENABLE_BREAKPAD=" "
ENABLE_RMF=" "

# Checks for library functions.
#Add the subdirectories to be considered for building.
SUBDIRS=" "
#to setup m4 macros
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])],
[AM_DEFAULT_VERBOSITY=1
AC_SUBST(AM_DEFAULT_VERBOSITY)])

PKG_CHECK_MODULES([GLIB], [glib-2.0 >= 2.22.5])
PKG_CHECK_MODULES([GST],  [gstreamer-1.0 >= 1.4])
PKG_CHECK_MODULES([CURL], [libcurl >= 7.19.6])
PKG_CHECK_MODULES([JANSON], [jansson >= 2.4])

AC_ARG_ENABLE([directQAM],
              AS_HELP_STRING([--enable-directQAM],[enable direct QAM (default is no)]),
              [
                case "${enableval}" in
                 yes) ENABLE_DIRECT_QAM=true ;;
                  no) AC_MSG_ERROR([direct QAM is disabled]) ;;
                   *) AC_MSG_ERROR([bad value ${enableval} for --enable-directQAM]) ;;
                esac
              ],
              [echo "direct QAM is disabled"])
AM_CONDITIONAL([ENABLE_DIRECT_QAM], [test x$ENABLE_DIRECT_QAM = xtrue])

AC_ARG_ENABLE([IPPVClient],
              AS_HELP_STRING([--enable-IPPVClient],[enable IPPV client (default is no)]),
              [
                case "${enableval}" in
                 yes) ENABLE_IPPV_CLIENT=true ;;
                  no) AC_MSG_ERROR([IPPV client is disabled]) ;;
                   *) AC_MSG_ERROR([bad value ${enableval} for --enable-IPPVClient]) ;;
                esac
              ],
              [echo "IPPV client is disabled"])
AM_CONDITIONAL([ENABLE_IPPV_CLIENT], [test x$ENABLE_IPPV_CLIENT = xtrue])

AC_ARG_ENABLE([VODSrc],
              AS_HELP_STRING([--enable-VODSrc],[enable VOD src (default is no)]),
              [
                case "${enableval}" in
                 yes) ENABLE_VOD_SRC=true ;;
                  no) AC_MSG_ERROR([VOD src is disabled]) ;;
                   *) AC_MSG_ERROR([bad value ${enableval} for --enable-VODSrc]) ;;
                esac
              ],
              [echo "VOD src is disabled"])
AM_CONDITIONAL([ENABLE_VOD_SRC], [test x$ENABLE_VOD_SRC = xtrue])

AC_ARG_ENABLE([rdk-logger],
              AS_HELP_STRING([--enable-rdk-logger],[enable rdk logger (default is no)]),
              [
                case "${enableval}" in
                 yes) ENABLE_RDK_LOGGER=true ;;
                  no) AC_MSG_ERROR([rdk logger is disabled]) ;;
                   *) AC_MSG_ERROR([bad value ${enableval} for --enable-rdk-logger]) ;;
                esac
              ],
              [echo "rdk logger is disabled"])
AM_CONDITIONAL([ENABLE_RDK_LOGGER], [test x$ENABLE_RDK_LOGGER = xtrue])

AC_ARG_ENABLE([breakpad],
              AS_HELP_STRING([--enable-breakpad],[enable breakpad support (default is no)]),
              [
                case "${enableval}" in
                 yes) ENABLE_BREAKPAD=true
                      PKG_CHECK_MODULES([breakpad], [breakpad, breakpad-client]) ;;
                  no) AC_MSG_ERROR([breakpad support is disabled]) ;;
                   *) AC_MSG_ERROR([bad value ${enableval} for --enable-breakpad]) ;;
                esac
              ],
              [echo "breakpad support is disabled"])
AM_CONDITIONAL([ENABLE_BREAKPAD], [test x$ENABLE_BREAKPAD = xtrue])

AM_CONDITIONAL([USE_PXCORE_SHARED_LIBS], [test x$USE_PXCORE_SHARED_LIBS = xTRUE])

AC_ARG_ENABLE([comcast-build],
              AS_HELP_STRING([--enable-comcast-build],[enable comcast build (default is no)]),
              [
                case "${enableval}" in
                 yes) ENABLE_COMCAST_BUILD=true ;;
                  no) [echo "comcast build is disabled"] ;;
                   *) AC_MSG_ERROR([bad value ${enableval} for --enable-comcast-build]) ;;
                esac
              ],
              [
                ENABLE_COMCAST_BUILD=false
                [echo "comcast build is disabled"]
              ])
AM_CONDITIONAL([ENABLE_COMCAST_BUILD], [test x$ENABLE_COMCAST_BUILD = xtrue])

AC_CONFIG_FILES([Makefile])
AC_SUBST(ENABLE_DIRECT_QAM)
AC_SUBST(ENABLE_IPPV_CLIENT)
AC_SUBST(ENABLE_VOD_SRC)
AC_SUBST(ENABLE_RDK_LOGGER)
AC_SUBST(ENABLE_BREAKPAD)
AC_OUTPUT
