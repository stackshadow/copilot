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
    doDBEntry( QString originFunctionName );
    ~doDBEntry();

//    static doDBEntry*   ptr;

// reference count
public:
    void                incRefAtom( QString originFunctionName );
    static void         decRefAtom( QString originFunctionName, doDBEntry **p_dbEntry );
    int                 refCount();

#ifndef doDBEntry_C
    void                incRef();
    void                decRef( doDBEntry **p_dbEntry );
#endif

    bool                isWriteable( doDBEntry *dbEntry );

private:
    doDBEntry*          requestWrite();

public:
    static bool         connectionIDSet( doDBEntry **p_dbEntry, QString connectionID );
    QString             connectionID();
    doDBConnection*     connection();

    static bool         itemSet( doDBEntry **p_dbEntry, QString *tableName, QString *itemID, int *type, QString *displayName );
    void                item( QString* tableName, QString* itemID, int* type, QString* displayName );

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
    QString             itemDisplayName;

    QTreeWidgetItem*    treeItem;
    bool                treeItemEnabled;
};

#ifndef doDBEntry_C

    #define doDBEntry() doDBEntry( __PRETTY_FUNCTION__ )
    #define incRef() incRefAtom( __PRETTY_FUNCTION__ )
    #define decRef( entry ) doDBEntry::decRefAtom( __PRETTY_FUNCTION__, entry )

#endif


#endif
