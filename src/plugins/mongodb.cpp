/*
Copyright (C) 2018 by Martin Langlotz

This file is part of copilot.

copilot is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, version 3 of this License

copilot is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with copilot.  If not, see <http://www.gnu.org/licenses/>.

*/


#ifndef mongodb_C
#define mongodb_C

#include <iostream>
#include <string>

#include "coCore.h"
#include "pubsub.h"

#include "core/etDebug.h"

#include "plugins/mongodb.h"


mdbClient::             mdbClient( std::string hostname, int port, const char* dbname, std::string* errorMessage ){
    
    
// init vars
    this->connected = false;
    this->client = NULL;
    this->database = NULL;
    this->collTemplate = NULL;
    
// build uri
    std::string uri = "mongodb://";
    uri += hostname;
    uri += ":";
    uri += std::to_string(port);
    
    
// init client
    this->client = mongoc_client_new( uri.c_str() );
    if( this->client == NULL ){
        if( errorMessage != NULL ) errorMessage->assign( "Could not connect to server." );
        return;
    }

// setup app-name
    mongoc_client_set_appname( this->client, "copilotd" );

// list dbs to check connection
    bson_error_t error;
    char **strv;
    strv = mongoc_client_get_database_names_with_opts( client, NULL, &error );
    if( strv == NULL ){
        if( errorMessage != NULL ) errorMessage->assign( error.message );
        return;
    }
    bson_strfreev( strv );


// get database
    this->database = database = mongoc_client_get_database( this->client, dbname );
    
// get collection
    this->collTemplate = mongoc_client_get_collection( this->client, dbname, "data" );
    if( this->collTemplate == NULL ){
        if( errorMessage != NULL ) errorMessage->assign( "Could not get collection 'data'" );
        return;
    }
    
// everything okay, return true
    if( errorMessage != NULL ) errorMessage->clear();
    this->connected = true;
    return;
}


mdbClient::             ~mdbClient(){
    
    if( this->database != NULL ){
        mongoc_database_destroy( this->database );
    }
    if( this->collTemplate != NULL ){
        mongoc_collection_destroy( this->collTemplate );
    }
    if( this->client != NULL ){
        mongoc_client_destroy ( this->client );
    }

}


bool mdbClient::        isConnected(){
    return this->connected;
}


bool mdbClient::        jsonAdd( const char* jsonString, std::string* errorMessage ){
    
// vars
    bson_error_t    error;
    bson_t*         bson;

// test
//   const char*  json = "{\"name\": {\"first\":\"Grace\", \"last\":\"Hopper\"}}";
    bson = bson_new_from_json ((const uint8_t *)jsonString, -1, &error);
    if (!bson) {
        if( errorMessage != NULL ) errorMessage->assign( error.message );
        return false;
    }


    if ( mongoc_collection_insert_one( this->collTemplate, bson, NULL, NULL, &error ) == false ) {
        if( errorMessage != NULL ) errorMessage->assign( error.message );
        return false;
    }
    
    errorMessage->clear();
    return true;
}


bool mdbClient::        queryToJson( bson_t* filter, bson_t* opts, json_t* jsonArray ){
    
// vars
    mongoc_cursor_t*        cursor;
    bson_error_t            error;
    const bson_t*           doc;
    char*                   str;
    json_t*                 jsonDocument;
    json_error_t            jsonError;

/* filter by "foo": 1, order by "bar" descending */
    //filter = BCON_NEW ( "_type", "template" );
    //opts = BCON_NEW ( "limit", BCON_INT64 (10), "sort", "{", "bar", BCON_INT32 (-1), "}" );

    cursor = mongoc_collection_find_with_opts( this->collTemplate, filter, opts, NULL );

    while( mongoc_cursor_next( cursor, &doc ) == true ){
        str = bson_as_canonical_extended_json( doc, NULL );
        
    // create a json from string
        jsonDocument = json_loads( str, strlen(str), &jsonError );
        if( jsonDocument == NULL || jsonError.line >= 0 ){
            etDebugMessage( etID_LEVEL_ERR, jsonError.text );
            bson_free (str);
            mongoc_cursor_destroy (cursor);
            return false;
        }
        
    // add it to global
        json_array_append_new( jsonArray, jsonDocument );
        
    // cleanup
        bson_free (str);
    }

    if (mongoc_cursor_error (cursor, &error)) {
        fprintf (stderr, "An error occurred: %s\n", error.message);
    }

    mongoc_cursor_destroy (cursor);
    
    
}






mongodb::               mongodb(){

// vars
    json_t*     jsonValue = NULL;

// init
    mongoc_init();
    
// get config
    this->config( NULL, NULL, NULL );

//
    this->client = NULL;
    
// subscribe
	psBus::inst->subscribe( coCore::ptr->nodeName(), "mdb", this, mongodb::onSubscriberMessage, NULL );
}

mongodb::               ~mongodb(){
    mongoc_cleanup ();
}


