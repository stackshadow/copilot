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
mqttService::               mqttService() : coPlugin( "mqttService" ){

// save
    mqttService::ptr = this;

// register this plugin
    coCore::ptr->registerPlugin( this, "", "" );

// save host / port
    memchr( this->hostName, 0, sizeof(this->hostName) );

// clean
    memchr( this->lastPubTopic, 0, sizeof(this->lastPubTopic) );

// connected clients
	this->connectedClients = 0;

// load config
    this->configLoad();

// clean pthread
    memset( &this->retryThread, 0, sizeof(this->retryThread) );


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

// we connect to mqtt
    //this->connect();
    pthread_create( &this->retryThread, NULL, mqttService::connectThread, this );
    pthread_detach( this->retryThread );

//    mosquitto_destroy( mqtt->mosq );
//    mosquitto_lib_cleanup();


}



bool mqttService::          configLoad(){

// vars
    bool            saveIt = false;
    json_error_t    jsonError;
    json_t*         jsonValue;
    const char*     hostName;
    int             hostNameLen = 0;

// load from file
    this->jsonConfigRoot = json_load_file( "/etc/copilot/mqtt.json", JSON_PRESERVE_ORDER, &jsonError );
    if( jsonError.position == 0 || jsonError.line >= 0 ){
        this->jsonConfigRoot = json_object();

        saveIt = true;
    }

// load from the existing json

// hostName
    jsonValue = json_object_get( this->jsonConfigRoot, "host" );
    if( jsonValue == NULL ){
        jsonValue = json_string("localhost");
        json_object_set_new( this->jsonConfigRoot, "host", jsonValue );
        saveIt = true;
    }
    hostName = json_string_value( jsonValue );
    hostNameLen = strlen( hostName );
    memchr( this->lastPubTopic, 0, sizeof(this->lastPubTopic) );
    strncpy( this->hostName, hostName, hostNameLen );

// port
    jsonValue = json_object_get( this->jsonConfigRoot, "port" );
    if( jsonValue == NULL ){
        jsonValue = json_integer( 1883 );
        json_object_set_new( this->jsonConfigRoot, "port", jsonValue );
        saveIt = true;
    }
    this->hostPort = json_integer_value( jsonValue );


// retryCount
    this->retryCount = 0;


// retryCountMax
    jsonValue = json_object_get( this->jsonConfigRoot, "retryCountMax" );
    if( jsonValue == NULL ){
        jsonValue = json_integer( 3 );
        json_object_set_new( this->jsonConfigRoot, "retryCountMax", jsonValue );
        saveIt = true;
    }
    this->retryCountMax = json_integer_value( jsonValue );

// retryDelay
    jsonValue = json_object_get( this->jsonConfigRoot, "retryDelay" );
    if( jsonValue == NULL ){
        jsonValue = json_integer( 10 );
        json_object_set_new( this->jsonConfigRoot, "retryDelay", jsonValue );
        saveIt = true;
    }
    this->retryDelay = json_integer_value( jsonValue );

// retryPause
    jsonValue = json_object_get( this->jsonConfigRoot, "retryPause" );
    if( jsonValue == NULL ){
        jsonValue = json_integer( 60 );
        json_object_set_new( this->jsonConfigRoot, "retryPause", jsonValue );
        saveIt = true;
    }
    this->retryPause = json_integer_value( jsonValue );


// save it if needed
    if( saveIt == true ){
        this->configSave();
    }

    return true;
}


bool mqttService::          configSave(){
    int returnCode = json_dump_file( this->jsonConfigRoot,
        "/etc/copilot/mqtt.json",
        JSON_PRESERVE_ORDER | JSON_INDENT(4)
    );

    if( returnCode != 0 ){
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "Could not save config to /etc/copilot/mqtt.json" );
        etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
        return false;
    }

    snprintf( etDebugTempMessage, etDebugTempMessageLen, "Saved config to /etc/copilot/mqtt.json" );
    etDebugMessage( etID_LEVEL_DETAIL, etDebugTempMessage );
    return true;
}


bool mqttService::          connect(){

    int keepalive = 60;

    if( mosquitto_connect( this->mosq, this->hostName, this->hostPort, keepalive ) ){

        this->connected = false;

    // debug message
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "Unable to connect" );
        etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );

        return false;
    }

    this->connected = true;
    mosquitto_loop_start( this->mosq );

    int subscribeID = 0;

// system subscriptions
	mosquitto_subscribe( this->mosq, &subscribeID, "$SYS/broker/clients/connected", 0 );


#ifdef MQTT_ONLY_LOCAL
    mosquitto_subscribe( this->mosq, &subscribeID, "nodes/all/#", 0 );
	std::string topic = "nodes/";
	topic += coCore::ptr->hostInfo.nodename;
	topic += "/#";
	mosquitto_subscribe( this->mosq, &subscribeID, topic.c_str(), 0 );
#else
    mosquitto_subscribe( this->mosq, &subscribeID, "#", 0 );
#endif

    //mosquitto_subscribe( this->mosq, &subscribeID, "$SYS/broker/load/messages/received/1min", 0 );

    return true;
}


void* mqttService::         connectThread( void* instance ){

    mqttService*    mqttInstance = (mqttService*)instance;

    while( mqttInstance->connected == false ){
        for( mqttInstance->retryCount = 0; mqttInstance->retryCount < mqttInstance->retryCountMax; mqttInstance->retryCount++ ){

            if( mqttInstance->connect() == true ){
                goto connectOk;
            }

            sleep( mqttInstance->retryDelay );
        }

        sleep( mqttInstance->retryPause );
    }

connectOk:
    return NULL;
}


