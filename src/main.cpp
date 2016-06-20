/*  Copyright (C) 2016 by Martin Langlotz alias stackshadow

    This file is part of doDB.

    doDB is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    doDB is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with doDB.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <core/etInit.h>
#include <QApplication>
#include <QDir>

#include "jsonSettings/jsonSettings.h"
#include "doDBMainWindow/doDBMainWindow.h"

#include "db/etDBObjectTable.h"
#include "db/etDBObjectTableColumn.h"

#include "main.h"
#include "core/etDebug.h"

#include "doDBConnection/doDBConnections.h"

#include "doDBEntry/doDBEntry.h"

#include "doDBPlugin/doDBPlugins.h"
#include "doDBRelation/doDBRelationPlugin.h"
#include "doDBEntryEditor/doDBEntryPlugin.h"

doDBSettings        *doDBSettingsGlobal;
t_globalSettings    globalSettings;

/*
apt install pkg-config libjansson-dev libqt5core5a libqt5gui5 qt5-qmake qtbase5-dev qtbase5-dev-tools qttools5-dev-tools
additional ( not needed but usefull ):
apt install sqlitebrowser

evilib:
libblkid-dev libsqlite3-dev
*/

int main(int argc, char *argv[])
{
    etInit(argc,(const char**)argv);
    etDebugLevelSet( etID_LEVEL_DETAIL_DB );
    etDebugProgramNameSet( "doDB" );

// init the qt-application
    QApplication a(argc, argv);

// create our debugger
    new doDBDebug();

// okay, we load the settings
    doDBSettingsGlobal = new doDBSettings();

// create the list of all connections
    new doDBConnections();
    doDBConnections::ptr->connectionsLoad();

// create the global actual entry
    new doDBEntry();

// our plugins
    new doDBPlugins();
// default entryEditor
    doDBEntryPlugin *entryEditor = new doDBEntryPlugin();
    doDBPlugins::ptr->append( entryEditor );
// relations
    doDBRelationPlugin *dbRelation = new doDBRelationPlugin();
    doDBPlugins::ptr->append( dbRelation );



// the main window
    doDBMainWindow *mainWindow = new doDBMainWindow(NULL);

    return a.exec();
}
