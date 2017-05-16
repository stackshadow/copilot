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

// plugin-list
    etListAlloc( this->pluginList );
    this->pluginListEnd = this->pluginList;
    this->iterator = NULL;

// get the hostinfo
    uname( &this->hostInfo );
    this->hostNodeNameLen = strlen( this->hostInfo.nodename );

// init libsodium
    if (sodium_init() == -1) {
        //return 1;
    }

	this->broadcastBusy = false;

// save the instance
    this->ptr = this;

}

coCore::                    ~coCore(){

    coPluginElement*    pluginElement = NULL;

    this->pluginsIterate();
    while( this->pluginNext(&pluginElement) == true ){
        delete pluginElement->plugin;
        delete pluginElement;
    }

    etListFree( this->pluginList );
}



void coCore::               setHostName( const char *hostname ){
    memset( this->hostInfo.nodename, 0, _UTSNAME_NODENAME_LENGTH );
    strncat( this->hostInfo.nodename, hostname, _UTSNAME_NODENAME_LENGTH );
}



bool coCore::               registerPlugin( coPlugin* plugin, const char *hostName, const char *group ){
// check
    if( this->pluginListEnd == NULL ) return false;

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
    etDebugMessage( etID_LEVEL_DETAIL, etDebugTempMessage );

// add it to the list
    etListAppend( this->pluginListEnd, (void*)listELement );

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
            etDebugMessage( etID_LEVEL_DETAIL, etDebugTempMessage );

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
        json_object_set_new( jsonAuthObject, "type", json_string("plain") );
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
    int                     cmpResult = -1;

    json_error_t            jsonError;
    json_t*                 jsonAnswerArray = json_array();
    json_t*                 jsonAnswerObject = NULL;

// locked ?
	while( this->broadcastBusy == true ){
		sleep(1);
	}
	this->broadcastBusy = true;

// get the source plugin element
    if( pluginElementGet(source,&pluginElement) == false ){
		this->broadcastBusy = false;
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


sendToAll:
    // iterate
        if( pluginElement->plugin != NULL ){

            snprintf( etDebugTempMessage, etDebugTempMessageLen, "Run onMessage() for Plugin: %s", pluginElement->plugin->name() );
            etDebugMessage( etID_LEVEL_DETAIL, etDebugTempMessage );

            jsonAnswerObject = json_object();
            if( pluginElement->plugin->onBroadcastMessage( msgHostName, msgGroup, msgCommand, msgPayload, jsonAnswerObject ) == false ){
                json_decref( jsonAnswerObject );
                snprintf( etDebugTempMessage, etDebugTempMessageLen, "Plugin '%s' requests break", pluginElement->plugin->name() );
                etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
                break;
            }

        // manipulate the topic
            if( coCore::setTopic( pluginElement, jsonAnswerObject, msgGroup ) == false ){
                snprintf( etDebugTempMessage, etDebugTempMessageLen, "Plugin '%s' dont provide a topic", pluginElement->plugin->name() );
                etDebugMessage( etID_LEVEL_DETAIL, etDebugTempMessage );
                continue;
            }


            json_array_append_new( jsonAnswerArray, jsonAnswerObject );
        }


    }

reply:
    if( json_array_size(jsonAnswerArray) > 0 ){
    // answer back
        source->onBroadcastReply( jsonAnswerArray );
    }

// cleanup
    json_decref( jsonAnswerArray );
	this->broadcastBusy = false;
}


void coCore::               mainLoop(){
    while(1){
        sleep(5);
    }
}



#endif

















