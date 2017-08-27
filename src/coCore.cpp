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


#ifndef doCore_C
#define doCore_C


#include "coCore.h"
#include <sodium.h>



coCore* coCore::ptr = NULL;
coCore::                    coCore(){

// init the semaphore
	sem_init( &this->mutex, 0, 1 );

// plugin-list
    etListAlloc( this->pluginList );
    this->iterator = NULL;

// get the hostinfo
    uname( &this->hostInfo );
    this->hostNodeNameLen = strlen( this->hostInfo.nodename );

// init libsodium
    if (sodium_init() == -1) {
        //return 1;
    }

// create temporary message
	this->broadcastMessage = new coMessage();

// load config
	this->configLoad();

// save the instance
    this->ptr = this;

}

coCore::                    ~coCore(){

    coPluginElement*    pluginElement = NULL;

    this->pluginsIterate();
    while( this->pluginNext(&pluginElement) == true ){
//        delete pluginElement->plugin;
        delete pluginElement;
    }

    etListFree( this->pluginList );

	delete( this->broadcastMessage );
}





bool coCore::				configLoad(){
	sem_wait( &this->mutex );

// vars
    json_error_t    jsonError;
	json_t*			jsonValue;
	bool			saveToFile = false;

// clear
	this->jsonConfig = NULL;
	this->jsonNodes = NULL;
	this->jsonNodesIterator = NULL;


// open the file
    this->jsonConfig = json_load_file( configFile("core.json"), JSON_PRESERVE_ORDER, &jsonError );
    if( this->jsonConfig == NULL ){
        this->jsonConfig = json_object();
		saveToFile = true;
    }

// get nodes
	this->jsonNodes = json_object_get( this->jsonConfig, "nodes" );
	if( this->jsonNodes == NULL ){
		this->jsonNodes = json_object();

		jsonValue = json_object();
		json_object_set_new( jsonValue, "host", json_string("") );
		json_object_set_new( jsonValue, "port", json_integer(0) );
		json_object_set_new( jsonValue, "type", json_integer(coCore::UNKNOWN) );

		json_object_set_new( this->jsonNodes, "dummyNode", jsonValue );
		json_object_set_new( this->jsonConfig, "nodes", this->jsonNodes );
		saveToFile = true;
	}

	sem_post( &this->mutex );

	if( saveToFile == true ){
		this->configSave(NULL);
	}

	return true;
}


bool coCore::				configSave( const char* jsonString ){
	sem_wait( &this->mutex );

// vars
	json_t*			jsonObject = NULL;
	json_error_t	jsonError;

	if( jsonString != NULL ){
	// parse json
		jsonObject = json_loads( jsonString, JSON_PRESERVE_ORDER | JSON_INDENT(4), &jsonError );
		if( jsonObject == NULL | jsonError.line > -1 ){
			snprintf( etDebugTempMessage, etDebugTempMessageLen, "json-error: %s", jsonError.text );
			etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
			sem_post( &this->mutex );
			return false;
		}

	// cool, we parse the json, now we can save it
	// but before we release the old memory
		if( this->jsonConfig != NULL ){
			json_decref( this->jsonConfig );
		}

	// set
		this->jsonConfig = jsonObject;
	}

// save the json to file
	if( json_dump_file( this->jsonConfig, configFile("core.json"), JSON_PRESERVE_ORDER | JSON_INDENT(4) ) == 0 ){
		sem_post( &this->mutex );
		return true;
	}

	sem_post( &this->mutex );
	return false;
}


bool coCore::				nodesGet( json_t** jsonObject ){
	if( jsonObject == NULL ) return false;
	if( this->jsonNodes == NULL ) return false;

	*jsonObject = this->jsonNodes;
	return true;
}


bool  coCore::				nodesArrayGet( json_t* jsonArray ){
	if( jsonArray == NULL ) return false;
	if( this->jsonNodes == NULL ) return false;

	sem_wait( &this->mutex );


// vars
	void*		iterator = NULL;

	iterator = json_object_iter( this->jsonNodes );
	while( iterator != NULL ){
		json_array_append_new( jsonArray, json_string(json_object_iter_key(iterator)) );
		iterator = json_object_iter_next( this->jsonNodes, iterator );
	}

// return
	sem_post( &this->mutex );
	return true;
}


bool coCore::				nodesIterate(){
	sem_wait( &this->mutex );
	return true;
}


