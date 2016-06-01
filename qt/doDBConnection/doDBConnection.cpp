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
#include "db/etDBObjectTableColumn.h"
#include "db/etDBObjectFilter.h"
#include "db/etDBObjectValue.h"
#include "dbdriver/etDBDriver.h"
#include "dbdriver/etDBSQLite.h"




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

    this->dbTableRelations = NULL;

    this->dbTableFolders = NULL;

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


// dbobjects


bool doDBConnection::               dbObjectGet( etDBObject **dbObject, QString *lockID ){

// get lock
    if( ! this->dbObjectLock(lockID) ){
        *dbObject = NULL;
        return false;
    }

// return
    *dbObject = this->dbObject;
    return true;
}


bool doDBConnection::               dbObjectLock( QString *lockID ){

// is unlocked
    if( this->dbObjectLockID == "0" ){
    // generate random
        this->dbObjectLockID = QUuid::createUuid().toString();

        doDBDebug::ptr->print( QString("%1: Get lock with id '%2' !").arg(__PRETTY_FUNCTION__).arg(this->dbObjectLockID) );

    // return
        *lockID = this->dbObjectLockID;
        return true;
    }

// locked from us
    if( this->dbObjectLockID == *lockID ){
        return true;
    }

// locked from somebody else
    doDBDebug::ptr->print( QString("%1: object is already locked from id '%2' !").arg(__PRETTY_FUNCTION__).arg(this->dbObjectLockID) );
    return false;

}


bool doDBConnection::               dbObjectUnLock( QString lockID ){

// is unlocked
    if( this->dbObjectLockID == "0" ){
        return true;
    }

// locked from us
    if( this->dbObjectLockID == lockID ){
        doDBDebug::ptr->print( QString("%1: unlock id '%2' !").arg(__PRETTY_FUNCTION__).arg(this->dbObjectLockID) );
        this->dbObjectLockID = "0";
        return true;
    }

// locked from somebody else
    doDBDebug::ptr->print( QString(__PRETTY_FUNCTION__) + ": dbObject is locked from somebody else, can not unlock !" );
    return false;
}




// connection ?

