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

#ifndef doDBRelation_H
#define doDBRelation_H

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

#include "doDBTree/doDBTree.h"
#include "doDBConnection/doDBConnection.h"


class doDBRelation {


public:
                            doDBRelation();
                            ~doDBRelation();

// relations
    bool                    relationImport( const char *jsonString );
    bool                    relationExport( const char **jsonString );

    bool                    relationAppend( const char *srcTable, const char *relatedTable, const char *srcColumn, const char *relatedColumn );
    bool                    relationRemove( const char *srcTable, const char *relatedTable, const char *srcColumn, const char *relatedColumn );

    void                    relationGetReset();
    bool                    relationGetNext( const char **srcTable, const char **srcColumn, const char **relatedTable, const char **relatedColumn );
    bool                    relatedTableGetNext( const char *srcTable, const char **srcColumn, const char **relatedTable, const char **relatedColumn );
    bool                    relatedTableFindNext( const char *srcTable, const char **srcColumn, const char *relatedTable, const char **relatedColumn );
    bool                    relatedTableFindNext( const char *srcTable, const char **srcColumn, const char **relatedTable, const char **relatedColumn );

    bool                    dbRelationLoad( doDBConnection* connection );
    bool                    dbRelationSave( doDBConnection* connection );
    bool                    dbDataGet( const char *connectionID, const char *srcTable, const char *srcTableItemID, const char *relatedTable, void *userdata, void (*callback)( void *userdata, const char *tableName, const char *connID, const char *primaryValue, const char *displayValue) );
    bool                    dbDataRead( const char *connectionID, const char *srcTable, const char *srcTableItemID, const char *relatedTable );

private:

// remember which connection was loaded
    doDBConnection*         loadedConnection;

// basic
    json_t*                 jsonRelation;
    void*                   jsonSrcTableItrerator;
    json_t*                 jsonRelationSrcTable;
    int                     jsonRelationIndex;


};




#endif // DODBMAINWINDOW_H

