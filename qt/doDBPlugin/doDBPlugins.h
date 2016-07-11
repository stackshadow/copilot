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
#ifndef doDBPlugins_H
#define doDBPlugins_H

#include <QMainWindow>
#include <QHBoxLayout>


#include "doDBPlugin/doDBPlugin.h"
#include "doDBConnection/doDBConnection.h"
#include "doDBTree/doDBTree.h"
#include "doDBEntryEditor/doDBEntryEditor.h"

#ifdef __gnu_linux__
    #define pluginPath1 "./plugins"
    #define pluginPath2 "~/doDB/plugins"
    #define pluginPath3 "/usr/share/doDB/plugins"
#endif

class doDBPlugins
{

typedef struct messageListener_s {
    doDBPlugin*                 target;
    doDBPlugin::messageID       type;
    bool                        onlyOnce;
} messageListener;



public:
                                doDBPlugins();
    virtual                     ~doDBPlugins();

    void                        load();

// append / remove
    void                        append( doDBPlugin *dbPlugin );

// prepare stuff
    void                        prepareLayout( QString name, QLayout* layout );
    void                        eventPrepareTree( doDBtree *dbTree );


// message
    void                        registerListener( doDBPlugin *plugin, doDBPlugin::messageID type, bool fireOnce = false );
    void                        sendBroadcast( doDBPlugin::messageID type, void *payload );

public:
    static doDBPlugins          *ptr;

private:
    int                         lastMessageID;
    QList<doDBPlugin*>          pluginList;
    QList<messageListener*>     messageList;


};







#endif

