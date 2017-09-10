


#include "websocket.h"
#include "coCore.h"


websocket* websocket::ptr = NULL;

websocket::							websocket( int wsPort ) : coPlugin( "websocketClient", "", "" ) {

    this->ptr = this;

    etStringAllocLen( this->actualClientReply, 512 );

// create websocket
    struct lws_context_creation_info info;
    memset( &info, 0, sizeof(lws_context_creation_info) );

    info.server_string = "copilot-ws";
    info.port = wsPort;
    info.protocols = this->protocols;
    info.gid = -1;
    info.uid = -1;
    info.ka_time = 10;
    info.ka_probes = 3;
    info.ka_interval = 5;
    info.max_http_header_pool = 1;
    info.count_threads = 1;


    this->wsContext = lws_create_context(&info);
    if (this->wsContext == NULL) {
        lwsl_err("libwebsocket init failed\n");
        return;
    }

    pthread_create( &this->thread, NULL, &websocket::wsThread, this );
    pthread_detach( this->thread );

// register plugin
	coCore::ptr->plugins->append( this );

}
websocket::							~websocket(){

}


void* websocket::           		wsThread( void* data ){

    websocket* wsInstance = (websocket*)data;

    while(1){
        lws_service( wsInstance->wsContext, 50000 );
    }

    pthread_exit(NULL);
}

int websocket::             		wsCallbackHttp(     struct lws *wsi, enum lws_callback_reasons reason,
                                                void *user, void *in, size_t len ){
    return 0;
}


int websocket::             		wsCallbackCopilot(  struct lws *wsi, enum lws_callback_reasons reason,
                                                void *user, void *in, size_t len ){

    struct websocket::clientSessionData*    pss = (struct websocket::clientSessionData *)user;


    switch (reason) {



        case LWS_CALLBACK_ESTABLISHED:
            fprintf( stdout, "[%p] LWS_CALLBACK_ESTABLISHED\n", pss );
            fflush( stdout );

        // only one connection is allowed
            if( websocket::ptr->actualClientSession != NULL ){
                snprintf( etDebugTempMessage, etDebugTempMessageLen, "[%p] Somebody already connected, you can not do anything !" );
                etDebugMessage( etID_LEVEL_WARNING, etDebugTempMessage );
                break;
            }

            websocket::ptr->actualClientSession = pss;
            websocket::ptr->actualClientSession->wsi = wsi;

            break;

        case LWS_CALLBACK_RECEIVE:
            fprintf( stdout, "[%p] LWS_CALLBACK_RECEIVE", pss );
            fflush( stdout );

        // are we allowed ?
            if( websocket::ptr->actualClientSession != pss ){
                fprintf( stdout, "\n" );
                fflush( stdout );

                snprintf( etDebugTempMessage, etDebugTempMessageLen, "[%p] Somebody already connected, you can not do anything !" );
                etDebugMessage( etID_LEVEL_WARNING, etDebugTempMessage );

                break;
            }

            websocket::ptr->wsOnMessage( (const char*)in, len );
            break;

        case LWS_CALLBACK_CLOSED:

        // are we allowed ?
            if( websocket::ptr->actualClientSession != pss ){
                fprintf( stdout, "\n" );
                fflush( stdout );

                snprintf( etDebugTempMessage, etDebugTempMessageLen, "[%p] Somebody already connected, you can not do anything !" );
                etDebugMessage( etID_LEVEL_WARNING, etDebugTempMessage );

                break;
            }

            fprintf( stdout, "[%p] LWS_CALLBACK_CLOSED\n", pss );
            websocket::ptr->actualClientSession = NULL;
            fflush( stdout );
            break;

        default:
            break;

    }

    return 0;

}