bool coCore::				nodeNext( const char** name, coCore::nodeType* type, bool set ){

// vars
	json_t*		jsonVar = NULL;

// iterate from beginning
	if( this->jsonNodesIterator == NULL ){
		this->jsonNodesIterator = json_object_iter( this->jsonNodes );
	}

// next node
	this->jsonNodesIterator = json_object_iter_next( this->jsonNodes, this->jsonNodesIterator );
	if( this->jsonNodesIterator == NULL ){
		return false;
	}

// get name and type
	this->jsonNode = json_object_iter_value( this->jsonNodesIterator );

// name
	*name = json_object_iter_key( this->jsonNodesIterator );

// type
	jsonVar = json_object_get( this->jsonNode, "type" );
	if( jsonVar == NULL ){
		jsonVar = json_integer( coCore::UNKNOWN );
		json_object_set_new( this->jsonNode, "type", jsonVar );
	}
	*type = (coCore::nodeType)json_integer_value( jsonVar );

	return true;
}


bool coCore::				nodeConnInfo( const char** host, int* port ){

// vars
	json_t*		jsonVar = NULL;

// host
	if( host != NULL ){
		if( *host = NULL ){
			jsonVar = json_object_get( this->jsonNode, "host" );
			if( jsonVar == NULL ){
				return false;
			}
			*host = json_string_value( jsonVar );
		}
	}

// port
	jsonVar = json_object_get( this->jsonNode, "port" );
	if( jsonVar == NULL ){
		return false;
	}
	if( port != NULL ){
		*port = (int)json_integer_value( jsonVar );
	}

// return
	return true;
}


bool coCore::				nodesIterateFinish(){
	sem_post( &this->mutex );
	return true;
}





void coCore::               setHostName( const char *hostname ){
    memset( this->hostInfo.nodename, 0, _UTSNAME_NODENAME_LENGTH );
    strncat( this->hostInfo.nodename, hostname, _UTSNAME_NODENAME_LENGTH );
}

bool coCore::				hostNameGet( const char** hostName, int* hostNameChars ){
	if( hostName != NULL ){
		*hostName = (const char*)&this->hostInfo.nodename;
	}
	if( hostNameChars != NULL ){
		*hostNameChars = this->hostNodeNameLen;
	}
	return true;
}

bool coCore::				hostNameAppend( etString* string ){
	etStringCharAdd( string, this->hostInfo.nodename );
}

bool coCore::				isHostName( const char* hostNameToCheck ){
	if( strncmp( this->hostInfo.nodename, hostNameToCheck, this->hostNodeNameLen) == 0 ){
		return true;
	}
	return false;
}



bool coCore::               registerPlugin( coPlugin* plugin, const char *hostName, const char *group ){
// check
    if( this->pluginList == NULL ) return false;

// vars
    const char*         pluginName = plugin->name();
    coPluginElement*    listELement = NULL;

// create element
    listELement = new coPluginElement();
    etStringCharSet( listELement->listenHostName, hostName, -1 );
    etStringCharSet( listELement->listenGroup, group, -1 );
    listELement->plugin = plugin;

// debug
    snprintf( etDebugTempMessage, etDebugTempMessageLen, "Register Plugin: %s", pluginName );
    etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );

// add it to the list
    etListAppend( this->pluginList, (void*)listELement );

}


bool coCore::               removePlugin( coPlugin* plugin ){

// iterate
    coPluginElement*        pluginElement = NULL;

// iterate through the list
    this->pluginsIterate();
    while( this->pluginNext(&pluginElement) == true ){

        if( pluginElement->plugin == plugin ){

        // debug
            snprintf( etDebugTempMessage, etDebugTempMessageLen, "Remove Plugin: %s", pluginElement->plugin->name() );
            etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );

            etListDataRemove( this->pluginList, pluginElement, etID_TRUE );
            return true;
        }

    }


    return false;
}


void coCore::               listPlugins( json_t* pluginNameArray ){

// iterate
    coPluginElement*        pluginElement = NULL;

// iterate through the list
    this->pluginsIterate();
    while( this->pluginNext(&pluginElement) == true ){

        const char* pluginName = pluginElement->plugin->name();

        json_t*     jsonPlugin = json_object();
        json_object_set_new( jsonPlugin, "name", json_string(pluginName) );

        json_array_append( pluginNameArray, jsonPlugin );

    }

}






bool coCore::               pluginsIterate(){
    etListIterate( this->pluginList, this->iterator );
    return true;
}


bool coCore::               pluginNext( coPluginElement** pluginElement ){
    coPluginElement*    listELement = NULL;
    if( etListIterateNext( this->iterator, listELement ) == etID_YES ){
        *pluginElement = listELement;
        return true;
    }

    *pluginElement = NULL;
    return false;
}


