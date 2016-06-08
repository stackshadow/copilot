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

#include "doDBConnection.h"

#include <QDebug>
#include <QUuid>

#include "main.h"
#include "doDBDebug/doDBDebug.h"

#include "core/etDebug.h"
#include "db/etDBObjectTableColumn.h"
#include "db/etDBObjectFilter.h"
#include "db/etDBObjectValue.h"
#include "dbdriver/etDBDriver.h"
#include "dbdriver/etDBSQLite.h"

#include "doDBConnections.h"



doDBConnection::                    doDBConnection( connectionType type, const char *id, const char *displayName ){


// save
    this->type = type;

// id
    etStringAlloc( this->uuid );

// create uuid if needed
    if( id == NULL ){
        QString Quuid = QString( QUuid::createUuid().toString() );
        Quuid.remove( "{" );
        Quuid.remove( "}" );
        etStringCharSet( this->uuid, Quuid.toUtf8(), Quuid.length() );
    } else {
        etStringCharSet( this->uuid, id, strlen(id) );
    }

// save the connection name
    etStringAlloc( this->displayName );
    etStringCharSet( this->displayName, displayName, strlen(displayName) );


// create a etDBObject
    etDBObjectAlloc( this->dbObject );
    this->dbObjectLockID = "0";

//
    this->dbDriver = NULL;
    this->doDBVersion = "000";


// disable lock
    this->dbLocked = false;


}

doDBConnection::                    ~doDBConnection(){

// release dbObject
    if( this->dbObject != NULL ){
        etDBObjectFree( this->dbObject );
    }

}


// set / get connection infos

doDBConnection::connectionType doDBConnection::     typeGet(){
    return this->type;
}


void doDBConnection::               typeSet( connectionType type ){
    this->type = type;
}



const char* doDBConnection::        UUIDGet(){

    const char *tempString = NULL;
    etStringCharGet( this->uuid, tempString );
    return tempString;
}



const char* doDBConnection::        displayNameGet(){

    const char *tempString = NULL;
    etStringCharGet( this->displayName, tempString );
    return tempString;
}


void doDBConnection::               displayNameSet( const char* displayName ){
    etStringCharSet( this->displayName, displayName, strlen(displayName) );
}



QString doDBConnection::            fileNameGet(){
    return this->fileName;
}


void doDBConnection::               fileNameSet( QString fileName ){
    this->fileName = fileName;
}




// connection ?

bool doDBConnection::               connect(){

// is already connected ?
    if( this->isConnected() == true ) return true;

// displayName
    const char*         displayName = NULL;
    const char*         doDBCoreVersion = NULL;
    etDBObject*         dbObjectCore = NULL;

    etStringCharGet( this->displayName, displayName );

// SQLITE
    if( this->type == doDBConnection::CONN_SQLITE ){
    // check if there is an filename
        if( this->fileName.length() == 0 ) return false;

    // check if the driver is already there
        if( this->dbDriver != NULL ) return false;

    // allocate memory for the driver
        this->dbDriver = (etDBDriver*)malloc( sizeof(etDBDriver) );
        memset( this->dbDriver, 0, sizeof(etDBDriver) );

    // init the sqlite-driver
        etDBSQLiteDriverInit( this->dbDriver, this->fileName.toUtf8() );
        doDBDebug::ptr->print( QString("%1: Init SQLITE %2").arg(displayName).arg(this->fileName) );

    // setup the debugging output
        this->dbDriver->queryAcknowledge = doDBConnection::queryAck;

    }

// connect
    etDBDriverConnect( this->dbDriver );

// init dodb
// check if connected
    if( this->isConnected() == false ) return false;
    snprintf( etDebugTempMessage, etDebugTempMessageLen, "%1: Connected", displayName );
    etDebugMessage( etID_LEVEL_DETAIL_DB, etDebugTempMessage );

    dbObjectCore = doDBConnections::ptr->dbObjectCore;

// load the version of the db
    if( this->dbDoDBValueGet("doDBVersion",&doDBCoreVersion) != true ){
        this->doDBVersion = doDBCoreVersion;
    }
    snprintf( etDebugTempMessage, etDebugTempMessageLen, "%1: Version of doDB: %2", displayName, this->doDBVersion.toUtf8() );
    etDebugMessage( etID_LEVEL_DETAIL_DB, etDebugTempMessage );


// load dbObject from db
    this->dbObjectLoad();



// test
    //this->relatedTableAppend( "storage", "storage", "parentID", "uuid" );
    //this->dbRelationSave();


    return true;
}


