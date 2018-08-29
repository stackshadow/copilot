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
#include "core/coreService.h"
#include "version.h"
#include "pubsub.h"
#include <string>

coreService::                           coreService() {



// subscribe
	psBus::inst->subscribe( this, coCore::ptr->nodeName(), "co", NULL, coreService::onSubscriberMessage, NULL );
	psBus::inst->subscribe( this, "all", "co", NULL, coreService::onSubscriberMessage, NULL );
}


coreService::                           ~coreService(){

}



//#ifndef MQTT_ONLY_LOCAL
void coreService::                      appendKnownNodes( const char* nodeName ){
    if( nodeName == NULL ) return;

    coConfig::ptr->nodesIterate();
    if( coConfig::ptr->nodeSelect( nodeName ) == false ){

    // debug
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "Host '%s' unknown, append it.", nodeName );
        etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );

    // append the host
        coConfig::nodeType hostType = coConfig::CLIENT_IN;

        coConfig::ptr->nodeAppend( nodeName );
        coConfig::ptr->nodeInfo( NULL, &hostType, true );
        coConfig::ptr->nodeConnInfo( &nodeName, NULL, true );
        coConfig::ptr->nodesIterateFinish();

        coConfig::ptr->save();
        return;
    } else {
    // debug
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "Host '%s' known.", nodeName );
        etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
    }
    coConfig::ptr->nodesIterateFinish();
}
//#endif



