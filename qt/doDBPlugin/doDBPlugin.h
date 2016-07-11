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

#ifndef doDBPlugin_H
#define doDBPlugin_H

#include "qt/doDBTree/doDBTree.h"
#include "doDBEntry/doDBEntry.h"
#include "doDBConnection/doDBConnection.h"

class doDBPlugin : public QObject {
    Q_OBJECT

public:
typedef enum messageID {
    msgNothing = 0,

    msgInitToolBar = 10,
    msgInitCentralView,
    msgInitStackedLeft,
    msgInitStackedRight,

    msgConnectionEditorShow = 50,
    msgConnectionEditorHide,
    msgConnectionCreated,
    msgConnectionChanged,
    msgConnectionDeleted,
    msgConnectionSelected,
    msgConnectionConnected,
    msgConnectionDisconnected,

    msgItemEditorShow = 70,
    msgItemEditorHide,
    msgItemCreated,
    msgItemChanged,
    msgItemDeleted,
    msgItemSelected,
    msgItemExpanded,
    msgItemCollapsed,

// special
    msgInitTree = 90,

    msgLast = 100
} messageID;

    public:
        doDBPlugin();
        virtual ~doDBPlugin();

        virtual QString     valueGet( QString valueName ){ return ""; };

        virtual void        prepareLayout( QString name, QLayout* layout ){ return; };

// future stuff
        virtual bool        recieveMessage( messageID type, void* payload ){ return true; }



    private:
        QString         pluginName;
        QString         pluginAuthor;
        QString         pluginDescription;
        bool            pluginEnabled;
};

#endif // doDBPlugin_H
