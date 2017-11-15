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

#ifndef sslService_C
#define sslService_C

#include <sys/socket.h>
#include <netdb.h>

#include "plugins/sslService.h"
#include "coCore.h"


#include "plugins/sslSession.h"

#include "uuid.h"
#include "limits.h"     // for UINT_MAX


sslService::               			sslService() : coPlugin( "sslService", coCore::ptr->hostNameGet(), "cocom" ){


    sslSession::globalInit( coCore::ptr->hostNameGet() );

// register plugin
	coCore::ptr->plugins->append( this );
}


sslService::               			~sslService(){

}




coPlugin::t_state sslService::		onBroadcastMessage( coMessage* message ){

// vars
    const char*                 msgSource = message->nodeNameSource();
    const char*		            msgGroup = message->group();
	const char*		            msgCommand = message->command();
	const char*		            msgPayload = message->payload();
	json_t*			            jsonPayload = NULL;
	json_t*                     jsonTempValue;
    const char*                 jsonTempString = NULL;
	const char*                 hostName = NULL;
	int                         hostPort = 0;
    int                         serverEnabled = 0;
	json_error_t	            jsonError;
    coCoreConfig::nodeType      type = coCoreConfig::UNKNOWN;




	if( strncmp(msgCommand,"serverConfigGet",15) == 0 ){
        serverConfigGet:

    // lock
        coCore::ptr->config->nodesIterate();

    // build the respond-object
        jsonPayload = json_object();

    // read infos
        if( coCore::ptr->config->nodeSelectByHostName(coCore::ptr->hostNameGet()) == true ){
            coCore::ptr->config->nodeInfo( NULL, &type );
            coCore::ptr->config->nodeConnInfo( &hostName, &hostPort );
            json_object_set_new( jsonPayload, "enabled", json_integer(1) );
            json_object_set_new( jsonPayload, "host", json_string(hostName) );
            json_object_set_new( jsonPayload, "port", json_integer(hostPort) );
        } else {
            coCore::ptr->config->nodeAppend( coCore::ptr->hostNameGet() );
            json_object_set_new( jsonPayload, "enabled", json_integer(0) );
            json_object_set_new( jsonPayload, "host", json_string(coCore::ptr->hostNameGet()) );
            json_object_set_new( jsonPayload, "port", json_integer(4567) );
            coCore::ptr->config->save();
        }

    // unlock
        coCore::ptr->config->nodesIterateFinish();


    // set the message
        msgPayload = json_dumps( jsonPayload, JSON_PRESERVE_ORDER | JSON_INDENT(4) );

    // message send back so source
        coCore::ptr->plugins->messageAdd( this,
        coCore::ptr->hostNameGet(), msgSource,
        msgGroup, "serverConfig", msgPayload );


    // cleanup and return
        free((void*)msgPayload);
        json_decref(jsonPayload);
        return coPlugin::REPLY;
    }


	if( strncmp(msgCommand,"serverSave",10) == 0 ){

	// parse json
		jsonPayload = json_loads( msgPayload, JSON_PRESERVE_ORDER, &jsonError );
		if( jsonPayload == NULL || jsonError.line > -1 ){
			return coPlugin::NO_REPLY;
		}

    // get host / port
		json_t* jsonHostName = json_object_get( jsonPayload, "host" );
		json_t* jsonHostPort = json_object_get( jsonPayload, "port" );
        json_t* jsonHostEnabled = json_object_get( jsonPayload, "enabled" );

        hostName = json_string_value( jsonHostName );
		jsonTempString = json_string_value( jsonHostPort );
        hostPort = atoi( jsonTempString );
		jsonTempString = json_string_value( jsonHostEnabled );
        serverEnabled = atoi( jsonTempString );

    // lock
        coCore::ptr->config->nodesIterate();

    // read infos
        coCore::ptr->config->nodeSelectByHostName(coCore::ptr->hostNameGet());
        if( serverEnabled == 1 ){
            type = coCoreConfig::SERVER;
        } else {
            type = coCoreConfig::UNKNOWN;
        }
        coCore::ptr->config->nodeInfo( NULL, &type, true );
        coCore::ptr->config->nodeConnInfo( &hostName, &hostPort, true );
        coCore::ptr->config->save();

    // unlock
        coCore::ptr->config->nodesIterateFinish();

	// cleanup
		json_decref(jsonPayload);
        goto serverConfigGet;
		return coPlugin::NO_REPLY;
	}


    if( strncmp(msgCommand,"requestKeysGet",14) == 0 ){
        requestKeysGet:
    // vars
        json_t*         jsonKeys = json_object();

    // get the requested keys as json
        sslService::reqKeysGet( jsonKeys );

    // set the message
        msgPayload = json_dumps( jsonKeys, JSON_PRESERVE_ORDER | JSON_INDENT(4) );

    // add the message to list
        coCore::ptr->plugins->messageAdd( this,
        coCore::ptr->hostNameGet(), msgSource,
        msgGroup, "requestKeys", msgPayload );


    // cleanup and return
        free((void*)msgPayload);
        json_decref(jsonKeys);
        return coPlugin::REPLY;
    }


	if( strncmp(msgCommand,"requestKeyAccept",16) == 0 ){

	// parse json
		sslService::reqKeyAccept( msgPayload );

        goto requestKeysGet;
        return coPlugin::NO_REPLY;
    }


	if( strncmp(msgCommand,"requestKeyRemove",16) == 0 ){

	// parse json
		sslService::reqKeyRemove( msgPayload );

        goto requestKeysGet;
        return coPlugin::NO_REPLY;
    }


    if( strncmp(msgCommand,"acceptedKeysGet",15) == 0 ){
        acceptedKeysGet:
    // vars
        json_t*         jsonKeys = json_object();

    // get the requested keys as json
        sslService::acceptedKeysGet( jsonKeys );

    // set the message
        msgPayload = json_dumps( jsonKeys, JSON_PRESERVE_ORDER | JSON_INDENT(4) );

    // add the message to list
        coCore::ptr->plugins->messageAdd( this,
        coCore::ptr->hostNameGet(), msgSource,
        msgGroup, "acceptedKeys", msgPayload );

    // cleanup and return
        free((void*)msgPayload);
        json_decref(jsonKeys);
        return coPlugin::REPLY;
    }


	if( strncmp(msgCommand,"acceptedKeyRemove",17) == 0 ){

	// parse json
		sslService::acceptedKeyRemove( msgPayload );

        goto acceptedKeysGet;
        return coPlugin::NO_REPLY;
    }


    if( strncmp(msgCommand,"connectionStateGet",18) == 0 ){




    }


    return coPlugin::NO_REPLY;
}