bool coCore::               pluginNext( coPlugin **plugin ){

    coPluginElement*    listELement = NULL;

    if( etListIterateNext( this->iterator, listELement ) == etID_YES ){
        *plugin = listELement->plugin;
        return true;
    }


    *plugin = NULL;
    return false;
}


bool coCore::               pluginElementGet( coPlugin* plugin, coPluginElement** pluginElement ){

// vars
    void*               tempIterator;
    coPluginElement*    listELement = NULL;

    etListIterate( this->pluginList, tempIterator );
    while( etListIterateNext( tempIterator, listELement ) == etID_YES ){

        if( listELement->plugin == plugin ){
            *pluginElement = listELement;
            return true;
        }

    }

    return false;
}


bool coCore::               nextAviable(){
    if( etListIterateNextAviable(this->iterator) == etID_YES ) return true;
    return false;
}





bool coCore::               setTopic( coPluginElement* pluginElement, json_t* jsonAnswerObject, const char* msgGroup  ){
// check
    if( pluginElement == NULL ) return false;
    if( jsonAnswerObject == NULL ) return false;

// vars
    json_t*         jsonTopic;
    std::string     fullTopic;
    const char*     hostNameChar;
    const char*     hostGroupChar;

// get existing topic
    jsonTopic = json_object_get( jsonAnswerObject, "topic" );
    if( jsonTopic == NULL ) return false;


    fullTopic = "nodes/";

// hostname
    etStringCharGet( pluginElement->listenHostName, hostNameChar );
    if( strlen(hostNameChar) > 0 ){
        fullTopic += hostNameChar;
    } else {
        fullTopic += coCore::ptr->hostInfo.nodename;
    }

// group
    //etStringCharGet( pluginElement->listenGroup, hostGroupChar );
    //fullTopic += "/";
    //fullTopic += hostGroupChar;
	fullTopic += "/";
	fullTopic += msgGroup;

// topic
    fullTopic += "/";
    fullTopic += json_string_value(jsonTopic);

// write it back
    json_object_set_new( jsonAnswerObject, "topic", json_string(fullTopic.c_str()) );

    return true;
}


bool coCore::               jsonValue( json_t* jsonObject, const char* key, char* value, int valueMax, const char* defaultValue, bool toJson ){

// vars
    json_t*     jsonValue = NULL;

// from value to json
    if( toJson == true ){
        json_object_set_new( jsonObject, key, json_string(value) );
    }

// from json to value
    else {

    // clean
        memset( value, 0, valueMax );

    // set
        jsonValue = json_object_get( jsonObject, key );
        if( jsonValue != NULL ) {
            strncat( value, json_string_value(jsonValue), valueMax );
        } else {
            strncat( value, defaultValue, valueMax );
            json_object_set_new( jsonObject, key, json_string(defaultValue) );
        }

    }

    return true;
}


bool coCore::               jsonValue( json_t* jsonObject, const char* key, std::string* value, const char* defaultValue, bool toJson ){

// vars
    json_t*     jsonValue = NULL;

// from value to json
    if( toJson == true ){
        json_object_set_new( jsonObject, key, json_string(value->c_str()) );
    }

// from json to value
    else {

    // clean
        value->clear();


    // set
        jsonValue = json_object_get( jsonObject, key );
        if( jsonValue != NULL ) {
            value->assign( json_string_value(jsonValue) );
        } else {
            value->assign( defaultValue );
            json_object_set_new( jsonObject, key, json_string(defaultValue) );
        }

    }

    return true;
}


bool coCore::               passwordCheck( const char* user, const char* pass ){

// vars
    json_t*         jsonAuthObject = NULL;
    json_t*         jsonUserObject = NULL;
    json_error_t    jsonError;
    const char*     jsonPassword = NULL;
    int             jsonPasswordLen = 0;

// open the file
    jsonAuthObject = json_load_file( configFile("auth.json"), JSON_PRESERVE_ORDER, &jsonError );
    if( jsonAuthObject == NULL ){
        jsonAuthObject = json_object();
        json_object_set_new( jsonAuthObject, "username", json_object() );
        json_dump_file( jsonAuthObject, configFile("auth.json"), JSON_PRESERVE_ORDER | JSON_INDENT(4) );
    }

// try to get the user
    jsonUserObject = json_object_get( jsonAuthObject, user );
    if( jsonUserObject == NULL ) return false;

// try to get the password of the user
    jsonPasswordLen = 0;
    json_t* jsonPasswordObject = json_object_get( jsonUserObject, "salsa208sha256" );
    if( jsonPasswordObject != NULL ) {
        jsonPassword = json_string_value( jsonPasswordObject );
        jsonPasswordLen = strlen( jsonPassword );
    }

// no password provided yet, set it
    if( jsonPasswordLen == 0 ){

    // create the hashed password
        char hashed_password[crypto_pwhash_scryptsalsa208sha256_STRBYTES];
        if( crypto_pwhash_scryptsalsa208sha256_str( hashed_password, pass, strlen(pass),
                crypto_pwhash_scryptsalsa208sha256_OPSLIMIT_SENSITIVE,
                crypto_pwhash_scryptsalsa208sha256_MEMLIMIT_SENSITIVE) == 0) {
            json_object_set_new( jsonUserObject, "salsa208sha256", json_string(hashed_password) );
        }

    // save the auth-file
        json_dump_file( jsonAuthObject, configFile("auth.json"), JSON_PRESERVE_ORDER | JSON_INDENT(4) );

    // recalc password-stuff
        jsonPassword = hashed_password;
        jsonPasswordLen = strlen( jsonPassword );
    }


// verify password
    if( crypto_pwhash_scryptsalsa208sha256_str_verify(jsonPassword, pass, strlen(pass)) == 0) {
        return true;
    }


    return false;
}


