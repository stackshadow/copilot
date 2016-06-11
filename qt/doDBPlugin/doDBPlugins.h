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

class doDBPlugins
{

public:
                            doDBPlugins();
    virtual                 ~doDBPlugins();

// append / remove
    void                    append( doDBPlugin *dbPlugin );

// prepare stuff
    void                    eventPrepareToolBar( QLayout *layout );
    void                    eventPrepareTree( doDBtree *dbTree );
    void                    eventPrepareItemView( QLayout *layout );

// all events
    void                    eventTreeItemClicked( QTreeWidgetItem * item, int column );
    void                    eventTreeItemExpanded( QTreeWidgetItem * item );
    void                    eventTreeItemCollapsed( QTreeWidgetItem * item );
    void                    eventItemChanged( const char * columnName, const char * newColumnValue );


public:
    static doDBPlugins      *ptr;

private:
    QList<doDBPlugin*>      pluginList;



};







#endif

