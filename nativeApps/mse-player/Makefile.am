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

AUTOMAKE_OPTIONS = subdir-objects

SUBDIRS =
AM_CXXFLAGS= \
	 -std=c++0x \
   -DRT_PLATFORM_LINUX \
	-I$(top_srcdir)/include \
   -I${PKG_CONFIG_SYSROOT_DIR}/$(includedir)/wayland \
   -I=/usr/include/pxcore \
   $(WAYLAND_CLIENT_CFLAGS) $(WAYLAND_SERVER_CFLAGS) \
   $(XKBCOMMON_CFLAGS)   
AM_LDFLAGS=$(WAYLANDLIB) -Wl,--allow-shlib-undefined

bin_PROGRAMS = mse_player

## --- Sample player -------
mse_player_SOURCES = main.cpp \
mediasourcepipeline.cpp \
GstMSESrc.cpp \
glib_tools.cpp

if !HAVE_GLESV2
GLESV2_LIBS = "-lGLESv2 "
endif

mse_player_CXXFLAGS = $(AM_CXXFLAGS) $(GST_CFLAGS)
mse_player_LDFLAGS= \
   $(AM_LDFLAGS) \
   $(GST_LIBS) \
   $(GSTAPP_LIBS) \
   $(GSTVIDEO_LIBS) \
   $(WAYLAND_CLIENT_LIBS) \
   $(WSTEGL_LIBS) \
   -lEGL \
   -lessos \
   $(GLESV2_LIBS) \
   -lrtCore \
   -lrtRemote


