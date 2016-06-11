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
#include "doDBPlugin/doDBPlugins.h"
#include "doDBTree/doDBTree.h"
#include "doDBRelation.h"
#include "doDBRelation/doDBRelationEditor.h"



class doDBRelationPlugin :
public doDBPlugin {
    Q_OBJECT

public:
                            doDBRelationPlugin();
                            ~doDBRelationPlugin();

// overload
    void                    prepareTree( doDBtree *dbTree );
    void                    prepareItemView( QLayout *layout );

    bool                    dbTreeItemClicked( QTreeWidgetItem * item, int column );
    bool                    dbTreeItemExpanded( QTreeWidgetItem * item );
    bool                    dbTreeItemCollapsed( QTreeWidgetItem * item );


private slots:
    void                    editorShow();
    void                    editorClosed();
    void                    connectionStart();
    void                    connectionCheck();
    void                    connectionCancel();
    void                    connectionSave();


private:
    doDBRelation*           dbRelation;
    doDBRelationEditor*     dbRelationEditor;
    QString                 connectionID;

// db tree
    doDBtree*               dbTree;
    int                     dbTreeItemTypeRelatedTable;

// connection
    bool                    connectionMode;
    QLineEdit*              srcInfos;
    QPushButton*            btnEditRelations;
    QPushButton*            btnConnectRelation;
    QHBoxLayout*            connectToTablesLayout;
    QPushButton*            btnConnectRelationSave;
    QPushButton*            btnConnectRelationCancel;
    QLineEdit*              connectionTables;

// remember thinks
    QTreeWidgetItem*        srcItem;
    QString                 srcDislpayName;
    QString                 srcTable;
    QString                 srcItemID;
    QString                 srcColumn;

    bool                    relSelected;
    QString                 relTable;
    QString                 relItemID;
    QString                 relColumn;

};






#endif