bool sslService:: 					onSetup(){

// check if we have an server connection

// get the server infos
	bool 					serverFound = false;
	coCoreConfig::nodeType 	serverType;
	const char* 			serverHost;
	int 					serverPort;
	char 					answer[512] = { '0' };
	int						answerLen = 0;
	const char* 			pAnswer = (const char*)&answer;

	fprintf( stdout, "Copilot Node-Connection-Setup \n" );

ask:
	fprintf( stdout, "1.) Accept Connection from another node ( aka training-mode ) \n" );
	fprintf( stdout, "2.) Connect to another node \n" );
	fprintf( stdout, "What would you like to do ? (1/2) \n" );
	fflush( stdout );
	read( STDIN_FILENO, answer, 512 );
	fprintf( stdout, "\n" );

	if( answer[0] != '1' && answer[0] != '2' ) goto ask;

	if( answer[0] == '1' ){

		coCore::ptr->config->nodesIterate();

	// default server / port
		answer[0] = ':'; answer[1] = ':'; answer[2] = '\0';
		serverType = coCoreConfig::SERVER;
		serverPort = 4567;

	// read hostname
		fprintf( stdout, "Enter host/IP to bind. ( user :: for IPv6 and IPv4 ) " );
		fflush( stdout );
		read( STDIN_FILENO, answer, 512 );
		fprintf( stdout, "\n" );
	// remove 0
		answerLen = strlen(answer);
		if( answerLen > 0 ) answerLen--;
		answer[answerLen] = '\0';

	// save
		coCore::ptr->config->nodeSelect(coCore::ptr->hostNameGet());
		coCore::ptr->config->nodeInfo( NULL, &serverType, true );
		coCore::ptr->config->nodeConnInfo( (const char**)&pAnswer, &serverPort, true );
		coCore::ptr->config->nodesIterateFinish();

	// save it
		coCore::ptr->config->save();

	// serve connection
		this->serve();

	}


}


