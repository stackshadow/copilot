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
    const char *localrelTable;
    const char *localsrcColumn;
    const char *localrelColumn;

// get the first table
    this->relationGetReset();
    while( this->relationGetNext( &localsrcTable, &localsrcColumn, &localrelTable, &localrelColumn ) ){

        std::string tempString;

        tempString = localsrcTable;
        if( tempString == srcTable ){

            tempString = localsrcColumn;
            if( tempString == srcColumn ){

                tempString = localrelTable;
                if( tempString == relatedTable ){

                    tempString = localrelColumn;
                    if( tempString == relatedColumn ){

                        json_array_remove( this->jsonRelationSrcTable, this->jsonRelationIndex );

                    }

                }

            }

        }

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




bool doDBRelation::             relatedTableFindNext( const char **srcTable, const char **srcColumn, const char **relatedTable, const char **relatedColumn ){
// nothing should be NULL
    if( srcTable == NULL || srcColumn == NULL || relatedTable == NULL || relatedColumn == NULL ){
        return false;
    }

findAgain:

// vars
    json_t*         jsonRelatedTable = NULL;
    json_t*         jsonValue = NULL;
    const char*     jsonValueChar = NULL;
    std::string     jsonValueString;

// start ?
    if( this->jsonSrcTableItrerator ){
        this->jsonSrcTableItrerator = json_object_iter( this->jsonRelation );
        this->jsonRelationIndex = -1;
    }
//    this->jsonSrcTableItrerator = json_object_iter_next( this->jsonRelation, this->jsonSrcTableItrerator );


// reset arrayindex
    this->jsonRelationIndex = this->jsonRelationIndex + 1;

// get the related table
    jsonRelatedTable = json_array_get( this->jsonRelationSrcTable, this->jsonRelationIndex );

// no related table, get next source table
    if( jsonRelatedTable == NULL ){

        this->jsonSrcTableItrerator = json_object_iter_next( this->jsonRelation, this->jsonSrcTableItrerator );
        if( this->jsonSrcTableItrerator == NULL ) return false;

        this->jsonRelationIndex = 0;
        jsonRelatedTable = json_array_get( this->jsonRelationSrcTable, this->jsonRelationIndex );
        if( jsonRelatedTable == NULL ) return false;
    }


// src-Table
    jsonValueChar = json_object_iter_key(this->jsonSrcTableItrerator);
    if( *srcTable != NULL ){
        jsonValueString = jsonValueChar;
        if( jsonValueString != *srcTable ){
            goto findAgain;
        }
    }

// src-Column
    jsonValue = json_object_get( jsonRelatedTable, "srcColumn" );
    jsonValueChar = json_string_value(jsonValue);
    if( *srcColumn != NULL ){
        jsonValueString = jsonValueChar;
        if( jsonValueString != *srcColumn ){
            goto findAgain;
        }
    }

// src-Column
    jsonValue = json_object_get( jsonRelatedTable, "relTable" );
    jsonValueChar = json_string_value(jsonValue);
    if( *relatedTable != NULL ){
        jsonValueString = jsonValueChar;
        if( jsonValueString != *relatedTable ){
            goto findAgain;
        }
    }

// rel-Column
    jsonValue = json_object_get( jsonRelatedTable, "relColumn" );
    jsonValueChar = json_string_value(jsonValue);
    if( *relatedColumn != NULL ){
        jsonValueString = jsonValueChar;
        if( jsonValueString != *relatedColumn ){
            goto findAgain;
        }
    }


}




bool doDBRelation::             dbRelationLoad( const char *connectionID ){
// check
    if( connectionID == NULL ) return false;


// vars
    doDBConnection              *connection = NULL;
    const char                  *displayName = NULL;
    const char                  *dbValueString = NULL;


// get connection
    connection = doDBConnections::ptr->connectionGet( connectionID );
    if( connection == NULL ) return false;

// get display name ( for debugging purpose )
    displayName = connection->displayNameGet();



    if( connection->dbDoDBValueGet( "doDBRelations", &dbValueString ) ){
        this->relationImport( dbValueString );
    }



}


bool doDBRelation::             dbRelationSave( const char *connectionID ){
// check
    if( connectionID == NULL ) return false;


// vars
    doDBConnection              *connection = NULL;
    const char                  *displayName = NULL;
    const char                  *jsonDump = NULL;

// get connection
    connection = doDBConnections::ptr->connectionGet( connectionID );
    if( connection == NULL ) return false;

    this->relationExport( &jsonDump );
    if( jsonDump != NULL ){
        connection->dbDoDBValueSet( "doDBRelations", jsonDump );
        free( (void*)jsonDump );
    }




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
        if( connection->dbDataGet( srcTable, srcTableItemID )  != true ) break;


    // okay, get the value of the source column
        etDBObjectValueGet( dbObject, srcColumn, srcColumnValue );


    // create filter
        etDBObjectFilterClear( dbObject );
        etDBObjectFilterAdd(  dbObject, 1, etDBFILTER_OP_AND, relColumn, etDBFILTER_TYPE_EQUAL, srcColumnValue );

    // pick related table
        if( etDBObjectTablePick( dbObject, relatedTable ) != etID_YES ){
            return false;
        }

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




