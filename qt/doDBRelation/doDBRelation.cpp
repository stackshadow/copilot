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

#include <QDebug>
#include <QUuid>

#include "main.h"
#include "doDBRelation.h"
#include "doDBDebug/doDBDebug.h"
#include "doDBConnection/doDBConnections.h"
#include "doDBConnection/doDBConnection.h"

#include "core/etDebug.h"
#include "db/etDBObjectValue.h"
#include "db/etDBObjectFilter.h"

doDBRelation::                  doDBRelation() {

    this->loadedConnection = NULL;

// init member vars
    this->jsonRelation = NULL;
    this->jsonRelationSrcTable = NULL;
    this->jsonRelationIndex = 0;

}

doDBRelation::                  ~doDBRelation(){

}





bool doDBRelation::             relationImport( const char *jsonString ){



    json_error_t                jsonError;


// reset
    if( this->jsonRelation != NULL ){
        json_delete( this->jsonRelation );
        this->jsonRelation = NULL;
    }

// parse json-string
    this->jsonRelation = json_loads( jsonString, JSON_PRESERVE_ORDER, &jsonError );

    if( this->jsonRelation == NULL ){
        return false;
    }

    return true;
}


bool doDBRelation::             relationExport( const char **jsonString ){

    *jsonString = json_dumps( this->jsonRelation, JSON_PRESERVE_ORDER | JSON_INDENT(4) );

}


bool doDBRelation::             relationAppend( const char *srcTable, const char *relatedTable, const char *srcColumn, const char *relatedColumn ){

// check
    if( srcTable == NULL ) return false;
    if( relatedTable == NULL ) return false;
    if( srcColumn == NULL ) return false;
    if( relatedColumn == NULL ) return false;

// vars
    json_t      *jsonSrcTable = NULL;
    json_t      *jsonRelatedTable = NULL;

//
    if( this->jsonRelation == NULL ){
        this->jsonRelation = json_object();
    }

// get source table
    jsonSrcTable = json_object_get( this->jsonRelation, srcTable );
    if( jsonSrcTable == NULL ){
        jsonSrcTable = json_array();
        json_object_set_new( this->jsonRelation, srcTable, jsonSrcTable );
    }

// create a new relation
    jsonRelatedTable = json_object();
    json_array_append( jsonSrcTable, jsonRelatedTable );


// src-Column
    json_object_set_new( jsonRelatedTable, "srcColumn", json_string(srcColumn) );

// rel-Table
    json_object_set_new( jsonRelatedTable, "relTable", json_string(relatedTable) );

// rel-Column
    json_object_set_new( jsonRelatedTable, "relColumn", json_string(relatedColumn) );

    return true;
}


bool doDBRelation::             relationRemove( const char *srcTable, const char *relatedTable, const char *srcColumn, const char *relatedColumn ){

    const char *localsrcTable;
    const char *localsrcColumn = srcColumn;
    const char *localrelTable = relatedTable;
    const char *localrelColumn = relatedColumn;

// get the first table
    this->relationGetReset();
    if( this->relatedTableFindNext( srcTable, &localsrcColumn, &localrelTable, &localrelColumn ) ){

        json_array_remove( this->jsonRelationSrcTable, this->jsonRelationIndex );

    }

}


void doDBRelation::             relationGetReset(){
    this->jsonSrcTableItrerator = NULL;
}