bool doDBConnection::               connect(){

// is already connected ?
    if( this->isConnected() == true ) return true;

// displayName
    const char          *displayName = NULL;
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
    doDBDebug::ptr->print( QString("%1: Connected").arg(displayName) );

// get version of db
    etDBObjectTablePick( doDBCore->dbObjectCore, "doDB" );
    etDBObjectFilterClear( doDBCore->dbObjectCore );
    etDBObjectFilterAdd( doDBCore->dbObjectCore, 0, etDBFILTER_OP_AND, "key", etDBFILTER_TYPE_EQUAL, "doDBVersion" );

    etDBDriverDataGet( this->dbDriver, doDBCore->dbObjectCore );
    if( etDBDriverDataNext( this->dbDriver, doDBCore->dbObjectCore ) == etID_YES ){
        const char *version = NULL;
        if( etDBObjectValueGet( doDBCore->dbObjectCore, "value", version ) == etID_YES ){
            this->doDBVersion = version;
        }
    } else {
        // doDB-Stuff dont exist, we need to create it
        etDBObjectTablePick( doDBCore->dbObjectCore, "doDB" );
        etDBDriverTableAdd( this->dbDriver, doDBCore->dbObjectCore );
        etDBObjectTablePick( doDBCore->dbObjectCore, "doDBRelations" );
        etDBDriverTableAdd( this->dbDriver, doDBCore->dbObjectCore );
        etDBObjectTablePick( doDBCore->dbObjectCore, "doDBLog" );
        etDBDriverTableAdd( this->dbDriver, doDBCore->dbObjectCore );
        etDBObjectTablePick( doDBCore->dbObjectCore, "doDBFiles" );
        etDBDriverTableAdd( this->dbDriver, doDBCore->dbObjectCore );
        etDBObjectTablePick( doDBCore->dbObjectCore, "doDBLinks" );
        etDBDriverTableAdd( this->dbDriver, doDBCore->dbObjectCore );

    }
    doDBDebug::ptr->print( QString("%1: Version of doDB: %2").arg(displayName).arg(this->doDBVersion) );


// load dbObject from db
    this->dbObjectLoad();

// load relation
    this->dbRelationLoad();



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
    const char *displayName = this->displayNameGet();

// get doDBObject from db
    etDBObjectTablePick( doDBCore->dbObjectCore, "doDB" );
    etDBObjectFilterClear( doDBCore->dbObjectCore );
    etDBObjectFilterAdd( doDBCore->dbObjectCore, 0, etDBFILTER_OP_AND, "key", etDBFILTER_TYPE_EQUAL, "doDBObject" );
    etDBDriverDataGet( this->dbDriver, doDBCore->dbObjectCore );
    if( etDBDriverDataNext( this->dbDriver, doDBCore->dbObjectCore ) == etID_YES ){

        const char *jsonDoDBObject = NULL;
        if( etDBObjectValueGet( doDBCore->dbObjectCore, "value", jsonDoDBObject ) == etID_YES ){
            json_error_t jsonError;
            json_t *newdoDBObject = json_loads( jsonDoDBObject, JSON_PRESERVE_ORDER, &jsonError );

            if( jsonError.line > 0 ){
                doDBDebug::ptr->print( jsonError.text );
            } else {
                this->dbObject->jsonRootObject = newdoDBObject;
                doDBDebug::ptr->print( QString("%1: tables loaded from db").arg(displayName) );
            }
        } else {
            doDBDebug::ptr->print( QString("%1: no doDBObject in table, nothing will be shown of this db !").arg(displayName) );
            etDBObjectValueSet( doDBCore->dbObjectCore, "key", "doDBObject" );
            etDBDriverDataAdd( this->dbDriver, doDBCore->dbObjectCore );
            return false;
        }

    } else {
        doDBDebug::ptr->print( QString("%1: no doDBObject in table, nothing will be shown of this db !").arg(displayName) );
        etDBObjectValueSet( doDBCore->dbObjectCore, "key", "doDBObject" );
        etDBDriverDataAdd( this->dbDriver, doDBCore->dbObjectCore );
        return false;
    }

}


bool doDBConnection::               dbObjectSave(){

// vars
    const char *displayName = this->displayNameGet();

// get doDBObject from db
    etDBObjectTablePick( doDBCore->dbObjectCore, "doDB" );

    etDBObjectValueClean( doDBCore->dbObjectCore );
    etDBObjectValueSet( doDBCore->dbObjectCore, "key", "doDBObject" );

//dump json to char
    const char *jsonDump = json_dumps( this->dbObject->jsonRootObject, JSON_PRESERVE_ORDER | JSON_INDENT(4) );
    etDBObjectValueSet( doDBCore->dbObjectCore, "value", jsonDump );
    free( (void*)jsonDump );

    etDBObjectDump( doDBCore->dbObjectCore );
    //etDBDriverDataChange( this->dbDriver, dbObject );
    etDBDriverDataChange( this->dbDriver, doDBCore->dbObjectCore );


}