bool sslService:: 					onExecute(){
	if( coCore::setupMode == true ) return true;

	//this->serve();
	//this->connectAll();
}


// configuration
int sslService::                    maxConnection( int* setConnections ){
    if( setConnections != NULL ){
        this->maxConnections = *setConnections;
    }

    return this->maxConnections;
}





unsigned int sslService::           reqKeysCount(){

// vars
    DIR*			    keyDir = NULL;
    dirent*			    keyDirEntry = NULL;
    unsigned int        keyDirCounter = 0;

// open directory
    keyDir = opendir( sslAcceptedKeyPath );
    if( keyDir == NULL ){
        etDebugMessage( etID_LEVEL_ERR, "Could not open key-directory" );
        return UINT_MAX;
    }

// read every key
    keyDirEntry = readdir( keyDir );
    while( keyDirEntry != NULL ){
    // ignore "." and ".."
        if( keyDirEntry->d_name[0] != '.' ){
            keyDirCounter++;
        }
        keyDirEntry = readdir( keyDir );
    }
    closedir(keyDir);


    return keyDirCounter;
}


bool sslService::					reqKeysGet( json_t* jsonObject ){
    if( jsonObject == NULL ) return false;

// vars
    DIR*			keyDir = NULL;
    dirent*			keyDirEntry = NULL;
    json_t*         jsonKey = NULL;
    const char*     msgPayload = NULL;

// open directory
    keyDir = opendir( sslKeyReqPath );
    if( keyDir == NULL ){
        etDebugMessage( etID_LEVEL_ERR, "Could not open key-directory" );
        return coPlugin::NO_REPLY;
    }

// read every key
    keyDirEntry = readdir( keyDir );
    while( keyDirEntry != NULL ){

    // ignore "." and ".."
        if( keyDirEntry->d_name[0] == '.' ) goto nextKey;

    //
        jsonKey = json_object();
        json_object_set_new( jsonObject, keyDirEntry->d_name, jsonKey );

    nextKey:
        keyDirEntry = readdir( keyDir );
    }
    closedir(keyDir);

    return true;
}


bool sslService::					reqKeyRemove( const char* fingerprint ){

// we build the full path
    etString* fullKeyPath; const char* keyPath;
    etStringAllocLen( fullKeyPath, 128 );
    etStringCharSet( fullKeyPath, sslKeyReqPath, -1 );
    etStringCharAdd( fullKeyPath, fingerprint );
    etStringCharGet( fullKeyPath, keyPath );

// remove key
    snprintf( etDebugTempMessage, etDebugTempMessageLen, "Remove '%s'", keyPath );
    etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );
    unlink( keyPath );

// clean
    etStringFree( fullKeyPath );
    return true;
}


bool sslService::					reqKeyAccept( const char* fingerprint ){

// vars
    etString*       sourceKeyPath = NULL;
    etString*       targetKeyPath = NULL;
    const char*     sourcePath = NULL;
    const char*     targetPath = NULL;

// source
    etStringAllocLen( sourceKeyPath, 128 );
    etStringCharSet( sourceKeyPath, sslKeyReqPath, -1 );
    etStringCharAdd( sourceKeyPath, fingerprint );
    etStringCharGet( sourceKeyPath, sourcePath );

// target
    etStringAllocLen( targetKeyPath, 128 );
    etStringCharSet( targetKeyPath, sslAcceptedKeyPath, -1 );
    etStringCharAdd( targetKeyPath, fingerprint );
    etStringCharGet( targetKeyPath, targetPath );

// move
    if( rename( sourcePath, targetPath ) == 0 ){
        etStringFree( sourceKeyPath );
        etStringFree( targetKeyPath );
        return true;
    }

    etStringFree( sourceKeyPath );
    etStringFree( targetKeyPath );
    return false;
}