bool doDBConnection::               isConnected(){

    if( etDBDriverIsConnect(this->dbDriver) == etID_YES ){
        return true;
    } else {
        return false;
    }
}




etID_STATE doDBConnection::         queryAck( etDBDriver *dbDriver, etDBObject *dbObject, etString *sqlquery ){

// vars
    const char *queryChar;
    etStringCharGet( sqlquery, queryChar );

    doDBDebug::ptr->print( queryChar );

    return etID_YES;
}




bool doDBConnection::               tableAppend( const char *tableName ){
    if( tableName == NULL ) return false;

// pick the table
    if( etDBObjectTablePick(this->dbObject,tableName) != etID_YES ){
        return false;
    }

// add an table
    if( etDBDriverTableAdd( this->dbDriver, this->dbObject ) != etID_YES ){
        return false;
    }

    return true;
}


bool doDBConnection::               tableDisplayNameGet( const char *tableName, const char **tableDisplayName ){

    if( etDBObjectTablePick( this->dbObject, tableName ) == etID_YES ){

        __etDBObjectTableDisplayNameGet( this->dbObject, "", tableDisplayName );
        return true;
    }

    return false;
}



bool doDBConnection::               columnAppend( const char *tableName, const char *columnName ){
    if( tableName == NULL ) return false;
    if( columnName == NULL ) return false;

// pick the table
    if( etDBObjectTablePick(this->dbObject,tableName) != etID_YES ){
        return false;
    }

// pick te column
    if( etDBObjectTableColumnPick(this->dbObject,columnName) != etID_YES ){
        return false;
    }

// add the column
    if( etDBDriverColumnAdd( this->dbDriver, this->dbObject ) != etID_YES ){
        return false;
    }

    return true;
}


bool doDBConnection::               columnsGet( const char *tableName, void *userdata, void (*callback)( void *userdata, const char *columnName, const char *columnDisplayName) ){
    if( tableName == NULL ) return false;
    if( callback == NULL ) return false;

// pick the table
    if( etDBObjectTablePick(this->dbObject,tableName) != etID_YES ){
        return false;
    }

// vars
    const char          *columnName;
    const char          *columnDisplayName;

    etDBObjectIterationReset( this->dbObject );
    while( etDBObjectTableColumnNext( this->dbObject, columnName ) == etID_YES ){

        // get display
        etDBObjectTableColumnDisplayNameGet( this->dbObject, "", columnDisplayName );


        callback( userdata, columnName, columnDisplayName );

    }


    return true;
}



bool doDBConnection::               dbLockGet(){

// check if already locked and complain about it !
    if( this->dbLocked ){
        doDBDebug::ptr->print( "Datenbank is vom Programm selbst gesperrt, bitte später noch einmal probieren oder sich darüber hinweg setzen ( Gefährlich !!! )" );
        return false;
    }

    this->dbLocked = true;
    return true;
}


bool doDBConnection::               dbLockRelease(){
    this->dbLocked = false;
}




bool doDBConnection::               dbObjectLoad(){

// vars
    const char*         displayName = this->displayNameGet();
    etDBObject*         dbObjectCore = NULL;
    json_error_t        jsonError;
    const char*         jsonDoDBObject = NULL;

// get the dbObject as jsonString
    if( this->dbDoDBValueGet( "doDBObject", &jsonDoDBObject ) != true ){
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "could not load/create doDBObject, there would be nothing to show" );
        etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
        return false;
    }

