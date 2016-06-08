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

#ifndef doDBRelationPlugin_H
#define doDBRelationPlugin_H

#include <QString>
#include <QPushButton>
#include <QLayout>

#include "core/etIDState.h"
#include "string/etString.h"
#include "string/etStringChar.h"
#include "evillib-extra_depends.h"
#include "db/etDBObject.h"
#include "db/etDBObjectTableColumn.h"
#include "dbdriver/etDBDriver.h"

#include "doDBPlugin/doDBPlugin.h"
#include "doDBTree/doDBTree.h"
#include "doDBRelation.h"
#include "doDBRelation/doDBRelationEditor.h"



class doDBRelationPlugin :
public doDBPlugin {
    Q_OBJECT

public:
                            doDBRelationPlugin();
                            ~doDBRelationPlugin();

// overloaded plugin functions
// prepare stuff
//  void                    prepareDashboard( QLayout *dashboardLayout );
    void                    prepareTree( doDBtree *dbTree );
    void                    prepareItemView( QLayout *itemViewLayout );

// events
    bool                    dbTreeItemClicked( QTreeWidgetItem * item, int column );
    bool                    dbTreeItemExpanded( QTreeWidgetItem * item );
    bool                    dbTreeItemCollapsed( QTreeWidgetItem * item );




private slots:
    void                    editorShow();
    void                    editorClosed();


private:
    doDBRelation            *dbRelation;
    doDBRelationEditor      *dbRelationEditor;
    QString                 connectionID;

    QPushButton             *btnEditRelations;
    QPushButton             *btnConnectRelation;

// db tree
    doDBtree                *dbTree;
    int                     dbTreeItemTypeRelatedTable;

};






#endif


