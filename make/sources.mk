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
buildPath   = /tmp/copilotd/build
targetPath  = /tmp/copilotd/target




# core
sources     += src/evillib.c
sources     += src/main.cpp

sources     += src/coCore.cpp
sources     += src/coPluginElement.cpp
sources     += src/coPlugin.cpp


# plugins
sources     += src/plugins/coreService.cpp




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

CLIBS       += -ljansson
CLIBS       += -lpthread
CLIBS       += -lz
CLIBS       += -luuid
#CLIBS       += -luWS
#CLIBS       += -Wl,-rpath /usr/lib64
CLIBS		+= $(shell pkg-config --libs libsodium)



# mqtt
ifdef DISABLE_MQTT
CFLAGS      += -DDISABLE_MQTT
else
sources     += src/plugins/mqttService.cpp
CLIBS		+= -lmosquitto
endif
ifdef MQTT_ONLY_LOCAL
CFLAGS      += -DMQTT_ONLY_LOCAL
endif

# websocket
ifdef DISABLE_WEBSOCKET
CFLAGS      += -DDISABLE_WEBSOCKET
CFLAGS      += -Dwebsocket_H
else
sources     += src/plugins/websocket.cpp
#sourcesQT   += src/plugins/websocketClient.cpp
#CLIBS		+= -lQt5Network
#CLIBS		+= -lQt5WebSockets
CLIBS		+= -lwebsockets
endif

ifdef DISABLE_NFT
CFLAGS      += -DDISABLE_NFT
else
sources     += src/plugins/nftService.cpp
endif

ifdef DISABLE_LDAP
CFLAGS      += -DDISABLE_LDAP
CFLAGS      += -DldapService_H
else
sources     += src/plugins/ldapService.cpp
CLIBS		+= -lldap
endif

ifdef _DEBUG
CFLAGS      += -D_DEBUG
endif


default: binary-qt



/etc/copilot/services:
	@mkdir -v -p /etc/copilot/services
	@chown -R copilot:copilot /etc/copilot


client:
	make -f make/Makefile \
	DISABLE_WEBSOCKET=1 \
	MQTT_ONLY_LOCAL=1 \
	binary
clientTargets = /etc/copilot/services



clientTargets += $(prefix)/usr/bin/copilotd
$(prefix)/usr/bin/copilotd: $(buildPath)/app
	@cp -v $< $@

clientTargets += $(prefix)/lib/systemd/system/copilotd.service
$(prefix)/lib/systemd/system/copilotd.service: src/client/copilotd.service
	@cp -v $< $@

clientTargets += $(prefix)/lib/systemd/system/copilotd-ssh.service
$(prefix)/lib/systemd/system/copilotd-ssh.service:
	@ServerName=$(shell read -p "Enter the Server IP or Server-Name ( like dyndns.test.com ): ";echo $$REPLY) ;\
	sed -e "s/copilot@server/copilot@$$ServerName/" src/client/copilotd-ssh.service > $@

clientTargets += $(prefix)/lib/systemd/system/copilotd-ssh-keygen.service
$(prefix)/lib/systemd/system/copilotd-ssh-keygen.service: src/client/copilotd-ssh-keygen.service
	@cp -v $< $@

install-client: client $(clientTargets)
	@if [ "$(shell id -u copilot)" == "" ]; then useradd -U -m -s /usr/bin/nologin copilot; fi
	@chown -R copilot:copilot /etc/copilot
	systemctl daemon-reload

uninstall-client:
	@systemctl stop copilotd-ssh
	@rm -v $(clientTargets)




engineering:
	make -f make/Makefile \
	binary

singlestation:
	make -f make/Makefile binary-dbg



#### server
server: src/server/sshd_hostkey_ed25519
serverTargets = /etc/copilot/services

serverTargets += $(prefix)/etc/copilot/mqtt-server.conf
$(prefix)/etc/copilot/mqtt-server.conf: src/server/mqtt-server.conf
	@cp -v $< $@

serverTargets += $(prefix)/lib/systemd/system/copilotd-mqtt.service
$(prefix)/lib/systemd/system/copilotd-mqtt.service: src/server/copilotd-mqtt.service
	@cp -v $< $@

serverTargets += $(prefix)/etc/copilot/sshd_copilotd.conf
$(prefix)/etc/copilot/sshd_copilotd.conf: src/server/sshd_copilotd.conf
	@cp -v $< $@

serverTargets += $(prefix)/etc/copilot/sshd_authorized_keys
$(prefix)/etc/copilot/sshd_authorized_keys:
	touch $@

serverTargets += $(prefix)/lib/systemd/system/copilotd-sshd-keygen.service
$(prefix)/lib/systemd/system/copilotd-sshd-keygen.service: src/server/copilotd-sshd-keygen.service
	@cp -v $< $@

serverTargets += $(prefix)/lib/systemd/system/copilotd-sshd.service
$(prefix)/lib/systemd/system/copilotd-sshd.service: src/server/copilotd-sshd.service
	@cp -v $< $@

install-server: $(serverTargets)
	@if [ "$(shell id -u copilot)" == "" ]; then useradd -U -m -s /usr/bin/nologin copilot; fi
	@chown -R copilot:copilot /etc/copilot
	systemctl daemon-reload
uninstall-server:
	@systemctl stop copilotd-sshd
	@systemctl stop copilotd-mqtt
	@rm -v $(serverTargets)

install:
	cp $(buildPath)/app /usr/bin/copilotd
	cp $(sourcePath)/copilotd.service /lib/systemd/system/copilotd.service
	useradd -U -m -s /usr/bin/nologin copilot
	mkdir -p /etc/copilot
	chown -R copilot:copilot /etc/copilot
	cp $(sourcePath)/sudoers /etc/sudoers.d/copilot


