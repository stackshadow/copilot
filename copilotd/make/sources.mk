#!/bin/make
# An common makefile-based build system
#	Copyright (C) 2017 by Martin Langlotz
#
#	this is free software ( as in free Beer ): you can redistribute it and/or modify
#	it under the terms of the GNU Lesser General Public License as published by
#	the Free Software Foundation, either version 3 of the License, or
#	(at your option) any later version.
#
#	this is distributed in the hope that it will be useful,
#	but WITHOUT ANY WARRANTY; without even the implied warranty of
#	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#	GNU Lesser General Public License for more details.
#
#	You should have received a copy of the GNU Lesser General Public License
#	If not, see <http://www.gnu.org/licenses/>.


# from the default stuff starts, please dont edit this,
# just add your files at the end of this file, to keep
# track what is changed, and what is default

# at first, you need to define the source path of you app
# please NOT change the values here, just add yours at the END !
sourcePath =

# the build path, where all *.o files and other stuff that update
# rapidly. Most users prefer the /tmp/build to lower the io on your spinning disk :)
buildPath =

# here are the final binarys ( please don't use / )
# mos users prefer /tmp/dodb
targetPath =


# source which are compiled and linked to your app
sources +=

# QT-Sources
# moc's are automatically generated and compiled
# this objects are linked to your app
sourcesQT =


# UI-Sources
# .ui qt-designer files, which are compiled
# this objects are linked to your app
sourcesUI =


################## FINISHED ##################
# from here on, you can add your own variables
MOC = moc

sourcePath	= .
buildPath   = /tmp/dodbd-ws/build
targetPath  = /tmp/dodbd-ws/target




# core
sources     += src/evillib.c
sources     += src/main.cpp

sources     += src/coCore.cpp
sources     += src/coPluginElement.cpp
sources     += src/coPlugin.cpp


# plugins
sources     += src/plugins/coreService.cpp
sources     += src/plugins/nftService.cpp
sources     += src/plugins/ldapService.cpp


# additional sources which compiles to an shared object
sourcesLib  +=

CFLAGS      += -pie -fPIE -fPIC
CFLAGS      += -I/usr/include/qt
CFLAGS      += -I$(sourcePath)/src
CFLAGS      += -I$(sourcePath)/libs/evillib/core
CFLAGS      += -I$(sourcePath)/libs/evillib/extra
CFLAGS      += -I$(sourcePath)/libs/util-linux-2.29.2/libuuid/src

CFLAGSDBG	+= -g

CFLAGSREL   += -fstack-protector
CFLAGSREL   += -Wformat -Wformat-security
CFLAGSREL   += -O2


CLIBS       += -lz
CLIBS       += -luuid
#CLIBS       += -luWS
CLIBS       += -Wl,-rpath /usr/lib64
CLIBS		+= -lQt5Core
CLIBS		+= -lldap
CLIBS		+= $(shell pkg-config --libs libsodium)



# websocket
ifdef DISABLE_WEBSOCKET
CFLAGS      += -DDISABLE_WEBSOCKET
else
sources     += src/plugins/websocket.cpp
#sourcesQT   += src/plugins/websocketClient.cpp
#CLIBS		+= -lQt5Network
#CLIBS		+= -lQt5WebSockets
CLIBS		+= -lwebsockets
endif

# mqtt
ifdef DISABLE_MQTT
CFLAGS      += -DDISABLE_MQTT
else
sources     += src/plugins/mqttService.cpp
CLIBS		+= -lmosquitto
endif

ifdef DISABLE_NFT
CFLAGS      += -DDISABLE_NFT
endif

ifdef DISABLE_LDAP
CFLAGS      += -DDISABLE_LDAP
endif

ifdef _DEBUG
CFLAGS      += -D_DEBUG
endif


default: binary-qt

client:
	make -f make/Makefile DISABLE_WEBSOCKET=1 binary-qt


install:
	cp $(buildPath)/app /usr/bin/copilotd
	cp $(sourcePath)/copilotd.service /lib/systemd/system/copilotd.service
