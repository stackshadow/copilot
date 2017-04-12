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




coCore* coCore::ptr = NULL;
coCore::                    coCore(){

// plugin-list
    etListAlloc( this->start );
    this->end = this->start;
    this->iterator = NULL;

// get the hostinfo
    uname( &this->hostInfo );
    this->hostNodeNameLen = strlen( this->hostInfo.nodename );

// save the instance
    this->ptr = this;

}

coCore::                    ~coCore(){

    coPluginElement*    pluginElement = NULL;
    
    this->iterate();
    while( this->next(&pluginElement) == true ){
        delete pluginElement->plugin;
        delete pluginElement;
    }

    etListFree( this->start );
}



bool coCore::               registerPlugin( coPlugin* plugin, const char *hostName, const char *group ){
// check
    if( this->end == NULL ) return false;

// vars
    const char*         pluginName = plugin->name();
    coPluginElement*    listELement = new coPluginElement();

// save
    etStringCharSet( listELement->listenHostName, hostName, -1 );
    etStringCharSet( listELement->listenGroup, group, -1 );
    listELement->plugin = plugin;

// debug
    snprintf( etDebugTempMessage, etDebugTempMessageLen, "Register Plugin: %s", pluginName );
    etDebugMessage( etID_LEVEL_DETAIL, etDebugTempMessage );

// add it to the list
    etListAppend( this->end, (void*)listELement );

}


bool coCore::               removePlugin( coPlugin* plugin ){

// iterate
    coPluginElement*        pluginElement = NULL;

// iterate through the list
    this->iterate();
    while( this->next(&pluginElement) == true ){

        if( pluginElement->plugin == plugin ){

        // debug
            snprintf( etDebugTempMessage, etDebugTempMessageLen, "Remove Plugin: %s", pluginElement->plugin->name() );
            etDebugMessage( etID_LEVEL_DETAIL, etDebugTempMessage );

            etListDataRemove( this->start, pluginElement, etID_TRUE );
            return true;
        }

    }


    return false;
}


void coCore::               listPlugins( json_t* pluginNameArray ){

// iterate
    coPluginElement*        pluginElement = NULL;

// iterate through the list
    this->iterate();
    while( this->next(&pluginElement) == true ){

        const char* pluginName = pluginElement->plugin->name();

        json_t*     jsonPlugin = json_object();
        json_object_set_new( jsonPlugin, "name", json_string(pluginName) );

        json_array_append( pluginNameArray, jsonPlugin );

    }

}




void coCore::               iterate(){
    etListIterate( this->start, this->iterator );
}

bool coCore::               next( coPluginElement** pluginElement ){
    coPluginElement*    listELement = NULL;
    if( etListIterateNext( this->iterator, listELement ) == etID_YES ){
        *pluginElement = listELement;
        return true;
    }

    *pluginElement = NULL;
    return false;
}

bool coCore::               next( coPlugin **plugin ){

    coPluginElement*    listELement = NULL;
    
    if( etListIterateNext( this->iterator, listELement ) == etID_YES ){
        *plugin = listELement->plugin;
        return true;
    }        


    *plugin = NULL;
    return false;
}

bool coCore::               nextAviable(){
    if( etListIterateNextAviable(this->iterator) == etID_YES ) return true;
    return false;
}



bool coCore::               setTopic( coPluginElement* pluginElement, const char* alternHostName, json_t* jsonAnswerObject ){
// check
    if( alternHostName == NULL ) return false;
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
        fullTopic += alternHostName;
    }

// group
    etStringCharGet( pluginElement->listenGroup, hostGroupChar );
    fullTopic += "/";
    fullTopic += hostGroupChar;

// topic
    fullTopic += "/";
    fullTopic += json_string_value(jsonTopic);
    
// write it back
    json_object_set_new( jsonAnswerObject, "topic", json_string(fullTopic.c_str()) );
    
    return true;
}



void coCore::               broadcast( coPlugin *source,
                                        const char* msgHostName,
                                        const char* msgGroup,
                                        const char* msgCommand,
                                        json_t* jsonData ){
// from here the plugins start
    if( this->start == NULL ) return;
    if( msgGroup == NULL ) return;
    if( msgCommand == NULL ) return;

// iterate
    coPluginElement*        pluginElement = NULL;
    void*                   pluginIterator = NULL;
    void*                   pluginIteratorData = NULL;
    const char*             pluginTopic = NULL;
    int                     pluginHostNameLen = 0;
    const char*             pluginHostName = NULL;
    const char*             pluginGroup = NULL;
    const char*             pluginCommand = NULL;
    const char*             pluginName = NULL;
    int                     cmpResult = -1;

    json_t*                 jsonAnswerArray = json_array();
    json_t*                 jsonAnswerObject = NULL;





// iterate through the list
    this->iterate();
    while( this->next(&pluginElement) == true ){

    // we dont send it to source
        if( pluginElement->plugin == source ) continue;


// common commands
    
    // discover
        if( strncmp(msgGroup,"co",2) == 0 ){
            if( strncmp(msgCommand,"discover",8) == 0 ){
                coPluginElement* tmpPluginElement = NULL;

                etListIterate( this->start, pluginIterator );
                while( etListIterateNext( pluginIterator, tmpPluginElement ) == etID_YES  ){

                    jsonAnswerObject = json_object();
                    json_object_set_new( jsonAnswerObject, "topic", json_string("service") );
                    coCore::setTopic( pluginElement, msgHostName, jsonAnswerObject );
                    json_object_set_new( jsonAnswerObject, "payload", json_string("") );
                    
                    json_array_append_new( jsonAnswerArray, jsonAnswerObject );

                    goto reply;
                }

            }
        }



    // pluginGroup
        etStringCharGet( pluginElement->listenGroup, pluginGroup );
        if( pluginGroup == NULL ) continue;
    // check
        cmpResult = strncmp( pluginGroup, msgGroup, strlen(msgGroup) );
        if( cmpResult != 0 ) continue;


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
            if( pluginElement->plugin->onMessage( msgHostName, msgGroup, msgCommand, jsonData, jsonAnswerObject ) == false ){
                json_decref( jsonAnswerObject );
                continue;
            }
            
        // manipulate the topic
            coCore::setTopic( pluginElement, msgHostName, jsonAnswerObject );


            json_array_append_new( jsonAnswerArray, jsonAnswerObject );
        }


    }

reply:

// answer back
    source->broadcastReply( jsonAnswerArray );

// cleanup
    json_decref( jsonAnswerArray );

}






#endif

















