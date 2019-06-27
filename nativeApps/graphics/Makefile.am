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
	-I$(top_srcdir)/include \
   -I${PKG_CONFIG_SYSROOT_DIR}/$(includedir)/wayland \
   $(WAYLAND_CLIENT_CFLAGS) $(WAYLAND_SERVER_CFLAGS) \
   $(XKBCOMMON_CFLAGS)   
AM_LDFLAGS=$(WAYLANDLIB) -Wl,--allow-shlib-undefined

## --- Configurtion -------
if ENABLE_PLATFORM_EMBEDDED
AM_CXXFLAGS += -DWESTEROS_PLATFORM_EMBEDDED
endif

if HAVE_WAYLAND_EGL
AM_CXXFLAGS += -DWESTEROS_HAVE_WAYLAND_EGL
WSTEGL_LIBS= -lwayland-egl -lGLESv2
else
WSTEGL_LIBS= -lwesteros_egl
endif

if ENABLE_WESTEROS_SBPROTOCOL
AM_CXXFLAGS += -DENABLE_SBPROTOCOL
endif

AM_CFLAGS = $(AM_CXXFLAGS) -Wno-deprecated-declarations

bin_PROGRAMS =
bin_PROGRAMS += rne_triangle

## --- Sample client -------
rne_triangle_SOURCES = rne-triangle.cpp
rne_triangle_LDFLAGS= \
   $(AM_LDFLAGS) \
   $(WAYLAND_CLIENT_LIBS) \
   $(XKBCOMMON_LIBS) \
   -lxkbcommon \
   -lwesteros_simpleshell_client \
   $(WSTEGL_LIBS) \
   -lEGL
