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

#include "doDBConnection/doDBConnections.h"
#include "main.h"

#include "db/etDBObjectTable.h"
#include "db/etDBObjectTableColumn.h"

/**
@defgroup doDBConnections Connections
@short A List of all connections

This class holds all connections. Use it to get an connection by id or remove/add/iterate a connection.
@todo Locking of connection if somebody do something in the db
*/



doDBConnections* doDBConnections::ptr = NULL;

doDBConnections::                           doDBConnections(){

    this->ptr = this;

// nice, now we create the global table
// this table is in every db of an connection present
    etDBObjectAlloc( this->dbObjectCore );
    etDBObjectTableAdd( this->dbObjectCore, "doDB" );
    etDBObjectTableColumnPrimarySet( this->dbObjectCore, "key" );
    etDBObjectTableColumnAdd( this->dbObjectCore, "key", etDBCOLUMN_TYPE_STRING, etDBCOLUMN_OPTION_NOTNULL | etDBCOLUMN_OPTION_PRIMARY | etDBCOLUMN_OPTION_UNIQUE );
    etDBObjectTableColumnAdd( this->dbObjectCore, "table", etDBCOLUMN_TYPE_STRING, etDBCOLUMN_OPTION_NOTHING );
    etDBObjectTableColumnAdd( this->dbObjectCore, "value", etDBCOLUMN_TYPE_STRING, etDBCOLUMN_OPTION_NOTHING );

    this->connections.clear();
    this->connectionsLoad();
}


doDBConnections::                           ~doDBConnections(){
}


void doDBConnections::                      connectionAppend( doDBConnection *newConnection ){
    this->connections.append( newConnection );
}


void doDBConnections::                      connectionRemove( doDBConnection *newConnection ){
    this->connections.removeOne( newConnection );
    this->connections.removeAll( newConnection );
}


doDBConnection* doDBConnections::           connectionGetFirst(){

// vars
    doDBConnection *dbConnection = NULL;


    this->connectionIndex = 0;
    if( this->connections.length() == 0 ){
        return NULL;
    }

    dbConnection = this->connections.at( this->connectionIndex );

    return dbConnection;
}


doDBConnection* doDBConnections::           connectionGetNext(){

// vars
    doDBConnection *dbConnection = NULL;


    this->connectionIndex = this->connectionIndex + 1;
    if( this->connectionIndex >= this->connections.length() ){
        return NULL;
    }

    dbConnection = this->connections.at( this->connectionIndex );

    return dbConnection;
}


doDBConnection* doDBConnections::           connectionGet( const char *id ){

    doDBConnection *dbConnection = NULL;

// iterate
    foreach( dbConnection, this->connections ){
        if( dbConnection->UUIDGet() == QString(id) ){
            return dbConnection;
        }
    }

    return NULL;
}


void doDBConnections::                      connectionsLoad(){

// pick the group
    doDBSettings::ptr->groupPick( "connections" );

// iterate over connections
    void *iterator = json_object_iter( doDBSettings::ptr->jsonGroup );
    while( iterator != NULL ){
        json_t      *jsonConnection = NULL;
        json_t      *jsonValue;
        int         connType = 0;
        const char  *connID;
        const char  *connDisplayName;
        QString     tempValue;

    // get connection id
        connID = json_object_iter_key(iterator);

    // connection json-object
        jsonConnection = json_object_iter_value( iterator ) ;

    // get type
        jsonValue = json_object_get( jsonConnection, "type" );
        if( jsonValue != NULL ){
            connType = json_integer_value(jsonValue);
        }

    // display name
        jsonValue = json_object_get( jsonConnection, "displayName" );
        if( jsonValue != NULL ){
            connDisplayName = json_string_value(jsonValue);
        }


    // create the new connection
        doDBConnection *connection = new doDBConnection( (doDBConnection::connectionType)connType, connID, connDisplayName );

    // filename
        jsonValue = json_object_get( jsonConnection, "filename" );
        if( jsonValue != NULL ){
            tempValue = json_string_value(jsonValue);
            connection->fileNameSet( tempValue );
        }

        jsonValue = json_object_get( jsonConnection, "hostname" );
        if( jsonValue != NULL ){
            tempValue = json_string_value(jsonValue);
            connection->hostname = tempValue;
        }

        jsonValue = json_object_get( jsonConnection, "hostip" );
        if( jsonValue != NULL ){
            tempValue = json_string_value(jsonValue);
            connection->hostip = tempValue;
        }

        jsonValue = json_object_get( jsonConnection, "port" );
        if( jsonValue != NULL ){
            tempValue = json_string_value(jsonValue);
            connection->port = tempValue;
        }

        jsonValue = json_object_get( jsonConnection, "database" );
        if( jsonValue != NULL ){
            tempValue = json_string_value(jsonValue);
            connection->database = tempValue;
        }

        jsonValue = json_object_get( jsonConnection, "username" );
        if( jsonValue != NULL ){
            tempValue = json_string_value(jsonValue);
            connection->username = tempValue;
        }

        jsonValue = json_object_get( jsonConnection, "password" );
        if( jsonValue != NULL ){
            tempValue = json_string_value(jsonValue);
            connection->password = tempValue;
        }






        this->connectionAppend( connection );

        iterator = json_object_iter_next( doDBSettings::ptr->jsonGroup, iterator );
    }



}


void doDBConnections::                      connectionsSave(){

// pick the group
    doDBSettings::ptr->groupPick( "connections" );

// delete all connections
    doDBSettings::ptr->jsonGroup = json_object();
    json_object_set_new( doDBSettings::ptr->jsonRoot, "connections", doDBSettings::ptr->jsonGroup );

// iterate
    doDBConnection *connection;
    foreach( connection, this->connections ){

        json_t *jsonConnection = json_object();
        json_object_set_new( jsonConnection, "type", json_integer(connection->typeGet()) );
        json_object_set_new( jsonConnection, "displayName", json_string(connection->displayNameGet() ) );
        json_object_set_new( jsonConnection, "filename", json_string(connection->fileNameGet().toUtf8() ) );

        json_object_set_new( jsonConnection, "hostname", json_string(connection->hostname.toUtf8() ) );
        json_object_set_new( jsonConnection, "hostip", json_string(connection->hostip.toUtf8() ) );
        json_object_set_new( jsonConnection, "port", json_string(connection->port.toUtf8() ) );
        json_object_set_new( jsonConnection, "database", json_string(connection->database.toUtf8() ) );
        json_object_set_new( jsonConnection, "username", json_string(connection->username.toUtf8() ) );
        json_object_set_new( jsonConnection, "password", json_string(connection->password.toUtf8() ) );

        json_object_set_new( doDBSettings::ptr->jsonGroup, connection->UUIDGet(), jsonConnection );

    }


}




