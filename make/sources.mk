#!/bin/make
# An common makefile-based build system
#	Copyright (C) 2016 by Martin Langlotz
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
sources =

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


sourcePath	= $(PWD)
buildPath   = /tmp/dodb/build
targetPath  = /tmp/dodb/target

# this defines the sources for the target binary
sources     += ./src/jsonSettings/jsonSettings.cpp
sources     += ./src/evillib.c

# sources which contains qt-stuff
sourcesQT   += ./src/main.cpp

sourcesQT   += ./qt/doDBDebug/doDBDebug.cpp

sourcesQT   += ./qt/doDBSettings/doDBSettings.cpp

sourcesQT   += ./qt/doDBMainWindow/doDBMainWindow.cpp
sourcesUI   += ./qt/doDBMainWindow/doDBMainWindowUi.ui

sourcesQT   += ./qt/doDBPlugin/doDBPlugins.cpp
sourcesQT   += ./qt/doDBPlugin/doDBPlugin.cpp


sourcesQT   += ./qt/doDBConnection/doDBConnections.cpp
sourcesQT   += ./qt/doDBConnection/doDBConnection.cpp

sourcesQT   += ./qt/doDBTableEditor/doDBTableEditor.cpp
sourcesUI   += ./qt/doDBTableEditor/doDBTableEditor.ui

sourcesQT   += ./qt/doDBConnection/doDBConnectionEditor.cpp
sourcesUI   += ./qt/doDBConnection/doDBConnectionEditor.ui

sourcesQT   += ./qt/doDBTree/doDBTree.cpp

sourcesQT   += ./qt/doDBRelation/doDBRelationPlugin.cpp
sourcesQT   += ./qt/doDBRelation/doDBRelation.cpp
sourcesQT   += ./qt/doDBRelation/doDBRelationEditor.cpp
sourcesUI   += ./qt/doDBRelation/doDBRelationEditor.ui

sourcesQT   += ./qt/doDBFile/doDBFile.cpp
sourcesQT   += ./qt/doDBFile/doDBTableFoldersEditor.cpp
sourcesUI   += ./qt/doDBFile/doDBTableFoldersEditor.ui

sourcesQT   += ./qt/doDBEntryEditor/doDBEntryPlugin.cpp
sourcesQT   += ./qt/doDBEntryEditor/doDBEntryEditor.cpp
sourcesUI   += ./qt/doDBEntryEditor/doDBEntryEditor.ui

# additional sources which compiles to an shared object
sourcesLib  +=
