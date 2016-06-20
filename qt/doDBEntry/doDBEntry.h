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

// This class represents the selected entry to access it globally

#ifndef doDBEntry_H
#define doDBEntry_H

#include <QString>
#include <QTreeWidgetItem>
#include "qt/doDBConnection/doDBConnection.h"

#include "jansson.h"

class doDBEntry
{

public:
    doDBEntry();
    ~doDBEntry();

//    static doDBEntry*   ptr;

// reference count
public:
    void                incRef();
    void                decRef();
    int                 refCount();

    bool                isWriteable( doDBEntry *dbEntry );

private:
    doDBEntry*          requestWrite();

public:
    static bool         connectionIDSet( doDBEntry **p_dbEntry, QString connectionID );
    QString             connectionID();
    doDBConnection*     connection();

    static bool         itemSet( doDBEntry **p_dbEntry, QString tableName, QString itemID, int type );
    void                item( QString* tableName, QString* itemID, int* type );

    static bool         treeWidgetItemSet( doDBEntry **p_dbEntry, QTreeWidgetItem* treeItem );
    QTreeWidgetItem*    treeWidgetItem();

    static bool         treeWidgetItemEnabledSet( doDBEntry **p_dbEntry, bool enabled );
    bool                treeWidgetItemEnabled();



private:
    int                 referenceCount;


    QString             connectionIdent;
    doDBConnection*     dbConnection;
    QString             itemTableName;
    QString             itemID;
    int                 itemType;

    QTreeWidgetItem*    treeItem;
    bool                treeItemEnabled;
};




#endif