void websocket::            		wsOnMessage( const char* message, int messageLen ){


    fprintf( stdout, "[%p]: Message: %s\n", __PRETTY_FUNCTION__, message );

// json
    json_error_t        jsonError;
    json_t*             jsonObject = json_loads( message, messageLen, &jsonError );
    if( jsonError.line != -1 ){
        fprintf( stdout, "[%s]: Error in parse json-message: %s\n", __PRETTY_FUNCTION__, jsonError.text );
        return;
    }

// we need a small message
	coMessage* tempMessage = new coMessage();
	tempMessage->fromJson( jsonObject );

    const char*			cmd = tempMessage->command();
	const char*			payload = tempMessage->payload();

// websocket only message
    if( strncmp((char*)cmd, "hostNameGet", 11) == 0 ){

	// vars
        json_t* jsonAnswerObject = json_object();

	// get the hostname
		coCore::ptr->hostNameGet( &payload, NULL );

	// set reply
		tempMessage->replyCommand( "hostName" );
		tempMessage->replyPayload( payload );

    // reply
		tempMessage->toJson( jsonAnswerObject, true );
        char* jsonDump = json_dumps( jsonAnswerObject, JSON_PRESERVE_ORDER | JSON_COMPACT );
        this->wsReply( jsonDump );
        free( jsonDump );
        json_decref(jsonAnswerObject);
        return;
    }

    if( strncmp((char*)cmd, "authMethodeGet", 11) == 0 ){

	// vars
        json_t* jsonAnswerObject = json_object();

	// set reply
		tempMessage->replyCommand( "authMethode" );

        if( coCore::ptr->config->authMethode() == false ){
            tempMessage->replyPayload( "none" );
            this->setAuth( true );
        } else {
            tempMessage->replyPayload( "password" );
        }

    // reply
		tempMessage->toJson( jsonAnswerObject, true );
        char* jsonDump = json_dumps( jsonAnswerObject, JSON_PRESERVE_ORDER | JSON_COMPACT );
        this->wsReply( jsonDump );

    // clean and return
        free( jsonDump );
        json_decref(jsonAnswerObject);
        return;
    }

// websocket must do auth
    if( strncmp( (char*)cmd, "login", 5 ) == 0 ){

        json_t* jsonCredentials = json_loads( payload, JSON_PRESERVE_ORDER, &jsonError );
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
            this->setAuth( false );
            //json_decref(jsonCredentials);
			goto checkAuth;
        }
        this->setAuth( true );

	// ckeanup
		json_decref(jsonCredentials);

	// vars
        json_t* jsonAnswerObject = json_object();

	// set reply
		tempMessage->replyCommand( "loginok" );
		tempMessage->replyPayload( "" );

    // reply
		tempMessage->toJson( jsonAnswerObject, true );
        char* jsonDump = json_dumps( jsonAnswerObject, JSON_PRESERVE_ORDER | JSON_COMPACT );
        this->wsReply( jsonDump );
        free( jsonDump );
        json_decref(jsonAnswerObject);
        return;
    }

checkAuth:
// authenticated ?
    if( this->isAuth() == false ){
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "[%s] %s", __PRETTY_FUNCTION__, "not authenticated, do nothing" );
        etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );
        return;
    }



// broadcast
    coCore::ptr->plugins->broadcast( this, tempMessage );

}


void websocket::            		wsReply( const char *message ){
    if( this->actualClientSession == NULL ) return;

	int 	messageLen = strlen(message);
	char 	messageBuf[LWS_PRE + messageLen];
	memset( messageBuf, 0, LWS_PRE + messageLen);

// clear and copy to the send-buffer
	memcpy( &messageBuf[LWS_PRE], message, messageLen );

    lws_write( this->actualClientSession->wsi, (unsigned char*)&messageBuf[LWS_PRE], messageLen, LWS_WRITE_TEXT );

}




bool websocket::            		isAuth(){
    if( this->actualClientSession == NULL ) return false;

    return this->actualClientSession->authenthicated;
}


void websocket::            		setAuth( bool authenticated ){
    if( this->actualClientSession == NULL ) return;

    this->actualClientSession->authenthicated = authenticated;
}





coPlugin::t_state websocket::		onBroadcastMessage( coMessage* message ){

// vars
    json_t*             jsonObject = NULL;
    const char*         jsonObjectChar = NULL;

// convert message to json
	jsonObject = json_object();
	message->toJson( jsonObject, false );
	jsonObjectChar = json_dumps( jsonObject, JSON_PRESERVE_ORDER | JSON_COMPACT );

// send it out
    this->wsReply( jsonObjectChar );

// cleanup
    free( (void*)jsonObjectChar );
    json_decref( jsonObject );

// return
    return coPlugin::NO_REPLY;
}


bool websocket:: 					onBroadcastReply( coMessage* message ){

// vars
    json_t*             jsonObject = NULL;
    const char*         jsonObjectChar = NULL;

// convert message to json
	jsonObject = json_object();
	message->toJson( jsonObject, true );
	jsonObjectChar = json_dumps( jsonObject, JSON_PRESERVE_ORDER | JSON_COMPACT );

// send it out
    this->wsReply( jsonObjectChar );

// cleanup
    free( (void*)jsonObjectChar );
    json_decref( jsonObject );

// return
    return coPlugin::NO_REPLY;
}

