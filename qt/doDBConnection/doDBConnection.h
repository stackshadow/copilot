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

#ifndef DODBCONNECTION_H
#define DODBCONNECTION_H

#include <QList>
#include <QString>
#include <QHash>
#include <QTreeWidget>
#include <QMap>

#include "core/etIDState.h"
#include "string/etString.h"
#include "string/etStringChar.h"
#include "evillib-extra_depends.h"
#include "db/etDBObject.h"
#include "db/etDBObjectTableColumn.h"
#include "dbdriver/etDBDriver.h"

#include "doDBTree/doDBTree.h"


class doDBConnection
{

public:
    enum connectionType {
        CONN_NOTHING = 0,
        CONN_SQLITE,
        CONN_POSTGRES
    };

public:
    doDBConnection( connectionType type, const char *id, const char *displayName );
    ~doDBConnection();


public:
// set / get connection infos
    connectionType          typeGet();
    void                    typeSet( connectionType type );

    const char*             UUIDGet();

    const char*             displayNameGet();
    void                    displayNameSet( const char* displayName );

    QString                 fileNameGet();
    void                    fileNameSet( QString fileName );

// connection ?
public:
    bool                    connect();
    bool                    isConnected();
    bool                    disconnect();

// some basic
public:
    static etID_STATE       queryAck( etDBDriver *dbDriver, etDBObject *dbObject, etString *sqlquery );

// tables
public:
    bool                    tableAppend( const char *tableName );
    bool                    tableDisplayNameGet( const char *tableName, const char **tableDisplayName );

// columns
public:
    bool                    columnAppend( const char *tableName, const char *columnName );
    bool                    columnsGet( const char *tableName, void *userdata, void (*callback)( void *userdata, const char *columnName, const char *columnDisplayName) );

// locking db-stuff
    bool                    dbLockGet();
    bool                    dbLockRelease();

// db-functions
    bool                    dbObjectLoad();
    bool                    dbObjectSave();

// data
    bool                    dbDataGet( const char *tableName, void *userdata, void (*callback)( void *userdata, const char *tableName, const char *connID, const char *primaryValue, const char *displayValue) );
    bool                    dbDataGet( const char *table, const char *tableItemID );
    bool                    dbDataGet( const char *table, const char *tableItemID, void *userdata, void (*callback)( void *userdata, const char *columnName, const char *columnValue ) );
    bool                    dbDataNew( const char *tableName );
    bool                    dbDataChange( const char *tableName );
    bool                    dbDataDelete( const char *table, const char *tableItemID );


    bool                    dbDoDBValueGet( const char *key, const char **value );
    bool                    dbDoDBValueSet( const char *key, const char *value );


    etID_STATE              columnDisplayValueGet( const char *tableName, const char **displayColumn );

// get an etDBObject from an entry
    QString                 itemPrimaryColumnName();
    void                    itemValueClean();
    void                    itemValueSet( QString columnName, QString columnValue );
    bool                    itemValueNew();
    bool                    itemValueSave();


    bool                    dbTableFolderLoad();
    bool                    dbTableFolderSave();
    bool                    dbTableFolderAppend( const char *tableName, const char *os, const char *folder );
    bool                    folderGet( const char *tableName, const char **folderName );




// static functions
    static doDBConnection*  find( QList<doDBConnection*> *connectionList, QString uuid );


private:
    connectionType          type;
    etString                *uuid;
    etString                *displayName;

public:
    QString                 fileName;
    QString                 hostname;
    QString                 hostip;
    QString                 port;
    QString                 database;
    QString                 username;
    QString                 password;


public:
// database stuff
    etDBDriver              *dbDriver;
    etDBObject              *dbObject;
    QString                 dbObjectLockID;
    QString                 doDBVersion;

// actual data stuff
    bool                    dbLocked;

};

#endif // DODBCONNECTIONS_H