int coreService::                       onSubscriberMessage(
    void* objectInstance,
    const char* id,
    const char* nodeSource,
    const char* nodeTarget,
    const char* group,
    const char* command,
    const char* payload,
    void* userdata ){

    // vars
    coreService*        coreServiceInst = (coreService*)objectInstance;
    int                 msgCommandLen = 0;
    const char*         myNodeName = coCore::ptr->nodeName();

    // check
    if( command == NULL ) return psBus::NEXT_SUBSCRIBER;
    msgCommandLen = strlen(command);

    // to all hosts
    if( strncmp( (char*)nodeTarget, "all", 3 ) == 0 || strncmp( (char*)nodeTarget, myNodeName, strlen(myNodeName) ) == 0 ){

    // ping
        if( strncmp(command,"ping",4) == 0 ){

        // publish
            psBus::inst->publish( coreServiceInst, id, myNodeName, nodeSource, group, "pong", "" );

            return psBus::END;
        }

    }

    // pong
    if( strncmp(command,"pong",4) == 0 ){

    //    #ifndef MQTT_ONLY_LOCAL
    // append hostname to known hosts
        coreServiceInst->appendKnownNodes( nodeSource );
    //    #endif

    }



// get nodename
    if( coCore::strIsExact("nodeNameGet",command,msgCommandLen) == true ){
        psBus::inst->publish( coreServiceInst, id, nodeTarget, nodeSource, group, "nodeName", myNodeName );
        return psBus::END;
    }

    // get version
    if( coCore::strIsExact("versionGet",command,msgCommandLen) == true ){

        psBus::inst->publish( coreServiceInst, id, nodeTarget, nodeSource, group, "version", copilotVersion );

        return psBus::END;
    }



    // get hosts
    if( coCore::strIsExact("nodeListGet",command,msgCommandLen) == true ){

    // vars
        json_t*         jsonObject = NULL;
        void*           jsonObjectIterator = NULL;
        json_t*         jsonResultArray = json_array();
        char*           jsonArrayChar = NULL;


    // create result-array
        coConfig::ptr->nodesGet( &jsonObject );
        jsonObjectIterator = json_object_iter( jsonObject );
        while( jsonObjectIterator != NULL ){

            const char* nodeName = json_object_iter_key( jsonObjectIterator );
            json_array_append_new( jsonResultArray, json_string(nodeName) );

            jsonObjectIterator = json_object_iter_next( jsonObject, jsonObjectIterator );
        }

        jsonArrayChar = json_dumps( jsonResultArray, JSON_PRESERVE_ORDER | JSON_COMPACT );

    // publish
        psBus::inst->publish( coreServiceInst, id, myNodeName, nodeSource, group, "nodeList", jsonArrayChar );

    // free
        free( jsonArrayChar );
        json_decref( jsonResultArray );


        return psBus::END;
    }


    if( coCore::strIsExact("nodesGet",command,msgCommandLen) == true ){
        nodesGet:
    // vars
        //json_t*			jsonArray = json_array();
        json_t*         jsonObject = NULL;
        char*           jsonArrayChar = NULL;

        coConfig::ptr->nodesGet( &jsonObject );
        jsonArrayChar = json_dumps( jsonObject, JSON_PRESERVE_ORDER | JSON_COMPACT );

    // publish
        psBus::inst->publish( coreServiceInst, id, myNodeName, nodeSource, group, "nodes", jsonArrayChar );

    // free
        free(jsonArrayChar);
        //json_decref(jsonArray);

        return psBus::END;
    }


    if( coCore::strIsExact("nodeForEditGet",command,msgCommandLen) == true ){

    // vars
        //json_t*			jsonArray = json_array();
        json_t*			jsonObject = NULL;
        char*			jsonArrayChar = NULL;

    // publish
        psBus::inst->publish( coreServiceInst, id, myNodeName, nodeSource, group, "nodes", jsonArrayChar );

        coConfig::ptr->nodesIterate();
        if( coConfig::ptr->nodeSelect( payload ) == false ){
            coConfig::ptr->nodesIterateFinish();
            return psBus::END;
        }
        coConfig::ptr->nodeGet( &jsonObject );

    // add jsonNode
        json_object_set( jsonObject, "nodename", json_string(payload) );
        jsonArrayChar = json_dumps( jsonObject, JSON_PRESERVE_ORDER | JSON_COMPACT );
        json_object_del( jsonObject, "nodename" );

        coConfig::ptr->nodesIterateFinish();

    // publish
        psBus::inst->publish( coreServiceInst, id, myNodeName, nodeSource, group, "nodeForEdit", jsonArrayChar );

    // free
        free(jsonArrayChar);

        return psBus::END;
    }


    if( coCore::strIsExact("nodeRemove",command,msgCommandLen) == true ){
        coConfig::ptr->nodeRemove( payload );
        bool configSaved = coConfig::ptr->save();

    // add the message to list
        if( configSaved == true ){
        // publish
            psBus::inst->publish( coreServiceInst, id, myNodeName, nodeSource, group, "configSaved", "" );
        } else {
        // publish
            psBus::inst->publish( coreServiceInst, id, myNodeName, nodeSource, group, "configNotSaved", "" );
        }

        return psBus::END;
    }


    if( coCore::strIsExact("nodeSave",command,msgCommandLen) == true ){

    // vars
        json_error_t                jsonError;
        json_t*                     jsonPayload = NULL;
        json_t*                     jsonValue = NULL;

        const char*                 nodeName = NULL;
        const char*                 nodeHostName = NULL;
        int                         nodeHostPostPort = 0;
        coConfig::nodeType      nodeType = coConfig::UNKNOWN;



    // parse json
        jsonPayload = json_loads( payload, JSON_PRESERVE_ORDER, &jsonError );
        if( jsonPayload == NULL || jsonError.line > -1 ){
            snprintf( etDebugTempMessage, etDebugTempMessageLen, "%s: %s", __PRETTY_FUNCTION__, jsonError.text );
            etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );

            return psBus::END;
        }

        nodeType = coConfig::CLIENT;

        jsonValue = json_object_get( jsonPayload, "node" );
        nodeName = json_string_value( jsonValue );

        jsonValue = json_object_get( jsonPayload, "type" );
        if( jsonValue != NULL ){
            nodeType = (coConfig::nodeType)json_integer_value( jsonValue );
        }

        jsonValue = json_object_get( jsonPayload, "host" );
        nodeHostName = json_string_value( jsonValue );

        jsonValue = json_object_get( jsonPayload, "port" );
        nodeHostPostPort = json_integer_value( jsonValue );


        coConfig::ptr->nodesIterate();
        coConfig::ptr->nodeAppend( nodeName );
        coConfig::ptr->nodeInfo( NULL, &nodeType, true );
        coConfig::ptr->nodeConnInfo( &nodeHostName, &nodeHostPostPort, true );
        bool configSaved = coConfig::ptr->save();
        coConfig::ptr->nodesIterateFinish();

    // add the message to list
        if( configSaved == true ){
        // publish
            psBus::inst->publish( coreServiceInst, id, myNodeName, nodeSource, group, "configSaved", "" );
        } else {
        // publish
            psBus::inst->publish( coreServiceInst, id, myNodeName, nodeSource, group, "configNotSaved", "" );
        }

        return psBus::END;
    }



    return psBus::END;
}


#endif // ldapService_C