void mongodb::          config( const char** hostName, int* port, const char** db ){

// vars
    json_t*     jsonValue = NULL;
    
// load config
    this->jsonConfig = coCore::ptr->config->section( "mdb" );
    
// host
    jsonValue = json_object_get( this->jsonConfig, "host" );
    if( jsonValue == NULL ) json_object_set( this->jsonConfig, "host", json_string("localhost") );
    if( hostName != NULL ) *hostName = json_string_value( jsonValue );

// port
    jsonValue = json_object_get( this->jsonConfig, "port" );
    if( jsonValue == NULL ) json_object_set( this->jsonConfig, "port", json_integer(27017) );
    if( port != NULL ) *port = json_integer_value( jsonValue );

// db
    jsonValue = json_object_get( this->jsonConfig, "db" );
    if( jsonValue == NULL ) json_object_set( this->jsonConfig, "db", json_string("copilotd") );
    if( db != NULL ) *db = json_string_value( jsonValue );

}






int mongodb::           onSubscriberMessage( 
                            const char* id, 
                            const char* nodeSource, 
                            const char* nodeTarget, 
                            const char* group, 
                            const char* command, 
                            const char* payload, 
                            void* userdata 
                        ){
// vars
    mongodb*		    mongodbPtr = (mongodb*)userdata;
    const char*			myNodeName = coCore::ptr->nodeName();
    std::string         errorMessage = "";
    

// command len
    if( command == NULL ) return psBus::END;
    int msgCommandLen = strlen(command);


    if( coCore::strIsExact("configGet",command,msgCommandLen) == true ){

    // dump
        const char* jsonTempString = json_dumps( mongodbPtr->jsonConfig, JSON_PRESERVE_ORDER | JSON_COMPACT );

    // add the message to list
        psBus::inst->publish( id, myNodeName, nodeSource, group, "config", jsonTempString );

    // cleanup
        free( (void*)jsonTempString );

        return psBus::END;
    }
    
    
    if( coCore::strIsExact("connect",command,msgCommandLen) == true ){
        
        if( mongodbPtr->client != NULL ){
            goto connstate;
        }
        
        const char* hostName; int port; const char* db;
        mongodbPtr->config( &hostName, &port, &db );
        
        mongodbPtr->client = new mdbClient( hostName, port, db, &errorMessage );
        if( errorMessage.length() > 0 ){
            etDebugMessage( etID_LEVEL_ERR, errorMessage.c_str() );
            delete mongodbPtr->client;
            mongodbPtr->client = NULL;
        }
        
    // print connection state
        goto connstate;
    }


    if( coCore::strIsExact("isConnected",command,msgCommandLen) == true ){
    connstate:
    // connected ?
        if( mongodbPtr->client == NULL ){
            psBus::inst->publish( id, myNodeName, nodeSource, group, "state", "disconnected" );
            return psBus::END;
        }
    
        if( mongodbPtr->client->isConnected() == true ){
            psBus::inst->publish( id, myNodeName, nodeSource, group, "state", "connected" );
        } else {
            psBus::inst->publish( id, myNodeName, nodeSource, group, "state", "disconnected" );
            if( errorMessage.length() > 0 ){
                psBus::inst->publish( id, myNodeName, nodeSource, group, "error", errorMessage.c_str() );
            }
        }

        return psBus::END;
    }


    if( coCore::strIsExact("templatesGet",command,msgCommandLen) == true ){

        if( mongodbPtr->client == NULL ){
            return psBus::END;
        }
        
        json_t*     jsonArray = json_array();
        
        bson_t*     bsonFilter;
        bson_t*     bsonOpts;


        bsonFilter = BCON_NEW( 
            "_type", "template"
        );

        bsonOpts = BCON_NEW( 
            "projection",
                "{",
                    "id", BCON_INT32(1),
                    "displayName", BCON_INT32(1),
                "}"
        );
        

    // query
        mongodbPtr->client->queryToJson( bsonFilter, bsonOpts, jsonArray );
        bson_destroy( bsonFilter );
        bson_destroy( bsonOpts );
        
    // dump
        const char* jsonArrayString = json_dumps( jsonArray, JSON_PRESERVE_ORDER | JSON_COMPACT );

    // answer
        psBus::inst->publish( id, myNodeName, nodeSource, "mdb", "templates", jsonArrayString );
        

    // cleanup
        free( (void*)jsonArrayString );
        json_decref( jsonArray );


        return psBus::END;
    }


    if( coCore::strIsExact("templateSave",command,msgCommandLen) == true ){
        
        if( mongodbPtr->client == NULL ){
            return psBus::END;
        }

        if( mongodbPtr->client->jsonAdd( payload, &errorMessage ) == false ){
            psBus::inst->publish( id, myNodeName, nodeSource, group, "templateNotSaved", "" );
            if( errorMessage.length() > 0 ){
                psBus::inst->publish( id, myNodeName, nodeSource, group, "error", errorMessage.c_str() );
            }
        } else {
            psBus::inst->publish( id, myNodeName, nodeSource, group, "templateSaved", "" );
        }
        
        
        return psBus::END;
    }


// we dont do anything with it, next please
    return psBus::NEXT_SUBSCRIBER;
}


#endif