bool doDBConnection::               dbRelationLoad(){

// vars
    const char *displayName = this->displayNameGet();

// reset
    this->dbTableRelations = NULL;

// get doDBObject from db
    etDBObjectTablePick( doDBCore->dbObjectCore, "doDB" );
    etDBObjectFilterClear( doDBCore->dbObjectCore );
    etDBObjectFilterAdd( doDBCore->dbObjectCore, 0, etDBFILTER_OP_AND, "key", etDBFILTER_TYPE_EQUAL, "doDBRelations" );
    etDBDriverDataGet( this->dbDriver, doDBCore->dbObjectCore );
    if( etDBDriverDataNext( this->dbDriver, doDBCore->dbObjectCore ) == etID_YES ){

        const char *jsonRelationString = NULL;
        if( etDBObjectValueGet( doDBCore->dbObjectCore, "value", jsonRelationString ) == etID_YES ){
            json_error_t jsonError;
            this->dbTableRelations = json_loads( jsonRelationString, JSON_PRESERVE_ORDER, &jsonError );

            if( jsonError.line > 0 ){
                doDBDebug::ptr->print( jsonError.text );
            } else {
                doDBDebug::ptr->print( QString("%1: relations loaded from db").arg(displayName) );
            }
        } else {
            doDBDebug::ptr->print( QString("%1: no relations in table").arg(displayName) );
            return false;
        }

    } else {
        doDBDebug::ptr->print( QString("%1: no relations in table").arg(displayName) );
        return false;
    }

}


bool doDBConnection::               dbRelationSave(){

// vars
    const char *displayName = this->displayNameGet();

// get doDBObject from db
    etDBObjectTablePick( doDBCore->dbObjectCore, "doDB" );

    etDBObjectValueClean( doDBCore->dbObjectCore );
    etDBObjectValueSet( doDBCore->dbObjectCore, "key", "doDBRelations" );

//dump json to char
    const char *jsonDump = json_dumps( this->dbTableRelations, JSON_PRESERVE_ORDER | JSON_INDENT(4) );
    etDBObjectValueSet( doDBCore->dbObjectCore, "value", jsonDump );
    free( (void*)jsonDump );

    etDBObjectDump( doDBCore->dbObjectCore );
    //etDBDriverDataChange( this->dbDriver, dbObject );
    etDBDriverDataChange( this->dbDriver, doDBCore->dbObjectCore );

}


bool doDBConnection::               relatedTableAppend( const char *srcTable, const char *relatedTable, const char *srcColumn, const char *relatedColumn ){

// check
    if( srcTable == NULL ) return false;
    if( relatedTable == NULL ) return false;
    if( srcColumn == NULL ) return false;
    if( relatedColumn == NULL ) return false;

// vars
    json_t      *jsonRelatedTable = NULL;
    json_t      *jsonValue = NULL;

//
    if( this->dbTableRelations == NULL ){
        this->dbTableRelations = json_object();
    }

// get source table
    this->dbTableRelationSrcTable = json_object_get( this->dbTableRelations, srcTable );
    if( this->dbTableRelationSrcTable == NULL ){
        this->dbTableRelationSrcTable = json_array();
        json_object_set_new( this->dbTableRelations, srcTable, this->dbTableRelationSrcTable );
    }

// create a new relation
    jsonRelatedTable = json_object();
    json_array_append( this->dbTableRelationSrcTable, jsonRelatedTable );


// src-Column
    json_object_set_new( jsonRelatedTable, "srcColumn", json_string(srcColumn) );

// rel-Table
    json_object_set_new( jsonRelatedTable, "relTable", json_string(relatedColumn) );

// rel-Column
    json_object_set_new( jsonRelatedTable, "relColumn", json_string(relatedColumn) );

    return true;
}


bool doDBConnection::               relatedTableGetFirst( const char *srcTable, const char **srcColumn, const char **relatedTable, const char **relatedColumn ){

// check
    if( srcTable == NULL ) return false;


// vars
    json_t      *jsonRelatedTable = NULL;
    json_t      *jsonValue = NULL;

// get first related-table
    this->dbTableRelationSrcTable = json_object_get( this->dbTableRelations, srcTable );
    if( this->dbTableRelationSrcTable == NULL ) return false;

// reset arrayindex
    this->dbTableRelationIndex = 0;

// get the table
    jsonRelatedTable = json_array_get( this->dbTableRelationSrcTable, this->dbTableRelationIndex );
    if( jsonRelatedTable == NULL ) return false;

// src-Column
    jsonValue = json_object_get( jsonRelatedTable, "srcColumn" );
    if( srcColumn != NULL ){
        *srcColumn = json_string_value(jsonValue);
    }

// src-Column
    jsonValue = json_object_get( jsonRelatedTable, "relTable" );
    if( relatedTable != NULL ){
        *relatedTable = json_string_value(jsonValue);
    }

// rel-Column
    jsonValue = json_object_get( jsonRelatedTable, "relColumn" );
    if( relatedColumn != NULL ){
        *relatedColumn = json_string_value(jsonValue);
    }

    return true;
}


