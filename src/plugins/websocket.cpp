


#include "websocket.h"
#include "coCore.h"


websocket* websocket::ptr = NULL;

websocket::websocket( int wsPort ) : coPlugin( "websocketClient" ) {

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

// register this plugin
    coCore::ptr->registerPlugin( this, "", "" );

}
websocket::~websocket(){

}


void* websocket::           wsThread( void* data ){

    websocket* wsInstance = (websocket*)data;

    while(1){
        lws_service( wsInstance->wsContext, 50000 );
    }

    pthread_exit(NULL);
}

int websocket::             wsCallbackHttp(     struct lws *wsi, enum lws_callback_reasons reason,
                                                void *user, void *in, size_t len ){
    return 0;
}


int websocket::             wsCallbackCopilot(  struct lws *wsi, enum lws_callback_reasons reason,
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


void websocket::            wsOnMessage( const char* message, int messageLen ){


    fprintf( stdout, "[%p]: Message: %s\n", __PRETTY_FUNCTION__, message );

// json
    json_error_t        jsonError;
    json_t*             jsonObject = json_loads( message, messageLen, &jsonError );
    if( jsonError.line != -1 ){
        fprintf( stdout, "[%s]: Error in parse json-message: %s\n", __PRETTY_FUNCTION__, jsonError.text );
        return;
    }

    json_t*             jsonTopic = json_object_get( jsonObject, "topic" );
    json_t*             jsonPayload = json_object_get( jsonObject, "payload" );
    const char*         topic = json_string_value( jsonTopic );
    char*               hostName;
    char*               group;
    char*               cmd;


// websocket only message
    if( strncmp((char*)topic, "nodes/localhost/co/gethostname", messageLen) == 0 ){

        json_t* jsonAnswerObject = json_object();
        json_object_set_new( jsonAnswerObject, "topic", json_string("nodes/localhost/co/hostname") );
        json_object_set_new( jsonAnswerObject, "payload", json_string(coCore::ptr->hostInfo.nodename) );

    // reply
        char*   jsonDump = json_dumps( jsonAnswerObject, JSON_PRESERVE_ORDER | JSON_COMPACT );
        this->wsReply( jsonDump );
        free( jsonDump );
        json_decref(jsonAnswerObject);
        return;
    }


// we grab the hostname out of the topic
    hostName = strtok( (char*)topic, "/" );
    hostName = strtok( NULL, "/" );
    group = strtok( NULL, "/" );
    cmd = strtok( NULL, "/" );

// check
    if( hostName == NULL ) return;
    if( group == NULL ) return;
    if( cmd == NULL ) return;

// websocket must do auth
    if( strncmp( (char*)cmd, "login", 5 ) == 0 ){

        json_t* jsonCredentials = json_loads( json_string_value(jsonPayload), JSON_PRESERVE_ORDER, &jsonError );
        if( jsonCredentials == NULL ) return;

        const char* userName = json_string_value( json_object_get( jsonCredentials, "user" ) );
        const char* userPass = json_string_value( json_object_get( jsonCredentials, "password" ) );

    // check password
        if( coCore::ptr->passwordCheck(userName,userPass) == false ){
            this->setAuth( false );
            json_decref(jsonCredentials);

            return;
        }
        this->setAuth( true );

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
        this->wsReply( jsonDump );

    // clean
        free( jsonDump );
        json_decref(jsonAnswerObject);
        json_decref(jsonCredentials);
    }

// authenticated ?
    if( this->isAuth() == false ){
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "[%s] %s", __PRETTY_FUNCTION__, "not authenticated, close connection" );
        etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
        return;
    }



// broadcast
    coCore::ptr->broadcast( this, hostName, group, cmd, json_string_value(jsonPayload) );

}


void websocket::            wsReply( const char *message ){
    if( this->actualClientSession == NULL ) return;

	int 	messageLen = strlen(message);
	char 	messageBuf[LWS_PRE + messageLen];
	memset( messageBuf, 0, LWS_PRE + messageLen);

// clear and copy to the send-buffer
	memcpy( &messageBuf[LWS_PRE], message, messageLen );

    lws_write( this->actualClientSession->wsi, (unsigned char*)&messageBuf[LWS_PRE], messageLen, LWS_WRITE_TEXT );

}




bool websocket::            isAuth(){
    if( this->actualClientSession == NULL ) return false;

    return this->actualClientSession->authenthicated;
}


void websocket::            setAuth( bool authenticated ){
    if( this->actualClientSession == NULL ) return;

    this->actualClientSession->authenthicated = authenticated;
}





bool websocket::			onBroadcastMessage( coMessage* message ){

// vars
	const char*			msgHostName = message->hostName();
	const char*			msgGroup = message->group();
	const char*			msgCommand = message->command();
	const char*			msgPayload = message->payload();

    json_t*             jsonObject = NULL;
    const char*         jsonObjectChar = NULL;
    std::string         fullTopic = "";
    int                 msgPayloadLen;

// build full topic
	fullTopic = "nodes/";
    fullTopic += msgHostName;
    fullTopic += "/";
    fullTopic += msgGroup;
    fullTopic += "/";
    fullTopic += msgCommand;

// setup jsonObject
	jsonObject = json_object();
    json_object_set_new( jsonObject, "topic", json_string(fullTopic.c_str()) );
    json_object_set_new( jsonObject, "payload", json_string(msgPayload) );
    jsonObjectChar = json_dumps( jsonObject, JSON_PRESERVE_ORDER | JSON_COMPACT );

/*
	char jsonDump[4096]; memset( jsonDump, 0, 4096 );
	json_dumpb( jsonObject, jsonDump, 4096, JSON_PRESERVE_ORDER | JSON_COMPACT );
*/
// send it out
    this->wsReply( jsonObjectChar );

// cleanup
    free( (void*)jsonObjectChar );
    json_decref( jsonObject );


    return true;
}


bool websocket:: 			onBroadcastReply( coMessage* message ){

// vars
	const char*			msgReplyPayload = message->replyPayload();

// there was no reply-data
	if( message->replyExists() == false ){
		return true;
	}


    json_t*             jsonObject = NULL;
    char*         		jsonObjectChar = NULL;
    const char*         fullTopic = "";
    int                 msgPayloadLen;

// build full topic
	fullTopic = message->replyComandFull();

// setup jsonObject
	jsonObject = json_object();
    json_object_set_new( jsonObject, "topic", json_string(fullTopic) );
    json_object_set_new( jsonObject, "payload", json_string(msgReplyPayload) );
    jsonObjectChar = json_dumps( jsonObject, JSON_PRESERVE_ORDER | JSON_COMPACT );


// send it out
    this->wsReply( jsonObjectChar );

// cleanup
    free( (void*)jsonObjectChar );
    json_decref( jsonObject );


    return true;
}

