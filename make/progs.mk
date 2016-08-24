#!/bin/make

CC          = $(CROSS_COMPILE)gcc
CPP         = $(CROSS_COMPILE)g++
RM			= rm -fv
MOC         = /usr/lib/x86_64-linux-gnu/qt5/bin/moc
UIC			= /usr/lib/x86_64-linux-gnu/qt5/bin/uic
MKDIR		= mkdir -v -p
ECHO		= echo
CP          = cp -v

ifeq ($(OS),Windows_NT)
MOC     	= /usr/win32/qt/bin/moc
UIC     	= /usr/win32/qt/bin/uic
endif
