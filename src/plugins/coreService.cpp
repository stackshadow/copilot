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
    if( hostName == NULL ) return;

    coCore::ptr->config->nodesIterate();
    if( coCore::ptr->config->nodeSelectByHostName( hostName ) == false ){

    // append the host
        coCore::ptr->config->nodeSelect( hostName );
        coCoreConfig::nodeType hostType = coCoreConfig::CLIENT_IN;
        coCore::ptr->config->nodeInfo( NULL, &hostType, true );
        coCore::ptr->config->nodeConnInfo( &hostName, NULL, true );
        coCore::ptr->config->nodesIterateFinish();

        coCore::ptr->config->save();
        return;
    }
    coCore::ptr->config->nodesIterateFinish();
}
#endif


coPlugin::t_state coreService::			onBroadcastMessage( coMessage* message ){

// vars
    const char*         myHostName = coCore::ptr->hostNameGet();
	const char*			msgSource = message->nodeNameSource();
    const char*			msgTarget = message->nodeNameTarget();
	const char*			msgGroup = message->group();
	const char*			msgCommand = message->command();
	const char*			msgPayload = message->payload();

// to all hosts
    if( strncmp( (char*)msgTarget, "all", 3 ) == 0 ){

    // ping
        if( strncmp(msgCommand,"ping",4) == 0 ){

			const char* tempHostName = NULL;
			coCore::ptr->hostNameGet( &tempHostName, NULL );

        // add the message to list
            coCore::ptr->plugins->messageQueue->add( this,
            myHostName, msgSource,
            msgGroup, "pong", "" );


            return coPlugin::REPLY;
        }

    }

// pong
    if( strncmp(msgCommand,"pong",4) == 0 ){

    #ifndef MQTT_ONLY_LOCAL
    // append hostname to known hosts
        this->appendKnownNodes( msgSource );
    #endif

    }

// to "localhost" or to the nodename-host
    if( strncmp(msgTarget,"localhost",9) != 0 &&
	coCore::ptr->isHostName(msgTarget) == false ){
        return coPlugin::NO_REPLY;
    }



// get version
    if( strncmp( (char*)msgCommand, "versionGet", 10 ) == 0 ){

    // add the message to list
        coCore::ptr->plugins->messageQueue->add( this,
        myHostName, msgSource,
        msgGroup, "version", copilotVersion );

        return coPlugin::REPLY;
    }

// get hosts
    if( strncmp( (char*)msgCommand, "nodesGet", 8 ) == 0 ){
        nodesGet:
	// vars
		//json_t*			jsonArray = json_array();
		json_t*			jsonObject = NULL;
		char*			jsonArrayChar = NULL;

		coCore::ptr->config->nodesGet( &jsonObject );
		jsonArrayChar = json_dumps( jsonObject, JSON_PRESERVE_ORDER | JSON_COMPACT );

    // add the message to list
        coCore::ptr->plugins->messageQueue->add( this,
        myHostName, msgSource,
        msgGroup, "nodes", jsonArrayChar );

	// free
		free(jsonArrayChar);
		//json_decref(jsonArray);

        return coPlugin::REPLY;
    }


	if( strncmp( (char*)msgCommand, "nodeRemove", 10 ) == 0 ){
        coCore::ptr->config->nodeRemove( msgPayload );
        goto nodesGet;
    }


	if( strncmp( (char*)msgCommand, "nodeClientAdd", 13 ) == 0 ){

    // vars
        json_error_t                jsonError;
        json_t*                     jsonPayload = NULL;
        json_t*                     jsonValue = NULL;

        const char*                 nodeName = NULL;
        const char*                 nodeHostName = NULL;
        int                         nodeHostPostPort = 0;
        coCoreConfig::nodeType      nodeType = coCoreConfig::UNKNOWN;



	// parse json
		jsonPayload = json_loads( msgPayload, JSON_PRESERVE_ORDER, &jsonError );
		if( jsonPayload == NULL || jsonError.line > -1 ){
			return coPlugin::NO_REPLY;
		}

        nodeType = coCoreConfig::CLIENT;

		jsonValue = json_object_get( jsonPayload, "node" );
        nodeName = json_string_value( jsonValue );

		jsonValue = json_object_get( jsonPayload, "host" );
        nodeHostName = json_string_value( jsonValue );

		jsonValue = json_object_get( jsonPayload, "port" );
        nodeHostPostPort = json_integer_value( jsonValue );


        coCore::ptr->config->nodesIterate();
        coCore::ptr->config->nodeAppend( nodeName );
        coCore::ptr->config->nodeInfo( NULL, &nodeType, true );
        coCore::ptr->config->nodeConnInfo( &nodeHostName, &nodeHostPostPort, true );
        coCore::ptr->config->save();
        coCore::ptr->config->nodesIterateFinish();

        goto nodesGet;
    }


	if( strncmp( (char*)msgCommand, "nodesSave", 9 ) == 0 ){
        coCore::ptr->config->save();
    }


    return coPlugin::NO_REPLY;
}






#endif // ldapService_C