bool sslService::					acceptedKeysGet( json_t* jsonObject ){
    if( jsonObject == NULL ) return false;

// vars
    DIR*			keyDir = NULL;
    dirent*			keyDirEntry = NULL;
    json_t*         jsonKey = NULL;
    const char*     msgPayload = NULL;

// open directory
    keyDir = opendir( sslAcceptedKeyPath );
    if( keyDir == NULL ){
        etDebugMessage( etID_LEVEL_ERR, "Could not open key-directory" );
        return coPlugin::NO_REPLY;
    }

// read every key
    keyDirEntry = readdir( keyDir );
    while( keyDirEntry != NULL ){

    // ignore "." and ".."
        if( keyDirEntry->d_name[0] == '.' ) goto nextKey;

    //
        jsonKey = json_object();
        json_object_set_new( jsonObject, keyDirEntry->d_name, jsonKey );

    nextKey:
        keyDirEntry = readdir( keyDir );
    }
    closedir(keyDir);

    return true;
}


bool sslService::					acceptedKeyRemove( const char* fingerprint ){

// we build the full path
    etString* fullKeyPath; const char* keyPath;
    etStringAllocLen( fullKeyPath, 128 );
    etStringCharSet( fullKeyPath, sslAcceptedKeyPath, -1 );
    etStringCharAdd( fullKeyPath, fingerprint );
    etStringCharGet( fullKeyPath, keyPath );

// remove key
    snprintf( etDebugTempMessage, etDebugTempMessageLen, "Remove '%s'", keyPath );
    etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );
    unlink( keyPath );

// clean
    etStringFree( fullKeyPath );
    return true;
}





void sslService::					serve(){

// start the thread which wait for clients
	pthread_t thread;
	pthread_create( &thread, NULL, sslService::serveThread, this );
	pthread_detach( thread );

}


int sslService::                    serveOnNewPeer( void* userdata ){

// vars
	sslService*     service = (sslService*)userdata;

    return 0;
}


void* sslService::					serveThread( void* void_service ){

// vars
	sslService*             service = (sslService*)void_service;
    const char*             serverHost = NULL;
    int                     serverPort = 0;
    int                     socketChannel;
    int                     socketClientChannel;
    struct sockaddr_in      serverSocketAddress;
    struct sockaddr_in      clientSocketAddress;
    socklen_t               clientSocketAddressLen;
    int                     optval = 1;
    int                     clientHostNameSize = 1024;
    char                    clientHostName[clientHostNameSize];


// get the server infos
    coCore::ptr->config->nodesIterate();
    if( coCore::ptr->config->nodeSelectByHostName(coCore::ptr->hostNameGet()) != true ){
    // debugging message
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "No Server Configuration found, do nothing." );
        etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );

        coCore::ptr->config->nodesIterateFinish();
		return NULL;
	}
    coCore::ptr->config->nodeConnInfo( &serverHost, &serverPort );
    coCore::ptr->config->nodesIterateFinish();

// init server certs
    sslSession::globalServerInit( coCore::ptr->hostNameGet() );


// create address description
    memset( &serverSocketAddress, '\0', sizeof(serverSocketAddress) );
    serverSocketAddress.sin_family = AF_INET;
    serverSocketAddress.sin_addr.s_addr = INADDR_ANY;
    serverSocketAddress.sin_port = htons( serverPort );

