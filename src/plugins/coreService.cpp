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

coreService::                   		coreService() : coPlugin( "core", "", "co" ) {


// register plugin
	coCore::ptr->plugins->append( this );

}


coreService::                   		~coreService(){

}



#ifndef MQTT_ONLY_LOCAL
void coreService:: 						appendKnownNodes( const char* hostName ){

	json_t*			jsonObject = NULL;
	json_error_t	jsonError;
	json_t*			jsonNode = NULL;
	json_t*			jsonNodes = NULL;

// open the file
    jsonObject = json_load_file( baseFilePath "known_nodes.json", JSON_PRESERVE_ORDER, &jsonError );
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
		json_dump_file( jsonObject, baseFilePath "known_nodes.json", JSON_PRESERVE_ORDER | JSON_INDENT(4) );
	}

// cleanup
	json_decref( jsonObject );
}
#endif



coPlugin::t_state coreService::			onBroadcastMessage( coMessage* message ){

// vars
	const char*			msgHostName = message->hostName();
	const char*			msgGroup = message->group();
	const char*			msgCommand = message->command();
	const char*			msgPayload = message->payload();

// to all hosts
    if( strncmp( (char*)msgHostName, "all", 3 ) == 0 ){

    // ping
        if( strncmp(msgCommand,"ping",4) == 0 ){

			const char* tempHostName = NULL;
			coCore::ptr->hostNameGet( &tempHostName, NULL );

			message->hostName( tempHostName );
			message->replyCommand( "pong" );

            return coPlugin::REPLY;
        }
    }


// to "localhost" or to the nodename-host
    if( strncmp(msgHostName,"localhost",9) != 0 &&
	coCore::ptr->isHostName(msgHostName) == false ){
        return coPlugin::NO_REPLY;
    }

#ifndef MQTT_ONLY_LOCAL
// append hostname to known hosts
	this->appendKnownNodes( msgHostName );
#endif

// get version
    if( strncmp( (char*)msgCommand, "copilotdVersionGet", 10 ) == 0 ){

		message->replyCommand( "copilotdVersion" );
		message->replyPayload( copilotVersion );

        return coPlugin::REPLY;
    }

// get services
    if( strncmp( (char*)msgCommand, "getServices", 11 ) == 0 ){
        return coPlugin::NO_REPLY;
    }

// get hosts
    if( strncmp( (char*)msgCommand, "knownHostsGet", 10 ) == 0 ){

	// vars
		//json_t*			jsonArray = json_array();
		json_t*			jsonObject = NULL;
		char*			jsonArrayChar = NULL;

		coCore::ptr->config->nodesGet( &jsonObject );
		jsonArrayChar = json_dumps( jsonObject, JSON_PRESERVE_ORDER | JSON_COMPACT );

		message->replyCommand( "knownHosts" );
		message->replyPayload( jsonArrayChar );

	// free
		free(jsonArrayChar);
		//json_decref(jsonArray);

        return coPlugin::REPLY;
    }



    return coPlugin::NO_REPLY;
}






#endif // ldapService_C