bool doDBRelation::             relationGetNext( const char **srcTable, const char **srcColumn, const char **relatedTable, const char **relatedColumn ){

// vars
    json_t      *jsonRelatedTable = NULL;
    json_t      *jsonValue = NULL;


// start iteration ?
    if( this->jsonSrcTableItrerator == NULL ){
        this->jsonSrcTableItrerator = json_object_iter( this->jsonRelation );
        if( this->jsonSrcTableItrerator == NULL ) return false;

        this->jsonRelationSrcTable = json_object_iter_value( this->jsonSrcTableItrerator );
        this->jsonRelationIndex = -1;
    }

// next in the array
    this->jsonRelationIndex++;

// get the next related table ( if possible )
    jsonRelatedTable = json_array_get( this->jsonRelationSrcTable, this->jsonRelationIndex );

// NULL if no more relation is inside -> next table
    if( jsonRelatedTable == NULL ){

    // next source table
        this->jsonSrcTableItrerator = json_object_iter_next( this->jsonRelation, this->jsonSrcTableItrerator );
        while( this->jsonSrcTableItrerator != NULL ){

            this->jsonRelationSrcTable = json_object_iter_value( this->jsonSrcTableItrerator );
            this->jsonRelationIndex = 0;

            jsonRelatedTable = json_array_get( this->jsonRelationSrcTable, this->jsonRelationIndex );
            if( jsonRelatedTable != NULL ){
                break;
            }

        // next
            this->jsonSrcTableItrerator = json_object_iter_next( this->jsonRelation, this->jsonSrcTableItrerator );
        }

    // table iterator is null, exit
        if( this->jsonSrcTableItrerator == NULL ) return false;

    }



// get first related-table
    this->jsonRelationSrcTable = json_object_iter_value( this->jsonSrcTableItrerator );
    if( this->jsonRelationSrcTable == NULL ) return false;



// src-Table
    if( srcTable != NULL ){
        *srcTable = json_object_iter_key(this->jsonSrcTableItrerator);
    }

// src-Column
    jsonValue = json_object_get( jsonRelatedTable, "srcColumn" );
    if( srcColumn != NULL ){
        *srcColumn = json_string_value(jsonValue);
    }

// rel-Table
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



bool doDBRelation::             relatedTableGetNext( const char *srcTable, const char **srcColumn, const char **relatedTable, const char **relatedColumn ){


// vars
    json_t      *jsonRelatedTable = NULL;
    json_t      *jsonValue = NULL;

// start iteration ?
    if( this->jsonSrcTableItrerator == NULL ){
        this->jsonSrcTableItrerator = json_object_iter( this->jsonRelation );
        if( this->jsonSrcTableItrerator == NULL ) return false;

        this->jsonRelationSrcTable = json_object_get( this->jsonRelation, srcTable );
        this->jsonRelationIndex = -1;
    }


// no table present
    if( this->jsonRelationSrcTable == NULL ) return false;

// reset arrayindex
    this->jsonRelationIndex = this->jsonRelationIndex + 1;

// get the table
    jsonRelatedTable = json_array_get( this->jsonRelationSrcTable, this->jsonRelationIndex );
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


bool doDBRelation::             relatedTableFindNext( const char *srcTable, const char **srcColumn, const char *relatedTable, const char **relatedColumn ){

    const char *relatedTableLocal = NULL;

    while( relatedTableGetNext(srcTable,srcColumn,&relatedTableLocal,relatedColumn) ){
        if( QString(relatedTableLocal) == relatedTable ){
            return true;
        }
    }

    return false;
}




bool doDBRelation::             relatedTableFindNext( const char *srcTable, const char **srcColumn, const char **relatedTable, const char **relatedColumn ){
// nothing should be NULL
    if( srcTable == NULL ){
        return false;
    }

// vars
    json_t*         jsonRelatedTable = NULL;
    json_t*         jsonValue = NULL;
    const char*     tempValue = NULL;
    const char*     tempParameter = NULL;

// start iteration ?
    if( this->jsonSrcTableItrerator == NULL ){
        this->jsonSrcTableItrerator = json_object_iter( this->jsonRelation );
        if( this->jsonSrcTableItrerator == NULL ) return false;

        this->jsonRelationSrcTable = json_object_get( this->jsonRelation, srcTable );
        this->jsonRelationIndex = -1;
    }


// no table present
    if( this->jsonRelationSrcTable == NULL ) return false;


// next index
    this->jsonRelationIndex = this->jsonRelationIndex + 1;

// get the table
    jsonRelatedTable = json_array_get( this->jsonRelationSrcTable, this->jsonRelationIndex );
    while( jsonRelatedTable != NULL ){

    // src-Column
        jsonValue = json_object_get( jsonRelatedTable, "srcColumn" );
        tempValue = json_string_value(jsonValue);
        tempParameter = *srcColumn;
        if( srcColumn != NULL ){
            if( *srcColumn != NULL ){
                if( strncmp(tempValue,*srcColumn,strlen(tempValue)) != 0 ){
                    goto nextRelatedTable;
                }
            } else {
                *srcColumn = tempValue;
            }
        }

    // src-Column
        jsonValue = json_object_get( jsonRelatedTable, "relTable" );
        tempValue = json_string_value(jsonValue);
        tempParameter = *relatedTable;
        if( relatedTable != NULL ){
            if( *relatedTable != NULL ){
                if( strncmp(tempValue,*relatedTable,strlen(tempValue)) != 0 ){
                    goto nextRelatedTable;
                }
            } else {
                *relatedTable = tempValue;
            }
        }

    // rel-Column
        jsonValue = json_object_get( jsonRelatedTable, "relColumn" );
        tempValue = json_string_value(jsonValue);
        tempParameter = *relatedColumn;
        if( relatedColumn != NULL ){
            if( *relatedColumn != NULL ){
                if( strncmp(tempValue,*relatedColumn,strlen(tempValue)) != 0 ){
                    goto nextRelatedTable;
                }
            } else {
                *relatedColumn = tempValue;
            }
        }

    // when we are here, we found our item
        return true;

    nextRelatedTable:
        this->jsonRelationIndex = this->jsonRelationIndex + 1;
        jsonRelatedTable = json_array_get( this->jsonRelationSrcTable, this->jsonRelationIndex );
    }

    return false;
}




bool doDBRelation::             dbRelationLoad( doDBConnection* connection ){
    if( connection == NULL ) return false;

// vars
    const char                  *displayName = NULL;
    const char                  *dbValueString = NULL;


// already loaded from this connection
    if( this->loadedConnection == connection ) return true;

// get display name ( for debugging purpose )
    displayName = connection->displayNameGet();


    if( connection->dbDoDBValueGet( "doDBRelations", &dbValueString ) ){
        if( this->relationImport( dbValueString ) == true ){
            this->loadedConnection = connection;
        }
    }



}


bool doDBRelation::             dbRelationSave( doDBConnection* connection ){
    if( connection == NULL ) return false;

// vars
    const char                  *displayName = NULL;
    const char                  *jsonDump = NULL;
    bool                        returnValue = false;

    this->relationExport( &jsonDump );
    if( jsonDump != NULL ){
        returnValue = connection->dbDoDBValueSet( "doDBRelations", jsonDump );
        free( (void*)jsonDump );
    }

    return returnValue;
}



bool doDBRelation::             dbDataGet( const char *connectionID, const char *srcTable, const char *srcTableItemID, const char *relatedTable, void *userdata, void (*callback)( void *userdata, const char *tableName, const char *connID, const char *primaryValue, const char *displayValue) ){

// vars
    doDBConnection  *connection = NULL;
    etDBObject      *dbObject = NULL;
    etDBDriver      *dbDriver = NULL;
    QString         dbObjectLockID;
    const char      *connID = NULL;
    const char      *tableDisplayName = NULL;
    const char      *relPrimaryColumn = NULL;
    const char      *relPrimaryColumnValue = NULL;
    const char      *relDisplayColumn = NULL;
    const char      *relDisplayColumnValue = NULL;
    const char      *srcColumn = NULL;
    const char      *srcColumnValue = NULL;
    const char      *relColumn = NULL;


// get connection
    connection = doDBConnections::ptr->connectionGet( connectionID );
    if( connection == NULL ) return false;

// get the object
    dbObject = connection->dbObject;
    if( dbObject == NULL ) return false;

// get the driver
    dbDriver = connection->dbDriver;
    if( dbDriver == NULL ) return false;


// get the related columns
    this->relationGetReset();
    while( this->relatedTableFindNext( srcTable, &srcColumn, relatedTable, &relColumn ) ){


    // get all columns from selected row
        if( connection->dbDataRead( srcTable, srcTableItemID )  != true ) break;
        if( connection->dbDataNext() != true ) break;


    // okay, get the value of the source column
        etDBObjectValueGet( dbObject, srcColumn, srcColumnValue );



    // pick related table
        if( etDBObjectTablePick( dbObject, relatedTable ) != etID_YES ){
            return false;
        }

    // create filter
        etDBObjectFilterClear( dbObject );
        etDBObjectFilterAdd(  dbObject, 1, etDBFILTER_OP_AND, relColumn, etDBFILTER_TYPE_EQUAL, srcColumnValue );

    // get the column which respresent the visible value the value from the related table
        etDBObjectTableColumnMainGet( dbObject, relDisplayColumn );
    // primary column of related table
        etDBObjectTableColumnPrimaryGet( dbObject, relPrimaryColumn );

    // run the query
        if( etDBDriverDataGet( dbDriver, dbObject ) != etID_YES ) {
            continue;
        }


    // iterate through data
        while( etDBDriverDataNext( dbDriver, dbObject ) == etID_YES ){


            if( etDBObjectValueGet( dbObject, relPrimaryColumn, relPrimaryColumnValue ) != etID_YES ){
                continue;
            }


            if( etDBObjectValueGet( dbObject, relDisplayColumn, relDisplayColumnValue ) != etID_YES ){
                doDBDebug::ptr->print( __PRETTY_FUNCTION__ + QString(": table '%1' has no display column use primaryKeyColumn").arg(relatedTable) );
                relDisplayColumnValue = relPrimaryColumnValue;
            }

            callback( userdata, relatedTable, connectionID, relPrimaryColumnValue, relDisplayColumnValue );

        }


    }

    return true;
}


bool doDBRelation::             dbDataRead( const char *connectionID, const char *srcTable, const char *srcTableItemID, const char *relatedTable ){

// vars
    doDBConnection  *connection = NULL;
    etDBObject      *dbObject = NULL;
    etDBDriver      *dbDriver = NULL;
    QString         dbObjectLockID;
    const char      *connID = NULL;
    const char      *tableDisplayName = NULL;
    const char      *relPrimaryColumn = NULL;
    const char      *relPrimaryColumnValue = NULL;
    const char      *relDisplayColumn = NULL;
    const char      *relDisplayColumnValue = NULL;
    const char      *srcColumn = NULL;
    const char      *srcColumnValue = NULL;
    const char      *relColumn = NULL;


// get connection
    connection = doDBConnections::ptr->connectionGet( connectionID );
    if( connection == NULL ) return false;

// get the object
    dbObject = connection->dbObject;
    if( dbObject == NULL ) return false;

// get the driver
    dbDriver = connection->dbDriver;
    if( dbDriver == NULL ) return false;


// get the related columns
    this->relationGetReset();
    while( this->relatedTableFindNext( srcTable, &srcColumn, relatedTable, &relColumn ) ){


    // get all columns from selected row
        if( connection->dbDataRead( srcTable, srcTableItemID )  != true ) break;
        if( connection->dbDataNext() != true ) break;

    // okay, get the value of the source column
        etDBObjectValueGet( dbObject, srcColumn, srcColumnValue );



    // pick related table
        if( etDBObjectTablePick( dbObject, relatedTable ) != etID_YES ){
            return false;
        }

    // create filter
        etDBObjectFilterClear( dbObject );
        etDBObjectFilterAdd(  dbObject, 1, etDBFILTER_OP_AND, relColumn, etDBFILTER_TYPE_EQUAL, srcColumnValue );

    // get the column which respresent the visible value the value from the related table
        etDBObjectTableColumnMainGet( dbObject, relDisplayColumn );
    // primary column of related table
        etDBObjectTableColumnPrimaryGet( dbObject, relPrimaryColumn );

    // run the query
        if( etDBDriverDataGet( dbDriver, dbObject ) != etID_YES ) {
            continue;
        }

    }

    return true;
}


