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

#ifdef DISABLE_WEBSOCKET
#define websocketClient_C 1
#endif

#ifndef websocketClient_C
#define websocketClient_C

#include "coCore.h"
#include "plugins/qwebsocketClient.h"
#include "jansson.h"




websocketClient::           websocketClient( QWebSocket* remoteSocket )  : QObject(), coPlugin( "websocketClient" ){

// register this plugin
    coCore::ptr->registerPlugin( this, "", "" );

// connection infos
    this->remoteSocket = remoteSocket;
    this->remotePort = this->remoteSocket->peerPort();

// auth-infos
    this->authenticated = false;
    etStringAllocLen( this->username, 32 );

    qDebug() << QString( "%1: New Client %2 %3")
        .arg( this->remoteSocket->peerPort() )
        .arg( this->remoteSocket->peerAddress().toString() )
        .arg( this->remoteSocket->peerName() );

// connect events
    QObject::connect(   this->remoteSocket, &QWebSocket::textMessageReceived,
                        this, &websocketClient::onTextMessage );

    QObject::connect(   this->remoteSocket, &QWebSocket::disconnected,
                        this, &websocketClient::onDisconnected );


}

websocketClient::           ~websocketClient(){
    coCore::ptr->removePlugin( this );
    
    etStringFree( this->username );
}


void websocketClient::      onTextMessage( QString message ){

// message
    const char*         messageChar = message.toUtf8();
    int                 messageCharLen = strlen(messageChar);


    fprintf( stdout, "[%s][%i]: Message: %s\n", __PRETTY_FUNCTION__, this->remotePort, (const char*)message.toUtf8() );

// json
    json_error_t        jsonError;
    json_t*             jsonObject = json_loads( message.toUtf8(), messageCharLen, &jsonError );
    if( jsonError.line != -1 ){
        fprintf( stdout, "[%s][%i]: Error in parse json-message: %s\n", __PRETTY_FUNCTION__, this->remotePort, jsonError.text );
        return;
    }

    json_t*             jsonTopic = json_object_get( jsonObject, "topic" );
    json_t*             jsonPayload = json_object_get( jsonObject, "payload" );
    const char*         topic = json_string_value( jsonTopic );
    char*               hostName;
    char*               group;
    char*               cmd;


// websocket only message
    if( strncmp((char*)topic, "nodes/localhost/co/gethostname", 30) == 0 ){

        json_t* jsonAnswerObject = json_object();
        json_object_set_new( jsonAnswerObject, "topic", json_string("nodes/localhost/co/hostname") );
        json_object_set_new( jsonAnswerObject, "payload", json_string(coCore::ptr->hostInfo.nodename) );

    // reply
        char*   jsonDump = json_dumps( jsonAnswerObject, JSON_PRESERVE_ORDER | JSON_COMPACT );
        this->remoteSocket->sendTextMessage( jsonDump );
        this->remoteSocket->flush();
        free( jsonDump );
        json_decref(jsonAnswerObject);
        return;
    }


// we grab the hostname out of the topic
    hostName = strtok( (char*)topic, "/" );
    hostName = strtok( NULL, "/" );
    group = strtok( NULL, "/" );
    cmd = strtok( NULL, "/" );


// websocket must do auth
    if( strncmp( (char*)cmd, "login", 5 ) == 0 ){
        
        json_t* jsonCredentials = json_loads( json_string_value(jsonPayload), JSON_PRESERVE_ORDER, &jsonError );
        if( jsonCredentials == NULL ) return;

        const char* userName = json_string_value( json_object_get( jsonCredentials, "user" ) );
        const char* userPass = json_string_value( json_object_get( jsonCredentials, "password" ) );

    // check password
        if( coCore::ptr->passwordCheck(userName,userPass) == false ){
            this->authenticated = false;
            json_decref(jsonCredentials);
            this->remoteSocket->close();
            
            return;
        }
        this->authenticated = true;

    // create answer topic
        json_t* jsonAnswerObject = json_object();
        std::string fullTopic = "nodes/";
        fullTopic += hostName;
        fullTopic += "/";
        fullTopic += group;
        fullTopic += "/loginok";
        json_object_set_new( jsonAnswerObject, "topic", json_string(fullTopic.c_str()) );
        json_object_set_new( jsonAnswerObject, "payload", json_string("") );

    // reply
        char* jsonDump = json_dumps( jsonAnswerObject, JSON_PRESERVE_ORDER | JSON_COMPACT );
        this->remoteSocket->sendTextMessage( jsonDump );
        this->remoteSocket->flush();
        


    // clean
        free( jsonDump );
        json_decref(jsonAnswerObject);
        json_decref(jsonCredentials);
    }
    
// authenticated ?
    if( this->authenticated == false ){
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "[%i] %s", this->remotePort, "not authenticated, close connection" );
        etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
        this->remoteSocket->close();
        return;
    }