// load the json file
    json_t *newdoDBObject = json_loads( jsonDoDBObject, JSON_PRESERVE_ORDER, &jsonError );
    if( newdoDBObject == NULL ){
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "JSON ERROR: %s", jsonError.text );
        etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
        return false;
    } else {
        this->dbObject->jsonRootObject = newdoDBObject;
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "doDBObject loeded from db"  );
        etDebugMessage( etID_LEVEL_DETAIL, etDebugTempMessage );
        return true;
    }


    return false;
}


bool doDBConnection::               dbObjectSave(){

// vars
    const char*         displayName = this->displayNameGet();
    etDBObject*         dbObjectCore = NULL;


//dump json to char
    const char *jsonDump = json_dumps( this->dbObject->jsonRootObject, JSON_PRESERVE_ORDER | JSON_INDENT(4) );

    this->dbDoDBValueSet( "doDBObject", jsonDump );

    free( (void*)jsonDump );



}





etID_STATE doDBConnection::         columnDisplayValueGet( const char *tableName, const char **displayColumn ){

// set the column default to NULL
    *displayColumn = NULL;


// first try to pick the table inside the dbObject
    if( etDBObjectTablePick( this->dbObject, tableName ) != etID_YES ){

        doDBDebug::ptr->print( __PRETTY_FUNCTION__ + QString(": Table '%1' dont exist in connection!").arg(tableName) );

        *displayColumn = NULL;
        return etID_STATE_NODATA;
    }

    if( __etDBObjectTableColumnMainGet(this->dbObject,displayColumn) != etID_YES ){

        doDBDebug::ptr->print( __PRETTY_FUNCTION__ + QString(": Table '%1' dont have an displayColumn").arg(tableName) );

        if( __etDBObjectTableColumnPrimaryGet(this->dbObject,displayColumn) != etID_YES ){
            doDBDebug::ptr->print( __PRETTY_FUNCTION__ + QString(": Table '%1' dont have an primary Column").arg(tableName) );
            *displayColumn = NULL;
            return etID_STATE_NODATA;
        }

    }

    return etID_YES;
}


bool doDBConnection::               dbDataGet( const char *tableName, void *userdata, void (*callback)( void *userdata, const char *tableName, const char *connID, const char *primaryValue, const char *displayValue) ){
// check if connected
    if( this->isConnected() == false ){
        return false;
    }

// vars
    const char      *connID = NULL;
    const char      *tableDisplayName = NULL;
    const char      *primaryColumn = NULL;
    const char      *primaryColumnValue = NULL;
    const char      *displayColumn = NULL;
    const char      *displayColumnValue = NULL;

// pick table
    if( etDBObjectTablePick( this->dbObject, tableName ) != etID_YES ){
        doDBDebug::ptr->print( __PRETTY_FUNCTION__ + QString(": table '%1' not found in dbObject").arg(tableName) );
        return false;
    }

// get primary column
   if( etDBObjectTableColumnPrimaryGet( this->dbObject, primaryColumn ) != etID_YES ){
        doDBDebug::ptr->print( __PRETTY_FUNCTION__ + QString(": table '%1' has no primary column").arg(tableName) );
        return false;
    }

// get the column for display name
    if( this->columnDisplayValueGet(tableName,&displayColumn) != etID_YES ){
        doDBDebug::ptr->print( __PRETTY_FUNCTION__ + QString(": table '%1' has no display column").arg(tableName) );
        return false;
    }

// get uuid-char
    etStringCharGet( this->uuid, connID );


// clear the filter
    etDBObjectFilterClear( this->dbObject );

// get data ( query )
    etDBDriverDataGet( this->dbDriver, this->dbObject );

// iterate through data
    while( etDBDriverDataNext( this->dbDriver, this->dbObject ) == etID_YES ){


        if( etDBObjectValueGet( this->dbObject, primaryColumn, primaryColumnValue ) != etID_YES ){
            continue;
        }


        if( etDBObjectValueGet( this->dbObject, displayColumn, displayColumnValue ) != etID_YES ){
            doDBDebug::ptr->print( __PRETTY_FUNCTION__ + QString(": table '%1' has no display column use primaryKeyColumn").arg(tableName) );
            displayColumnValue = primaryColumnValue;
        }

        callback( userdata, tableName, connID, primaryColumnValue, displayColumnValue );

    }


}


