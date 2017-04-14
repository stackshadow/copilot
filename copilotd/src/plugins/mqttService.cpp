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

#ifndef mqttService_C
#define mqttService_C

#include "plugins/mqttService.h"
#include "coCore.h"

mqttService* mqttService::ptr = NULL;
mqttService::               mqttService( char* host, int port ) : coPlugin( "mqttService" ){

// save
    mqttService::ptr = this;

// register this plugin
    coCore::ptr->registerPlugin( this, "", "" );

// save host / port
    memchr( this->hostName, 0, sizeof(this->hostName) );
    strncpy( this->hostName, host, sizeof(this->hostName) );
    this->hostPort = port;

// clean
    memchr( this->lastPubTopic, 0, sizeof(this->lastPubTopic) );
    

    int keepalive = 60;
    bool clean_session = true;


    mosquitto_lib_init();

    this->mosq = mosquitto_new(NULL, clean_session, NULL);
    if( this->mosq == NULL ){
        fprintf(stderr, "Error: Out of memory.\n");
        return;
    }

    //mosquitto_log_callback_set(mosq, my_log_callback);
    mosquitto_connect_callback_set( this->mosq, mqttService::cb_onConnect );
    mosquitto_disconnect_callback_set( this->mosq, mqttService::cb_onDisConnect );
    mosquitto_message_callback_set( this->mosq, mqttService::cb_onMessage );

    if( mosquitto_connect( this->mosq, this->hostName, this->hostPort, keepalive ) ){
        fprintf(stderr, "Unable to connect.\n");
        return;
    }

    mosquitto_loop_start( this->mosq );
    
    int subscribeID = 0;
    mosquitto_subscribe( this->mosq, &subscribeID, "#", 0 );

//    mosquitto_destroy( mqtt->mosq );
//    mosquitto_lib_cleanup();


}



void mqttService::          cb_onConnect( struct mosquitto* mosq, void* userdata, int result ){
    mqttService::ptr->connected = true;
}


void mqttService::          cb_onDisConnect( struct mosquitto* mosq, void* userdata, int result ){
    mqttService::ptr->connected = false;
}


void mqttService::          cb_onMessage( struct mosquitto *mosq, void *obj, const struct mosquitto_message *message){
    
    if(message->payloadlen){
         printf("%s %s\n", message->topic, message->payload );
    }else{
         printf("%s (null)\n", message->topic);
    }
    fflush(stdout);

// dont respond on our message
    int lastPubTopicLen = strlen( mqttService::ptr->lastPubTopic );
    if( lastPubTopicLen > 0 ){
        if( strncmp( mqttService::ptr->lastPubTopic, message->topic, lastPubTopicLen  ) == 0 ){
            memchr( mqttService::ptr->lastPubTopic, 0, sizeof(mqttService::ptr->lastPubTopic) );
            return;
        }
        memchr( mqttService::ptr->lastPubTopic, 0, sizeof(mqttService::ptr->lastPubTopic) );
    }

// we grab the hostname out of the topic
    const char* hostName = strtok( (char*)message->topic, "/" );
    hostName = strtok( NULL, "/" );
    const char* group = strtok( NULL, "/" );
    const char* cmd = strtok( NULL, "/" );
    json_t* jsonPayload = json_string( (char*)message->payload );

    coCore::ptr->broadcast( mqttService::ptr, hostName, group, cmd, jsonPayload );

    json_decref( jsonPayload );

}


bool mqttService::          broadcastReply( json_t* jsonAnswerArray ){
    
    char *dump = json_dumps( jsonAnswerArray, JSON_PRESERVE_ORDER | JSON_INDENT(4) );
    if( dump == NULL ){
        etDebugMessage( etID_LEVEL_ERR, "Json error" );
        return false;
    }
    etDebugMessage( etID_LEVEL_DETAIL, dump );
    free( dump );




// we send the data back
    json_t*     jsonAnswer = NULL;
    json_t*     jsonTopic = NULL;
    json_t*     jsonPayload = NULL;
    int jsonArrayLen = json_array_size(jsonAnswerArray);
    int jsonArrayIndex = 0;
    for( jsonArrayIndex = 0; jsonArrayIndex < jsonArrayLen; jsonArrayIndex++ ){

        jsonAnswer = json_array_get( jsonAnswerArray, jsonArrayIndex );
        jsonTopic = json_object_get( jsonAnswer, "topic" );
        jsonPayload = json_object_get( jsonAnswer, "payload" );
    
    // check
        if( jsonAnswer == NULL ) continue;
        if( jsonTopic == NULL ) continue;
        if( jsonPayload == NULL ) continue;
        
        const char* jsonPayloadChar = json_dumps( jsonPayload, JSON_PRESERVE_ORDER | JSON_COMPACT );
        int jsonPayloadCharLen = 0;
        if( jsonPayloadChar != NULL ){
            jsonPayloadCharLen = strlen( jsonPayloadChar );
        }
        
    // publish
        int messageID;
        mosquitto_publish( 
            this->mosq, 
            &messageID, 
            json_string_value(jsonTopic), 
            jsonPayloadCharLen,
            jsonPayloadChar,
            0,
            false );
            
    // save last publicated message
        memchr( this->lastPubTopic, 0, sizeof(this->lastPubTopic) );
        strncpy( this->lastPubTopic, json_string_value(jsonTopic), json_string_length(jsonTopic) );

        free( (char*)jsonPayloadChar );

    }


}


bool mqttService::          onMessage(  const char*     msgHostName, 
                                const char*     msgGroup, 
                                const char*     msgCommand, 
                                json_t*         jsonData, 
                                json_t*         jsonAnswerObject ){

// vars
    std::string         fullTopic = "nodes/";
    const char*         jsonDataChar;
    int                 jsonDataCharLen;
    
// build full topic
    fullTopic += msgHostName;
    fullTopic += "/";
    fullTopic += msgGroup;
    fullTopic += "/";
    fullTopic += msgCommand;

// we only accept message which send to all nodes
    if( strncmp( msgHostName, "all", 3 ) != 0 ){
        return true;
    }

// dump jsonData
    jsonDataChar = json_dumps( jsonData, JSON_PRESERVE_ORDER | JSON_COMPACT );
    jsonDataCharLen = 0;
    if( jsonDataChar != NULL ){
        jsonDataCharLen = strlen( jsonDataChar );
    }
    
    
// publish
    int messageID;
    mosquitto_publish( 
        this->mosq, 
        &messageID, 
        fullTopic.c_str(), 
        jsonDataCharLen,
        jsonDataChar,
        0,
        false );
    
// save last publicated message
    memchr( this->lastPubTopic, 0, sizeof(this->lastPubTopic) );
    strncpy( this->lastPubTopic, fullTopic.c_str(), fullTopic.length() );

    free( (void*)jsonDataChar );

    return true;
}




#endif