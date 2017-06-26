/*
Copyright (C) 2017 by Martin Langlotz

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

#ifndef authService_C
#define authService_C

#include "coCore.h"
#include "plugins/coreService.h"
#include "version.h"

#include <string>

coreService::                   coreService() : coPlugin( "core" ) {

// alloc


// register this plugin
    coCore::ptr->registerPlugin( this, "", "co" );

}


coreService::                   ~coreService(){

}



void coreService:: 				appendKnownNodes( const char* hostName ){

	json_t*			jsonObject = NULL;
	json_error_t	jsonError;
	json_t*			jsonNode = NULL;
	json_t*			jsonNodes = NULL;

// open the file
    jsonObject = json_load_file( configFile("known_nodes.json"), JSON_PRESERVE_ORDER, &jsonError );
    if( jsonObject == NULL ){
        jsonObject = json_object();
        json_object_set_new( jsonObject, "nodes", json_object() );
    }
	jsonNodes = json_object_get( jsonObject, "nodes" );

// check if node exist
	jsonNode = json_object_get( jsonNodes, hostName );
	if( jsonNode == NULL ){
		jsonNode = json_object();
		json_object_set_new( jsonNodes, hostName, jsonNode );
		json_dump_file( jsonObject, configFile("known_nodes.json"), JSON_PRESERVE_ORDER | JSON_INDENT(4) );
	}

// cleanup
	json_decref( jsonObject );
}




bool coreService::              onBroadcastMessage( coMessage* message ){

// vars
	const char*			msgHostName = message->hostName();
	const char*			msgGroup = message->group();
	const char*			msgCommand = message->command();
	const char*			msgPayload = message->payload();

// to all hosts
    if( strncmp( (char*)msgHostName, "all", 3 ) == 0 ){

    // ping
        if( strncmp(msgCommand,"ping",4) == 0 ){

			message->hostName( coCore::ptr->hostInfo.nodename );
			message->replyCommand( "pong" );

            return true;
        }
    }


// to "localhost" or to the nodename-host
    if( strncmp(msgHostName,"localhost",9) != 0 &&
	strncmp(msgHostName,coCore::ptr->hostInfo.nodename,coCore::ptr->hostNodeNameLen) != 0 ){
        return true;
    }

// append hostname to known hosts
	this->appendKnownNodes( msgHostName );

// get version
    if( strncmp( (char*)msgCommand, "getVersion", 10 ) == 0 ){

		message->replyCommand( "version" );
		message->replyPayload( copilotVersion );

        return true;
    }

// get services
    if( strncmp( (char*)msgCommand, "getServices", 11 ) == 0 ){
        return true;
    }

// get hosts




    return true;
}






#endif // ldapService_C
