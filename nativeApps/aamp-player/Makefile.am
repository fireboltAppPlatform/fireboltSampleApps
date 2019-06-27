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
AUTOMAKE_OPTIONS = subdir-objects
# ACLOCAL_AMFLAGS = -I cfg

SUBDIRS =

AM_CXXFLAGS = -g \
	-Wall \
	-std=c++0x \
	-DGCC4_XXX \
	$(CURL_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GST_CFLAGS) \
	$(JANSON_CFLAGS) \
	-I$(top_srcdir) \
	-I=/usr/include/pxcore \
	-I=/usr/include/ \
	-I$(top_srcdir)/common \
	-I=/usr/include/rdk/ds \
	-I=/usr/include/rdk/ds-hal \
	-I=/usr/include/rdk/iarmbus \
	-I=/usr/include/rdk/iarmmgrs


AM_CXXFLAGS += -DRT_PLATFORM_LINUX

bin_PROGRAMS = aampplayer simpleaampplayer

aampplayer_SOURCES = \
	main.cpp \
	common/rtevents.h \
	common/rtevents.cpp \
	common/glib_tools.cpp \
	common/glib_tools.h \
	common/logger.cpp \
	common/logger.h \
	common/intrect.h \
	aampplayer.h \
	aampplayer.cpp 
	
aampplayer_LDFLAGS = \
	-lIARMBus -Wl,--allow-shlib-undefined

AM_CXXFLAGS += -DDISABLE_CLOSEDCAPTIONS

aampplayer_LDFLAGS += -laamp -lrtRemote -lrtCore

aampplayer_LDFLAGS += \
	$(CURL_LIBS) \
	$(GLIB_LIBS) \
	$(GST_LIBS) \
	$(JANSON_LIBS)

#simpleaampplayer
simpleaampplayer_SOURCES = main_aamp.cpp	

simpleaampplayer_LDFLAGS = -laamp -Wl,--allow-shlib-undefined
	
simpleaampplayer_LDFLAGS += \
	$(CURL_LIBS) \
	$(GLIB_LIBS) \
	$(GST_LIBS) \
	$(JANSON_LIBS)


if ENABLE_BREAKPAD
AM_CXXFLAGS += -DENABLE_BREAKPAD $(breakpad_CFLAGS)
aampplayer_LDFLAGS += $(breakpad_LIBS)
endif