// broadcast
    coCore::ptr->broadcast( this, hostName, group, cmd, json_string_value(jsonPayload) );

}


void websocketClient::      onDisconnected(){

// debug
    const char* pluginNameChar = NULL;
    etStringCharGet( this->pluginName, pluginNameChar );
    snprintf( etDebugTempMessage, etDebugTempMessageLen, "Register Plugin: %s", pluginNameChar );
    etDebugMessage( etID_LEVEL_DETAIL, etDebugTempMessage );

    qDebug() << QString( "%1: By By Client %2 %3")
        .arg( this->remoteSocket->peerPort() )
        .arg( this->remoteSocket->peerAddress().toString() )
        .arg( this->remoteSocket->peerName() );

// remove the plugin
    delete this;
}


bool websocketClient::      onBroadcastReply( json_t* jsonAnswerArray ){

    char *dump = json_dumps( jsonAnswerArray, JSON_PRESERVE_ORDER | JSON_INDENT(4) );
    if( dump == NULL ){
        etDebugMessage( etID_LEVEL_ERR, "Json error" );
        return false;
    }
    etDebugMessage( etID_LEVEL_DETAIL, dump );
    free( dump );


// we send the data back
    json_t*     jsonAnswer = NULL;
    int jsonArrayLen = json_array_size(jsonAnswerArray);
    int jsonArrayIndex = 0;
    for( jsonArrayIndex = 0; jsonArrayIndex < jsonArrayLen; jsonArrayIndex++ ){

        jsonAnswer = json_array_get( jsonAnswerArray, jsonArrayIndex );

        char*   jsonDump = json_dumps( jsonAnswer, JSON_PRESERVE_ORDER | JSON_COMPACT );
        
        
        this->remoteSocket->sendTextMessage( jsonDump );
        this->remoteSocket->flush();
        free( jsonDump );

    }

// cleanup
//    json_decref( jsonObject );

}


bool websocketClient::      onBroadcastMessage(     const char*     msgHostName, 
                                                    const char*     msgGroup, 
                                                    const char*     msgCommand, 
                                                    const char*     msgPayload, 
                                                    json_t*         jsonAnswerObject ){

// vars
    json_t*             jsonObject = json_object();
    const char*         jsonObjectChar = NULL;
    std::string         fullTopic = "nodes/";
    int                 msgPayloadLen;
    
// build full topic
    fullTopic += msgHostName;
    fullTopic += "/";
    fullTopic += msgGroup;
    fullTopic += "/";
    fullTopic += msgCommand;

// setup jsonObject
    json_object_set_new( jsonObject, "topic", json_string(fullTopic.c_str()) );
    json_object_set_new( jsonObject, "payload", json_string(msgPayload) );
    jsonObjectChar = json_dumps( jsonObject, JSON_PRESERVE_ORDER | JSON_COMPACT );

// send it out
    this->remoteSocket->sendTextMessage( jsonObjectChar );
    this->remoteSocket->flush();

// cleanup
    free( (void*)jsonObjectChar );
    json_decref( jsonObject );


    return true;
}









#endif
