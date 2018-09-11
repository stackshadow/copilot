/*
Copyright (C) 2018 by Martin Langlotz aka stackshadow

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




#include "uwebsockets.h"
#include "coCore.h"
#include <thread>

#include "pubsub.h"



#include "core/plugin.h"

#ifdef __cplusplus
extern "C" {
#endif





extern void                     websocket_pluginRun( pluginData_t* pluginData ){
    uwebsocket* service = (uwebsocket*)pluginUserData( pluginData );
    if( service == NULL ) return;

    service->init();
    service->serve();
}


extern int                      websocket_pluginParseCmdLine( pluginData_t* pluginData, const char* option, const char* value ){



    uwebsocket* service = (uwebsocket*)pluginUserData( pluginData );
    if( service != NULL ){
        service->parseOpt( option, value );
        return 0;
    }

    if( coCore::strIsExact( option, "websocket", 9 ) == true ){
        uwebsocket* service = new uwebsocket( 3333 );
        pluginSetUserData( pluginData, service );
        pluginRegisterCmdRun( pluginData, websocket_pluginRun );
    }


    return 0;
}


extern void                     pluginPrepare( pluginData_t* pluginData ){


    coCore::addOption( "websocket", "w", "Enable websocketport ( This MUST be the first option before other websocket options )", no_argument );
    coCore::addOption( "wsport", "p", "<port> Set websocket port", no_argument );


    pluginSetInfos( pluginData, "websocket\0", "Provide an websocket port for web-interfaces\0" );
    pluginSetConfigSection( pluginData, "websocket\0" );
    pluginSetUserData( pluginData, NULL );

    pluginRegisterCmdParse( pluginData, websocket_pluginParseCmdLine );
}



#ifdef __cplusplus
}
#endif






uwebsocket::                        uwebsocket( int wsPort )  {

    this->wsServer = NULL;
    this->port = wsPort;

}

uwebsocket::                        ~uwebsocket(){

}


bool uwebsocket::                   parseOpt( const char* option, const char* value ){


    if( coCore::strIsExact( option, "wsport", 6 ) == true ){
        this->port = atoi( value );
        return true;
    }


    return false;
}


void uwebsocket::                   init(){
    coCore::addOption( "wsport", "", "<port> Set websocket port", required_argument );
}





void uwebsocket::                   serve(){

// create thread
    pthread_create( &this->thread, NULL, &uwebsocket::wsThread, this );
    char threadName[10] = { '\0' };
    snprintf( threadName, 10, "websocket" );
    pthread_setname_np( this->thread, threadName );
    pthread_detach( this->thread );

// debug
    snprintf( etDebugTempMessage, etDebugTempMessageLen, "[%p]: uwebsocket", this );
    etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );

// subscribe
    psBus::inst->subscribe( this, NULL, NULL, NULL, NULL, uwebsocket::onSubscriberJsonMessage );
}


void* uwebsocket::                  wsThread( void* data ){

    uWS::Hub h;

    uwebsocket*     instance = (uwebsocket*)data;

    h.onMessage([&h,&instance](uWS::WebSocket<uWS::SERVER> *ws, char *message, size_t length, uWS::OpCode opCode) {
        //ws->send(message, length, opCode);
        instance->onMessage( ws, message, length, opCode );
    });

    h.listen( "127.0.0.1", instance->port);

// debug
    snprintf( etDebugTempMessage, etDebugTempMessageLen, "[%p]: uwebsocket listen on port %i", instance, instance->port );
    etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );


    h.run();

    return NULL;
}


void uwebsocket::                   onMessage( uWS::WebSocket<uWS::SERVER> *server, char *message, size_t messageSize, uWS::OpCode opCode ){


// switch
    char oldChar = message[messageSize];
    message[messageSize] = '\0';

    fprintf( stdout, "[%p]: Message: %s\n", __PRETTY_FUNCTION__, message );

// json
    json_error_t        jsonError;
    json_t*             jsonObject = json_loads( message, messageSize, &jsonError );
    json_t*             jsonObjectValue;
    if( jsonError.line != -1 ){
        fprintf( stdout, "[%s]: Error in parse json-message: %s\n", __PRETTY_FUNCTION__, jsonError.text );
        return;
    }

// switch back
    message[messageSize] = oldChar;

// set websocket
    this->wsServer = server;


// message vars
    const char*         myNodeName = coCore::ptr->nodeName();
    const char*         msgSource = myNodeName;
    const char*         msgID = NULL; jsonObjectValue = json_object_get( jsonObject, "id" ); if( jsonObjectValue != NULL ){ msgID = json_string_value(jsonObjectValue); }
    const char*         msgTarget = NULL; jsonObjectValue = json_object_get( jsonObject, "t" ); if( jsonObjectValue != NULL ){ msgTarget = json_string_value(jsonObjectValue); }
    const char*         msgGroup = NULL; jsonObjectValue = json_object_get( jsonObject, "g" ); if( jsonObjectValue != NULL ){ msgGroup = json_string_value(jsonObjectValue); }
    const char*         msgCommad = NULL; jsonObjectValue = json_object_get( jsonObject, "c" ); if( jsonObjectValue != NULL ){ msgCommad = json_string_value(jsonObjectValue); }
    const char*         msgPayload = NULL; jsonObjectValue = json_object_get( jsonObject, "v" ); if( jsonObjectValue != NULL ){ msgPayload = json_string_value(jsonObjectValue); }



// websocket only message
    if( strncmp((char*)msgCommad, "nodeNameGet", 11) == 0 ){

    // get a new json
        json_t*		newJsonAnswer = NULL;
        psBus::toJson( &newJsonAnswer, msgID, msgTarget, msgSource, msgGroup, "nodeName", coCore::ptr->nodeName() );
        uwebsocket::onSubscriberJsonMessage( this, newJsonAnswer, NULL );
        json_decref( newJsonAnswer );

        return;
    }

    if( strncmp((char*)msgCommad, "authMethodeGet", 11) == 0 ){

        json_t*     newJsonAnswer = NULL;

        if( coConfig::ptr->authMethode() == false ){
            psBus::toJson( &newJsonAnswer, msgID, msgTarget, msgSource, msgGroup, "authMethode", "none" );
        } else {
            psBus::toJson( &newJsonAnswer, msgID, msgTarget, msgSource, msgGroup, "authMethode", "password" );
        }

        uwebsocket::onSubscriberJsonMessage( this, newJsonAnswer, NULL );
        json_decref( newJsonAnswer );

        return;
    }

// websocket must do auth
    if( strncmp( (char*)msgCommad, "login", 5 ) == 0 ){

        json_t* jsonCredentials = json_loads( msgPayload, JSON_PRESERVE_ORDER, &jsonError );
        if( jsonCredentials == NULL ) return;
        json_t*         jsonString = NULL;
        const char*     userName = NULL;
        const char*     userPass = NULL;

        jsonString = json_object_get( jsonCredentials, "user" );
        if( jsonString != NULL ) userName = json_string_value( jsonString );

        jsonString = json_object_get( jsonCredentials, "password" );
        if( jsonString != NULL ) userPass = json_string_value( jsonString );

    // check password
        //if( coCore::ptr->passwordCheck(userName,userPass) == false ){
            //this->setAuth( false );
            //json_decref(jsonCredentials);
            //goto checkAuth;
        //}
        //this->setAuth( true );

    // ckeanup
        json_decref(jsonCredentials);



        psBus::inst->publish( this, msgID, msgTarget, msgSource, msgGroup, "loginok", "" );
        return;
    }

checkAuth:
// authenticated ?
/*
    if( this->isAuth() == false ){
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "[%s] %s", __PRETTY_FUNCTION__, "not authenticated, do nothing" );
        etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );
        return;
    }
*/



    json_object_set_new( jsonObject, "s", json_string("wsclient") );
    psBus::publishJson( this, jsonObject );
}