bool doDBConnection::               relatedTableGetFirst( const char *srcTable, const char **srcColumn, const char *relatedTable, const char **relatedColumn ){

    const char *relatedTableLocal = NULL;

// get first
    if( relatedTableGetFirst(srcTable,srcColumn,&relatedTableLocal,relatedColumn) ){
        if( QString(relatedTableLocal) == relatedTable ){
            return true;
        }
    }


    while( relatedTableGetNext(srcColumn,&relatedTableLocal,relatedColumn) ){
        if( QString(relatedTableLocal) == relatedTable ){
            return true;
        }
    }

    return false;
}


bool doDBConnection::               relatedTableGetNext( const char **srcColumn, const char **relatedTable, const char **relatedColumn ){


// vars
    json_t      *jsonRelatedTable = NULL;
    json_t      *jsonValue = NULL;

// get first related-table
    if( this->dbTableRelationSrcTable == NULL ) return false;

// reset arrayindex
    this->dbTableRelationIndex = this->dbTableRelationIndex + 1;

// get the table
    jsonRelatedTable = json_array_get( this->dbTableRelationSrcTable, this->dbTableRelationIndex );
    if( jsonRelatedTable == NULL ) return false;

// src-Column
    jsonValue = json_object_get( jsonRelatedTable, "srcColumn" );
    if( srcColumn != NULL ){
        *srcColumn = json_string_value(jsonValue);
    }

// src-Column
    jsonValue = json_object_get( jsonRelatedTable, "relTable" );
    if( relatedTable != NULL ){
        *relatedTable = json_string_value(jsonValue);
    }

// rel-Column
    jsonValue = json_object_get( jsonRelatedTable, "relColumn" );
    if( relatedColumn != NULL ){
        *relatedColumn = json_string_value(jsonValue);
    }

    return true;
}