bool doDBConnection::               dbDataGet( const char *table, const char *tableItemID ){
// check if connected
    if( this->isConnected() == false ){
        return false;
    }

// vars
    const char      *connID = NULL;
    const char      *tableDisplayName = NULL;
    const char      *primaryColumn = NULL;
    const char      *primaryColumnValue = NULL;
    const char      *columnNameActual = NULL;
    const char      *columnValueActual = NULL;
    const char      *srcColumn = NULL;
    const char      *srcColumnValue = NULL;
    const char      *relColumn = NULL;


// pick table
    if( etDBObjectTablePick( this->dbObject, table ) != etID_YES ){
        return false;
    }

// get primary column
   if( etDBObjectTableColumnPrimaryGet( this->dbObject, primaryColumn ) != etID_YES ){
        return false;
    }

// create filter
    etDBObjectFilterClear( this->dbObject );
    etDBObjectFilterAdd(  this->dbObject, 1, etDBFILTER_OP_AND, primaryColumn, etDBFILTER_TYPE_EQUAL, tableItemID );

// run query
    etDBDriverDataGet( this->dbDriver, this->dbObject );

// iterate through data
    if( etDBDriverDataNext( this->dbDriver, this->dbObject ) == etID_YES ){
        return true;
    }


    return false;
}


bool doDBConnection::               dbDataGet( const char *table, const char *tableItemID, void *userdata, void (*callback)( void *userdata, const char *columnName, const char *columnValue ) ){
// check if connected
    if( this->isConnected() == false ){
        return false;
    }

// vars
    const char      *connID = NULL;
    const char      *tableDisplayName = NULL;
    const char      *primaryColumn = NULL;
    const char      *primaryColumnValue = NULL;
    const char      *columnNameActual = NULL;
    const char      *columnValueActual = NULL;
    const char      *srcColumn = NULL;
    const char      *srcColumnValue = NULL;
    const char      *relColumn = NULL;


// pick table
    if( etDBObjectTablePick( this->dbObject, table ) != etID_YES ){
        return false;
    }

// get primary column
   if( etDBObjectTableColumnPrimaryGet( this->dbObject, primaryColumn ) != etID_YES ){
        return false;
    }

// create filter
    etDBObjectFilterClear( this->dbObject );
    etDBObjectFilterAdd(  this->dbObject, 1, etDBFILTER_OP_AND, primaryColumn, etDBFILTER_TYPE_EQUAL, tableItemID );

// run query
    etDBDriverDataGet( this->dbDriver, this->dbObject );

// iterate through data
    while( etDBDriverDataNext( this->dbDriver, this->dbObject ) == etID_YES ){

        etDBObjectIterationReset(this->dbObject);
        while( etDBObjectTableColumnNext(this->dbObject,columnNameActual) == etID_YES ){

            if( etDBObjectValueGet( this->dbObject, columnNameActual, columnValueActual ) != etID_YES ){
                continue;
            }

            callback( userdata, columnNameActual, columnValueActual );

        }



    }


    return true;


}


bool doDBConnection::               dbDataNew( const char *tableName ){

// pick table
    if( etDBObjectTablePick( this->dbObject, tableName ) != etID_YES ){
        return false;
    }

// table add
    etDBDriverDataAdd( this->dbDriver, this->dbObject );

    return true;
}