int uwebsocket::                    onSubscriberMessage( void* objectSource, const char* id, const char* nodeSource, const char* nodeTarget, const char* group, const char* command, const char* payload, void* userdata ){



    return 0;
}


int uwebsocket::                    onSubscriberJsonMessage( void* objectSource, json_t* jsonObject, void* userdata ){
// vars
    uwebsocket*         uwebsocketInstance = (uwebsocket*)objectSource;
    json_t*             jsonTempObject = NULL;
    const char*         jsonTempObjectChar = NULL;

// no webserver aviable
    if( uwebsocketInstance->wsServer == NULL ) return -1;

// vars
    const char*     msgSource = NULL;
    const char*     msgTarget = NULL;

// get source/target from json
    psBus::fromJson( jsonObject, NULL, &msgSource, &msgTarget, NULL, NULL, NULL );

// if there is a message for myHost we dont need to send it out to the world
    if( coCore::strIsExact( msgSource, "wsclient", 8 ) == true ){
        return 0;
    }

    jsonTempObjectChar = json_dumps( jsonObject, JSON_PRESERVE_ORDER | JSON_COMPACT );
    uwebsocketInstance->wsServer->send( jsonTempObjectChar, strlen(jsonTempObjectChar), uWS::TEXT );
    free( (void*)jsonTempObjectChar );

    return 0;
}