bool doDBConnection::               relatedTableGetNext( const char **srcColumn, const char *relatedTable, const char **relatedColumn ){

    const char *relatedTableLocal = NULL;

    while( relatedTableGetNext(srcColumn,&relatedTableLocal,relatedColumn) ){
        if( QString(relatedTableLocal) == relatedTable ){
            return true;
        }
    }

    return false;
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


bool doDBConnection::               dbDataGet( const char *srcTable, const char *srcTableItemID, const char *relatedTable, void *userdata, void (*callback)( void *userdata, const char *tableName, const char *connID, const char *primaryValue, const char *displayValue) ){
// check if connected
    if( this->isConnected() == false ){
        return false;
    }

// vars
    const char      *connID = this->UUIDGet();
    const char      *tableDisplayName = NULL;
    const char      *primaryColumn = NULL;
    const char      *primaryColumnValue = NULL;
    const char      *relDisplayColumn = NULL;
    const char      *relDisplayColumnValue = NULL;
    const char      *srcColumn = NULL;
    const char      *srcColumnValue = NULL;
    const char      *relColumn = NULL;


// get the related columns
    bool relationPresent = this->relatedTableGetFirst( srcTable, &srcColumn, relatedTable, &relColumn );
    while( relationPresent ){


    // pick table
        if( etDBObjectTablePick( this->dbObject, srcTable ) != etID_YES ){
            return false;
        }

    // get primary column
       if( etDBObjectTableColumnPrimaryGet( this->dbObject, primaryColumn ) != etID_YES ){
            return false;
        }

    // create filter
        etDBObjectFilterClear( this->dbObject );
        etDBObjectFilterAdd(  this->dbObject, 1, etDBFILTER_OP_AND, primaryColumn, etDBFILTER_TYPE_EQUAL, srcTableItemID );

    // run the query
        if( etDBDriverDataGet( this->dbDriver, this->dbObject ) != etID_YES ) {
            return false;
        }

    // get the first result ( there should be only one )
        if( etDBDriverDataNext( this->dbDriver, this->dbObject ) != etID_YES ){
            return false;
        }

    // okay, get the value of the source column
        etDBObjectValueGet( this->dbObject, srcColumn, srcColumnValue );


    // create filter
        etDBObjectFilterClear( this->dbObject );
        etDBObjectFilterAdd(  this->dbObject, 1, etDBFILTER_OP_AND, relColumn, etDBFILTER_TYPE_EQUAL, srcColumnValue );

    // pick related table
        if( etDBObjectTablePick( this->dbObject, relatedTable ) != etID_YES ){
            return false;
        }

    // get the column which respresent the visible value the value from the related table
        etDBObjectTableColumnMainGet( this->dbObject, relDisplayColumn );

    // run the query
        if( etDBDriverDataGet( this->dbDriver, this->dbObject ) != etID_YES ) {
            goto next;
        }


    // iterate through data
        while( etDBDriverDataNext( this->dbDriver, this->dbObject ) == etID_YES ){


            if( etDBObjectValueGet( this->dbObject, primaryColumn, primaryColumnValue ) != etID_YES ){
                continue;
            }


            if( etDBObjectValueGet( this->dbObject, relDisplayColumn, relDisplayColumnValue ) != etID_YES ){
                doDBDebug::ptr->print( __PRETTY_FUNCTION__ + QString(": table '%1' has no display column use primaryKeyColumn").arg(relatedTable) );
                relDisplayColumnValue = primaryColumnValue;
            }

            callback( userdata, relatedTable, connID, primaryColumnValue, relDisplayColumnValue );

        }

        next:
        relationPresent = this->relatedTableGetNext( &srcColumn, relatedTable, &relColumn );

    }

    return true;
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
    const char *displayName = this->displayNameGet();

// get doDBObject from db
    etDBObjectTablePick( doDBCore->dbObjectCore, "doDB" );
    etDBObjectFilterClear( doDBCore->dbObjectCore );
    etDBObjectFilterAdd( doDBCore->dbObjectCore, 0, etDBFILTER_OP_AND, "key", etDBFILTER_TYPE_EQUAL, key );
    etDBDriverDataGet( this->dbDriver, doDBCore->dbObjectCore );
    if( etDBDriverDataNext( this->dbDriver, doDBCore->dbObjectCore ) == etID_YES ){

        if( __etDBObjectValueGet( doDBCore->dbObjectCore, "value", value ) == etID_YES ){
            return true;
        } else {
            doDBDebug::ptr->print( QString("%1: no %2 in table").arg(displayName).arg(key) );
            return false;
        }

    } else {
        doDBDebug::ptr->print( QString("%1: no %2 in table").arg(displayName).arg(key) );
        return false;
    }

    return false;
}


bool doDBConnection::               dbDoDBValueSet( const char *key, const char *value ){
// check
    if( key == NULL ) return false;
    if( value == NULL ) return false;

// get doDBObject from db
    etDBObjectTablePick( doDBCore->dbObjectCore, "doDB" );

    etDBObjectValueClean( doDBCore->dbObjectCore );
    etDBObjectValueSet( doDBCore->dbObjectCore, "key", key );

//dump json to char
    etDBObjectValueSet( doDBCore->dbObjectCore, "value", value );

// change the value in the db
    etDBDriverDataChange( this->dbDriver, doDBCore->dbObjectCore );
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