// create socket
    socketChannel = socket( AF_INET, SOCK_STREAM, 0 );
    setsockopt( socketChannel, SOL_SOCKET, SO_REUSEADDR, (void*)&optval, sizeof (int) );
    bind( socketChannel, (struct sockaddr*)&serverSocketAddress, sizeof(serverSocketAddress) );
    listen( socketChannel, 1 );
    snprintf( etDebugTempMessage, etDebugTempMessageLen, "Server ready. Listening to port '%d'.\n\n", serverPort );
    etDebugMessage( etID_LEVEL_INFO, etDebugTempMessage );


    clientSocketAddressLen = sizeof( clientSocketAddress );

    for(;;){

    // got client connection ( blocking )
        etDebugMessage( etID_LEVEL_INFO, "Wait for client." );
        socketClientChannel = accept( socketChannel, (struct sockaddr *)&clientSocketAddress, &clientSocketAddressLen );

    // get infos of the peer
        memset( clientHostName, 0, clientHostNameSize );
        if( getnameinfo( (struct sockaddr *)&clientSocketAddress, sizeof(clientSocketAddress), clientHostName, clientHostNameSize, NULL, 0, 0 ) == 0 ){
            snprintf( etDebugTempMessage, etDebugTempMessageLen, "got connection from %s on port %d", clientHostName, ntohs (clientSocketAddress.sin_port) );
            etDebugMessage( etID_LEVEL_INFO, etDebugTempMessage );

        }

    // create a new session
        sslSession* newSession = new sslSession();
        newSession->host( clientHostName );
        newSession->port( ntohs (clientSocketAddress.sin_port) );
        newSession->socketChannel = socketClientChannel;
        newSession->socketChannelAddress = clientSocketAddress;
        newSession->socketChannelAddressLen = sizeof(clientSocketAddress);

    // client communication will be handled inside an thread
        pthread_t thread;
        pthread_create( &thread, NULL, sslService::serverHandleClientThread, newSession );
        pthread_detach( thread );


    }


}


void* sslService::					serverHandleClientThread( void* void_sslSession ){

	sslSession*     session = (sslSession*)void_sslSession;

// client loop
    session->handleClient();

// session finished
    delete session;
}




void sslService::					connectAll(){

// vars
	const char*					clientHost;
	int							clientPort;
	coCoreConfig::nodeType		nodeType;
    struct sockaddr_in          clientSocketAddress;
    socklen_t                   clientSocketAddressLen;
    int                         clientSocket;
    int                         ret;


// start iteration ( and lock core )
	coCore::ptr->config->nodesIterate();

	while( coCore::ptr->config->nodeNext() == true ){

		coCore::ptr->config->nodeInfo(&clientHost,&nodeType);

	// only clients
		if( nodeType != coCoreConfig::CLIENT ){
			continue;
		}

	// get host/port
		if( coCore::ptr->config->nodeConnInfo( &clientHost, &clientPort ) != true ){
			continue;
		}


    // create address description
        memset( &clientSocketAddress, '\0', sizeof(clientSocketAddress) );
        clientSocketAddress.sin_family = AF_INET;
        clientSocketAddress.sin_addr.s_addr = inet_addr( clientHost );
        clientSocketAddress.sin_port = htons( clientPort );      /* Server Port number */


    // create a new session
        sslSession* newSession = new sslSession();
        newSession->host( clientHost );
        newSession->port( clientPort );
        newSession->socketChannelAddress = clientSocketAddress;
        newSession->socketChannelAddressLen = sizeof(clientSocketAddress);

    // client communication will be handled inside an thread
        pthread_t thread;
        pthread_create( &thread, NULL, sslService::connectToClientThread, newSession );
        pthread_detach( thread );



	}

// finish
	coCore::ptr->config->nodesIterateFinish();
}



void* sslService::					connectToClientThread( void* void_session ){

    int             ret = -1;
	sslSession*     session = (sslSession*)void_session;

    for(;;){


    // connect
        session->socketChannel = socket( AF_INET, SOCK_STREAM, 0 );
        ret = connect( session->socketChannel, (const sockaddr*)&session->socketChannelAddress, session->socketChannelAddressLen );
        if( ret != 0 ){
            snprintf( etDebugTempMessage, etDebugTempMessageLen, "Socket connect error: %s", strerror(errno) );
            etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );

            sleep(10);
            continue;
        }

        session->client();
        sleep(10);
    }


}




#endif
