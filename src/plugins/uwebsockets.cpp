




#include "uwebsockets.h"
#include "coCore.h"
#include <thread>

#include "pubsub.h"

uwebsocket* uwebsocket::inst = NULL;

uwebsocket::						uwebsocket( int wsPort )  {
    
    this->wsServer = NULL;
	this->port = wsPort;
	this->inst = this;


    pthread_create( &this->thread, NULL, &uwebsocket::wsThread, this );
    char threadName[10] = { '\0' };
    snprintf( threadName, 10, "websocket" );
    pthread_setname_np( this->thread, threadName );
    pthread_detach( this->thread );
	
	
// subscribe
	psBus::inst->subscribe( NULL, NULL, this, NULL, uwebsocket::onSubscriberJsonMessage );


}

uwebsocket::						~uwebsocket(){
	
}


void* uwebsocket::           		wsThread( void* data ){

    uWS::Hub h;
	
	uwebsocket*		instance = (uwebsocket*)data;

    h.onMessage([&h,&instance](uWS::WebSocket<uWS::SERVER> *ws, char *message, size_t length, uWS::OpCode opCode) {
        //ws->send(message, length, opCode);
		instance->onMessage( ws, message, length, opCode );
    });
	
	h.listen(instance->port);
	h.run();
	
	return NULL;
}


void uwebsocket::					onMessage( uWS::WebSocket<uWS::SERVER> *server, char *message, size_t messageSize, uWS::OpCode opCode ){
    

// switch
	char oldChar = message[messageSize];
	message[messageSize] = '\0';
	
	fprintf( stdout, "[%p]: Message: %s\n", __PRETTY_FUNCTION__, message );

// json
    json_error_t        jsonError;
    json_t*             jsonObject = json_loads( message, messageSize, &jsonError );
	json_t*				jsonObjectValue;
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
    const char*			msgSource = myNodeName;
	const char*			msgID = NULL; jsonObjectValue = json_object_get( jsonObject, "id" ); if( jsonObjectValue != NULL ){ msgID = json_string_value(jsonObjectValue); }
    const char*			msgTarget = NULL; jsonObjectValue = json_object_get( jsonObject, "t" ); if( jsonObjectValue != NULL ){ msgTarget = json_string_value(jsonObjectValue); }
    const char*			msgGroup = NULL; jsonObjectValue = json_object_get( jsonObject, "g" ); if( jsonObjectValue != NULL ){ msgGroup = json_string_value(jsonObjectValue); }
    const char*			msgCommad = NULL; jsonObjectValue = json_object_get( jsonObject, "c" ); if( jsonObjectValue != NULL ){ msgCommad = json_string_value(jsonObjectValue); }
	const char*			msgPayload = NULL; jsonObjectValue = json_object_get( jsonObject, "v" ); if( jsonObjectValue != NULL ){ msgPayload = json_string_value(jsonObjectValue); }
	
	

// websocket only message
    if( strncmp((char*)msgCommad, "nodeNameGet", 11) == 0 ){
		
	// get a new json
		json_t*		newJsonAnswer = NULL;
		psBus::toJson( &newJsonAnswer, msgID, msgTarget, msgSource, msgGroup, "nodeName", coCore::ptr->nodeName() );
		uwebsocket::onSubscriberJsonMessage( newJsonAnswer, this );
		json_decref( newJsonAnswer );
		
        return;
    }

    if( strncmp((char*)msgCommad, "authMethodeGet", 11) == 0 ){

		json_t*		newJsonAnswer = NULL;
		
        if( coCore::ptr->config->authMethode() == false ){
			psBus::toJson( &newJsonAnswer, msgID, msgTarget, msgSource, msgGroup, "authMethode", "none" );
        } else {
			psBus::toJson( &newJsonAnswer, msgID, msgTarget, msgSource, msgGroup, "authMethode", "password" );
        }
		
		uwebsocket::onSubscriberJsonMessage( newJsonAnswer, this );
		json_decref( newJsonAnswer );

        return;
    }

// websocket must do auth
    if( strncmp( (char*)msgCommad, "login", 5 ) == 0 ){

        json_t* jsonCredentials = json_loads( msgPayload, JSON_PRESERVE_ORDER, &jsonError );
        if( jsonCredentials == NULL ) return;
		json_t*			jsonString = NULL;
		const char* 	userName = NULL;
		const char* 	userPass = NULL;

		jsonString = json_object_get( jsonCredentials, "user" );
		if( jsonString != NULL ) userName = json_string_value( jsonString );

		jsonString = json_object_get( jsonCredentials, "password" );
		if( jsonString != NULL ) userPass = json_string_value( jsonString );

    // check password
        if( coCore::ptr->passwordCheck(userName,userPass) == false ){
            //this->setAuth( false );
            //json_decref(jsonCredentials);
			goto checkAuth;
        }
        //this->setAuth( true );

	// ckeanup
		json_decref(jsonCredentials);


		
		psBus::inst->publish( msgID, msgTarget, msgSource, msgGroup, "loginok", "" );
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
	psBus::inst->publish( jsonObject );
}




int uwebsocket::					onSubscriberMessage( const char* id, const char* nodeSource, const char* nodeTarget, const char* group, const char* command, const char* payload, void* userdata ){
	
	
	
	return 0;
}


int uwebsocket::					onSubscriberJsonMessage( json_t* jsonObject, void* userdata ){
// vars
	uwebsocket*			uwebsocketInstance = (uwebsocket*)userdata;
    json_t*             jsonTempObject = NULL;
    const char*         jsonTempObjectChar = NULL;
    
// no webserver aviable
    if( uwebsocketInstance->wsServer == NULL ) return -1;
	
	jsonTempObjectChar = json_dumps( jsonObject, JSON_PRESERVE_ORDER | JSON_COMPACT );
	uwebsocketInstance->wsServer->send( jsonTempObjectChar, strlen(jsonTempObjectChar), uWS::TEXT );
    free( (void*)jsonTempObjectChar );

	return 0;
}



