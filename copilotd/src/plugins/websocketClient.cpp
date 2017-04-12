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

#ifndef websocketClient_C
#define websocketClient_C

#include "coCore.h"
#include "plugins/websocketClient.h"
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
    this->remoteSocket->close();
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

// we grab the hostname out of the topic
    hostName = strtok( (char*)topic, "/" );
    hostName = strtok( NULL, "/" );
    group = strtok( NULL, "/" );
    cmd = strtok( NULL, "/" );

// broadcast
    coCore::ptr->broadcast( this, hostName, group, cmd, jsonPayload );

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
    coCore::ptr->removePlugin( this );
    delete this;
}


bool websocketClient::      broadcastReply( json_t* jsonAnswerArray ){

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
        free( jsonDump );

    }

// cleanup
//    json_decref( jsonObject );

}












#endif