bool coCore::               passwordChange( const char* user, const char* oldpw, const char* newpw ){

}


void coCore::               broadcast( coPlugin *source,
										const char* msgID,
                                        const char* msgHostName,
                                        const char* msgGroup,
                                        const char* msgCommand,
                                        const char* msgPayload ){
// from here the plugins start
    if( this->pluginList == NULL ) return;
    if( msgGroup == NULL ) return;
    if( msgCommand == NULL ) return;


// vars
    coPluginElement*        pluginElement = NULL;
    const char*             pluginHostName = NULL;
    int                     pluginHostNameLen = 0;
    const char*             pluginGroup = NULL;
    int                     pluginGroupLen = 0;
	coPlugin::t_state		pluginState = coPlugin::BREAK;
    int                     cmpResult = -1;

    json_error_t            jsonError;
    json_t*                 jsonAnswerArray = json_array();
    json_t*                 jsonAnswerObject = NULL;

// locked ?
	pthread_mutex_lock( &this->broadcastRunning );

// get the source plugin element
    if( pluginElementGet(source,&pluginElement) == false ){
		pthread_mutex_unlock( &this->broadcastRunning );
        return;
    }


// iterate through the list
    this->pluginsIterate();
    while( this->pluginNext(&pluginElement) == true ){

    // we dont ask the source
        if( pluginElement->plugin == source ) continue;


    // pluginGroup
        etStringCharGet( pluginElement->listenGroup, pluginGroup );
        if( pluginGroup == NULL ) continue;
    // check
        pluginGroupLen = strlen( pluginGroup );
        cmpResult = strncmp( pluginGroup, msgGroup, strlen(msgGroup) );
        if( cmpResult != 0 && pluginGroupLen > 0 ) continue;


    // hostname
        etStringCharGet( pluginElement->listenHostName, pluginHostName );
        if( pluginHostName == NULL ) continue;
    // check
        pluginHostNameLen = strlen(pluginHostName);
        cmpResult = strncmp( pluginHostName, msgHostName, pluginHostNameLen );
        if( cmpResult != 0 && pluginHostNameLen > 0 ) continue;


    // iterate
        if( pluginElement->plugin != NULL ){

			snprintf( etDebugTempMessage, etDebugTempMessageLen, "CMD %s", msgCommand );
            etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );

		// build the message
			this->broadcastMessage->reqID( msgID );
			this->broadcastMessage->hostName( msgHostName );
			this->broadcastMessage->group( msgGroup );
			this->broadcastMessage->command( msgCommand );
			this->broadcastMessage->payload( msgPayload );

			this->broadcastMessage->clearReply();

		// send it to the plugin and wait for response
            snprintf( etDebugTempMessage, etDebugTempMessageLen, "CALL %s::onBroadcastMessage()", pluginElement->plugin->name() );
            etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );

            jsonAnswerObject = json_object();
			pluginState = pluginElement->plugin->onBroadcastMessage( this->broadcastMessage );

		// plugin dont reply something
			if( pluginState == coPlugin::NO_REPLY ) continue;

		// plugin want to break
			if( pluginState == coPlugin::BREAK ) {
                snprintf( etDebugTempMessage, etDebugTempMessageLen, "Plugin '%s' requests break", pluginElement->plugin->name() );
                etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
                break;
			}

        // reply back to plugin
			source->onBroadcastReply( this->broadcastMessage );
        }


    }


// cleanup
	pthread_mutex_unlock( &this->broadcastRunning );
}


void coCore::               mainLoop(){
    while(1){
        sleep(5);
    }
}



#endif

