bool mqttService::          disconnect(){
    this->connected = false;
    return true;
}




void mqttService::          cb_onConnect( struct mosquitto* mosq, void* userdata, int result ){
    mqttService::ptr->connected = true;
}


void mqttService::          cb_onDisConnect( struct mosquitto* mosq, void* userdata, int result ){
    mqttService::ptr->connected = false;
}


void mqttService::          cb_onMessage( struct mosquitto *mosq, void *obj, const struct mosquitto_message *message){

// debug
    if(message->payloadlen){
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "%s %s", message->topic, message->payload );
        etDebugMessage( etID_LEVEL_DETAIL, etDebugTempMessage );

    }else{
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "%s (null)", message->topic );
        etDebugMessage( etID_LEVEL_DETAIL, etDebugTempMessage );
    }
    fflush(stdout);

// dont respond on what we send out
    int lastPubTopicLen = strlen( mqttService::ptr->lastPubTopic );
    if( lastPubTopicLen > 0 ){
        if( strncmp( mqttService::ptr->lastPubTopic, message->topic, lastPubTopicLen  ) == 0 ){
            memset( mqttService::ptr->lastPubTopic, 0, sizeof(mqttService::ptr->lastPubTopic) );
            memchr( mqttService::ptr->lastPubTopic, 0, sizeof(mqttService::ptr->lastPubTopic) );
            return;
        }
    }

// we grab the hostname from the topic
    int     topicTempLen = strlen(message->topic);
    char    topicTemp[topicTempLen + 1];
    memset( topicTemp, 0, topicTempLen + 1 );
    strncpy( topicTemp, message->topic, topicTempLen );

    const char* firstElement = strtok( topicTemp, "/" );
    const char* hostName = strtok( NULL, "/" );
    const char* group = strtok( NULL, "/" );
    const char* cmd = strtok( NULL, "/" );

// if we recieve system-messages
    if( strncmp(message->topic,"$SYS/broker/clients/connected", 29) == 0 ){
        mqttService::ptr->connectedClients = atoi((char*)message->payload);
		cmd = "clientCount";
    }

// from here we MUST have a host and a topic
    if( hostName == NULL ) return;
    if( group == NULL ) return;
    if( cmd == NULL ) return;

// we only allow message to all or to our host
/*
    char* myHostName = coCore::ptr->hostInfo.nodename;
    if( strncmp(hostName,"all",3) != 0 && strncmp(hostName,myHostName,strlen(myHostName)) != 0 ){
        return;
    }
*/

// send to all plugins
    coCore::ptr->broadcast( mqttService::ptr, hostName, group, cmd, (const char*)message->payload );


}


bool mqttService::          onBroadcastReply( json_t* jsonAnswerArray ){

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

        const char* jsonPayloadChar = json_string_value(jsonPayload);
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
        memset( mqttService::ptr->lastPubTopic, 0, sizeof(mqttService::ptr->lastPubTopic) );
        memchr( mqttService::ptr->lastPubTopic, 0, sizeof(mqttService::ptr->lastPubTopic) );
        strncpy( this->lastPubTopic, json_string_value(jsonTopic), json_string_length(jsonTopic) );


    }


}


bool mqttService::          onBroadcastMessage(     const char*     msgHostName,
                                                    const char*     msgGroup,
                                                    const char*     msgCommand,
                                                    const char*     msgPayload,
                                                    json_t*         jsonAnswerObject ){

// vars
    std::string         fullTopic = "nodes/";
    int                 msgPayloadLen;


// we dont send messages which are deticated to our own
/*
    if( strncmp(msgHostName,coCore::ptr->hostInfo.nodename,strlen(coCore::ptr->hostInfo.nodename)) == 0 ){
        return true;
    }
*/

// build full topic
    fullTopic += msgHostName;
    fullTopic += "/";
    fullTopic += msgGroup;
    fullTopic += "/";
    fullTopic += msgCommand;

// infos about mqtt
    if( strncmp( msgGroup,"mqtt", 8 ) == 0 ){
        if( strncmp( msgCommand,"getinfos", 8 ) == 0 ){

			json_t* jsonMQTTInfos = json_object();

		// connected ?
			if( this->connected == true ) json_object_set_new( jsonMQTTInfos, "connected", json_integer(1) );
            else json_object_set_new( jsonMQTTInfos, "connected", json_integer(0) );

		// connected clients
			json_object_set_new( jsonMQTTInfos, "clients", json_integer(this->connectedClients) );


			json_object_set_new( jsonAnswerObject, "topic", json_string("infos") );
			json_object_set_new( jsonAnswerObject, "payload", jsonMQTTInfos );

            //mosquitto_lib_version()


            return true;
        }
    }


// dont send messages to localhost over mqtt
    if( strncmp(msgHostName,"localhost",9) == 0 ){
        return true;
    }

// dump jsonData
    msgPayloadLen = strlen( msgPayload );

// publish
    int messageID;
    mosquitto_publish(
        this->mosq,
        &messageID,
        fullTopic.c_str(),
        msgPayloadLen,
        msgPayload,
        0,
        false );

// save last publicated message
    memchr( this->lastPubTopic, 0, sizeof(this->lastPubTopic) );
    strncpy( this->lastPubTopic, fullTopic.c_str(), fullTopic.length() );


    return true;
}




#endif
