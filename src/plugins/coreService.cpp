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



//#ifndef MQTT_ONLY_LOCAL
void coreService:: 						appendKnownNodes( const char* nodeName ){
    if( nodeName == NULL ) return;

    coCore::ptr->config->nodesIterate();
    if( coCore::ptr->config->nodeSelect( nodeName ) == false ){

    // debug
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "Host '%s' unknown, append it.", nodeName );
        etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );

    // append the host
        coCoreConfig::nodeType hostType = coCoreConfig::CLIENT_IN;

        coCore::ptr->config->nodeAppend( nodeName );
        coCore::ptr->config->nodeInfo( NULL, &hostType, true );
        coCore::ptr->config->nodeConnInfo( &nodeName, NULL, true );
        coCore::ptr->config->nodesIterateFinish();

        coCore::ptr->config->save();
        return;
    } else {
    // debug
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "Host '%s' known.", nodeName );
        etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
    }
    coCore::ptr->config->nodesIterateFinish();
}
//#endif


coPlugin::t_state coreService::			onBroadcastMessage( coMessage* message ){

// vars
    const char*         myNodeName = coCore::ptr->nodeName();
	const char*			msgSource = message->nodeNameSource();
    const char*			msgTarget = message->nodeNameTarget();
	const char*			msgGroup = message->group();
	const char*			msgCommand = message->command();
    int                 msgCommandLen = 0;
	const char*			msgPayload = message->payload();

// check
    if( msgCommand == NULL ) return coPlugin::NO_REPLY;
    msgCommandLen = strlen(msgCommand);


// to all hosts
    if( strncmp( (char*)msgTarget, "all", 3 ) == 0 ){

    // ping
        if( strncmp(msgCommand,"ping",4) == 0 ){

        // add the message to list
            coCore::ptr->plugins->messageQueue->add( this,
            myNodeName, msgSource,
            msgGroup, "pong", "" );


            return coPlugin::REPLY;
        }

    }

// pong
    if( strncmp(msgCommand,"pong",4) == 0 ){

//    #ifndef MQTT_ONLY_LOCAL
    // append hostname to known hosts
        this->appendKnownNodes( msgSource );
//    #endif

    }

// to "localhost" or to the nodename-host
    if( strncmp(msgTarget,"localhost",9) != 0 &&
	coCore::ptr->isNodeName(msgTarget) == false ){
        return coPlugin::NO_REPLY;
    }



// get version
    if( coCore::strIsExact("versionGet",msgCommand,msgCommandLen) == true ){

    // add the message to list
        coCore::ptr->plugins->messageQueue->add( this,
        myNodeName, msgSource,
        msgGroup, "version", copilotVersion );

        return coPlugin::REPLY;
    }



// get hosts
    if( coCore::strIsExact("nodesGet",msgCommand,msgCommandLen) == true ){
        nodesGet:
	// vars
		//json_t*			jsonArray = json_array();
		json_t*			jsonObject = NULL;
		char*			jsonArrayChar = NULL;

		coCore::ptr->config->nodesGet( &jsonObject );
		jsonArrayChar = json_dumps( jsonObject, JSON_PRESERVE_ORDER | JSON_COMPACT );

    // add the message to list
        coCore::ptr->plugins->messageQueue->add( this,
        myNodeName, msgSource,
        msgGroup, "nodes", jsonArrayChar );

	// free
		free(jsonArrayChar);
		//json_decref(jsonArray);

        return coPlugin::REPLY;
    }


    if( coCore::strIsExact("nodeForEditGet",msgCommand,msgCommandLen) == true ){

	// vars
		//json_t*			jsonArray = json_array();
		json_t*			jsonObject = NULL;
		char*			jsonArrayChar = NULL;


        coCore::ptr->config->nodesIterate();
		if( coCore::ptr->config->nodeSelect( msgPayload ) == false ){
            coCore::ptr->config->nodesIterateFinish();
            return coPlugin::REPLY;
        }
        coCore::ptr->config->nodeGet( &jsonObject );

    // add jsonNode
        json_object_set( jsonObject, "nodename", json_string(msgPayload) );
		jsonArrayChar = json_dumps( jsonObject, JSON_PRESERVE_ORDER | JSON_COMPACT );
        json_object_del( jsonObject, "nodename" );

        coCore::ptr->config->nodesIterateFinish();

    // add the message to list
        coCore::ptr->plugins->messageQueue->add( this, myNodeName, msgSource, msgGroup, "nodeForEdit", jsonArrayChar );

	// free
		free(jsonArrayChar);

        return coPlugin::REPLY;
    }


	if( coCore::strIsExact("nodeRemove",msgCommand,msgCommandLen) == true ){
        coCore::ptr->config->nodeRemove( msgPayload );
        bool configSaved = coCore::ptr->config->save();

    // add the message to list
		if( configSaved == true ){
			coCore::ptr->plugins->messageQueue->add( this,
			myNodeName, msgSource,
			msgGroup, "configSaved", "" );
		} else {
			coCore::ptr->plugins->messageQueue->add( this,
			myNodeName, msgSource,
			msgGroup, "configNotSaved", "" );
		}

        return coPlugin::MESSAGE_FINISHED;
    }


	if( coCore::strIsExact("nodeSave",msgCommand,msgCommandLen) == true ){

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
            snprintf( etDebugTempMessage, etDebugTempMessageLen, "%s: %s", __PRETTY_FUNCTION__, jsonError.text );
            etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );

			return coPlugin::MESSAGE_FINISHED;
		}

        nodeType = coCoreConfig::CLIENT;

		jsonValue = json_object_get( jsonPayload, "node" );
        nodeName = json_string_value( jsonValue );

        jsonValue = json_object_get( jsonPayload, "type" );
        if( jsonValue != NULL ){
            nodeType = (coCoreConfig::nodeType)json_integer_value( jsonValue );
        }

		jsonValue = json_object_get( jsonPayload, "host" );
        nodeHostName = json_string_value( jsonValue );

		jsonValue = json_object_get( jsonPayload, "port" );
        nodeHostPostPort = json_integer_value( jsonValue );


        coCore::ptr->config->nodesIterate();
        coCore::ptr->config->nodeAppend( nodeName );
        coCore::ptr->config->nodeInfo( NULL, &nodeType, true );
        coCore::ptr->config->nodeConnInfo( &nodeHostName, &nodeHostPostPort, true );
        bool configSaved = coCore::ptr->config->save();
        coCore::ptr->config->nodesIterateFinish();

    // add the message to list
		if( configSaved == true ){
			coCore::ptr->plugins->messageQueue->add( this,
			myNodeName, msgSource,
			msgGroup, "configSaved", "" );
		} else {
			coCore::ptr->plugins->messageQueue->add( this,
			myNodeName, msgSource,
			msgGroup, "configNotSaved", "" );
		}

        return coPlugin::MESSAGE_FINISHED;
    }





    return coPlugin::NO_REPLY;
}






#endif // ldapService_C