bool doDBConnection::               dbDataChange( const char *tableName ){

// pick table
    if( etDBObjectTablePick( this->dbObject, tableName ) != etID_YES ){
        return false;
    }

// change data
    etDBDriverDataChange( this->dbDriver, this->dbObject );

    return true;
}




bool doDBConnection::               dbDoDBValueGet( const char *key, const char **value ){

// vars
    const char      *displayName = this->displayNameGet();
    etDBObject      *dbObjectCore = doDBConnections::ptr->dbObjectCore;


// get doDBObject from db
    etDBObjectTablePick( dbObjectCore, "doDB" );
    etDBObjectFilterClear( dbObjectCore );
    etDBObjectFilterAdd( dbObjectCore, 0, etDBFILTER_OP_AND, "key", etDBFILTER_TYPE_EQUAL, key );
    etDBDriverDataGet( this->dbDriver, dbObjectCore );
    if( etDBDriverDataNext( this->dbDriver, dbObjectCore ) == etID_YES ){

        if( __etDBObjectValueGet( dbObjectCore, "value", value ) == etID_YES ){
            return true;
        } else {
            snprintf( etDebugTempMessage, etDebugTempMessageLen, "%s: value of %s is empty", displayName, key );
            etDebugMessage( etID_LEVEL_WARNING, etDebugTempMessage );
            return false;
        }

    } else {
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "%s: no %s in doDBTable, create an empty one", displayName, key );
        etDebugMessage( etID_LEVEL_WARNING, etDebugTempMessage );

        etDBObjectValueSet( dbObjectCore, "key", key );
        etDBObjectValueSet( dbObjectCore, "value", "" );
        if( etDBDriverDataAdd(this->dbDriver, dbObjectCore) != etID_YES ){
            return false;
        }
        return true;
    }

    return false;
}


bool doDBConnection::               dbDoDBValueSet( const char *key, const char *value ){
// check
    if( key == NULL ) return false;
    if( value == NULL ) return false;

// vars
    etDBObject*         dbObjectCore = NULL;

// get dbObjects
    dbObjectCore = doDBConnections::ptr->dbObjectCore;

// get doDBObject from db
    etDBObjectTablePick( dbObjectCore, "doDB" );

    etDBObjectValueClean( dbObjectCore );
    etDBObjectValueSet( dbObjectCore, "key", key );

//dump json to char
    etDBObjectValueSet( dbObjectCore, "value", value );

// change the value in the db
    etDBDriverDataChange( this->dbDriver, dbObjectCore );
}




QString doDBConnection::            itemPrimaryColumnName(){

    const char *primaryKeyColumn = NULL;
    if( etDBObjectTableColumnPrimaryGet( this->dbObject, primaryKeyColumn ) != etID_YES ){
        return QString();
    }

    return QString(primaryKeyColumn);
}


void doDBConnection::               itemValueClean(){
    etDBObjectValueClean(this->dbObject);
}


void doDBConnection::               itemValueSet( QString columnName, QString columnValue ){
    etDBObjectValueSet( this->dbObject, columnName.toUtf8(), columnValue.toUtf8() );
}


bool doDBConnection::               itemValueNew(){
    if( etDBDriverDataAdd( this->dbDriver, this->dbObject ) == etID_YES ){
        return true;
    }
    return false;
}


bool doDBConnection::               itemValueSave(){
    if( etDBDriverDataChange( this->dbDriver, this->dbObject ) == etID_YES ){
        return true;
    }
    return false;
}








doDBConnection* doDBConnection::    find( QList<doDBConnection*> *connectionList, QString uuid ){

// vars
    doDBConnection      *connection = NULL;
    int                 connectionIndex = 0;
    int                 connectionLen = connectionList->length();

//
    for( connectionIndex = 0; connectionIndex < connectionLen; connectionIndex++ ){
        connection = connectionList->at(connectionIndex);

        if( connection->UUIDGet() == uuid ){
            return connection;
        }
    }

// return "not-found"
    return NULL;
}



