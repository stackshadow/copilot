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


sslService::               			sslService() : coPlugin( "sslService", coCore::ptr->nodeName(), "cocom" ){

// init keys
    sslSession::globalInit( coCore::ptr->nodeName() );



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
        coCore::ptr->config->nodeSelectByHostName(coCore::ptr->nodeName());
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
        coCore::ptr->plugins->messageQueue->add( this,
        coCore::ptr->nodeName(), msgSource,
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
        coCore::ptr->plugins->messageQueue->add( this,
        coCore::ptr->nodeName(), msgSource,
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
	fprintf( stdout, "Please choose:\n" );
	fprintf( stdout, "1.) Create an listener to accept Connection from another node \n" );
	fprintf( stdout, "2.) Creata a connection to another node \n" );
    fprintf( stdout, "9.) Finished, do nothing \n" );
    fprintf( stdout, "What would you like to do ? (1/2/9) \n" );
	fflush( stdout );
	read( STDIN_FILENO, answer, 512 );

// wrong answer
	if( answer[0] != '1' && answer[0] != '2' && answer[0] != '9' ) goto ask;

// do nothing
	if( answer[0] == '9' ){
        return true;
    }


// lock
    coCore::ptr->config->nodesIterate();

// default server / port
    answer[0] = ':'; answer[1] = ':'; answer[2] = '\0';
    serverPort = 4567;

// server or client ?
    if( answer[0] == '1' ){
        serverType = coCoreConfig::SERVER;
    }
    if( answer[0] == '2' ){
        serverType = coCoreConfig::CLIENT;
    }

// read port
    fprintf( stdout, "Enter port ( Default is 4567, must have min 4 digits ): " );
    fflush( stdout );
    read( STDIN_FILENO, answer, 512 );
    fprintf( stdout, "\n" );

    answerLen = strlen( answer );
    if( answerLen > 3 && answer != NULL ){
        serverPort = atoi(answer);
    }


// read hostname
    if( answer[0] == '1' ){
        fprintf( stdout, "Enter host/IP to bind. ( user :: for IPv6 and IPv4 ) " );
    }
    if( answer[0] == '2' ){
        fprintf( stdout, "Enter host/IP to connect to." );
    }
    fflush( stdout );
    read( STDIN_FILENO, answer, 512 );
    fprintf( stdout, "\n" );
// remove 0
    answerLen = strlen(answer);
    if( answerLen > 0 ) answerLen--;
    answer[answerLen] = '\0';



// save
    if( coCore::ptr->config->nodeSelectByHostName( answer ) == false ){
        if( coCore::ptr->config->nodeAppend( answer ) == false ){
            coCore::ptr->config->nodesIterateFinish();
            return false;
        }
    }
    coCore::ptr->config->nodeInfo( NULL, &serverType, true );
    coCore::ptr->config->nodeConnInfo( (const char**)&pAnswer, &serverPort, true );
    coCore::ptr->config->nodesIterateFinish();

// save it
    coCore::ptr->config->save();


    goto ask;
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
    const char*         sslKeyPath = NULL;
    DIR*			    keyDir = NULL;
    dirent*			    keyDirEntry = NULL;
    unsigned int        keyDirCounter = 0;



// get accepted key path
    etStringCharGet( sslSession::pathRequestedKeys, sslKeyPath );


// open directory
    keyDir = opendir( sslKeyPath );
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
    const char*     sslKeyPath = NULL;
    DIR*			keyDir = NULL;
    dirent*			keyDirEntry = NULL;
    json_t*         jsonKey = NULL;
    const char*     msgPayload = NULL;


// get accepted key path
    etStringCharGet( sslSession::pathRequestedKeys, sslKeyPath );


// open directory
    keyDir = opendir( sslKeyPath );
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

// vars
    const char*     sslKeyPath = NULL;

// get accepted key path
    etStringCharGet( sslSession::pathRequestedKeys, sslKeyPath );


// we build the full path
    etString* fullKeyPath;
    etStringAllocLen( fullKeyPath, 128 );
    etStringCharSet( fullKeyPath, sslKeyPath, -1 );
    etStringCharSet( fullKeyPath, "/", -1 );
    etStringCharAdd( fullKeyPath, fingerprint );
    etStringCharGet( fullKeyPath, sslKeyPath );

// remove key
    snprintf( etDebugTempMessage, etDebugTempMessageLen, "Remove '%s'", sslKeyPath );
    etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );
    unlink( sslKeyPath );

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
    etStringCharGet( sslSession::pathRequestedKeys, sourcePath );
    etStringAllocLen( sourceKeyPath, 128 );
    etStringCharSet( sourceKeyPath, sourcePath, -1 );
    etStringCharSet( sourceKeyPath, "/", -1 );
    etStringCharAdd( sourceKeyPath, fingerprint );
    etStringCharGet( sourceKeyPath, sourcePath );

// target
    etStringCharGet( sslSession::pathAcceptedKeys, targetPath );
    etStringAllocLen( targetKeyPath, 128 );
    etStringCharSet( targetKeyPath, targetPath, -1 );
    etStringCharSet( targetKeyPath, "/", -1 );
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
    const char*     sslKeyPath = NULL;
    DIR*			keyDir = NULL;
    dirent*			keyDirEntry = NULL;
    json_t*         jsonKey = NULL;
    const char*     msgPayload = NULL;

// get accepted key path
    etStringCharGet( sslSession::pathAcceptedKeys, sslKeyPath );


// open directory
    keyDir = opendir( sslKeyPath );
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

// vars
    const char*     sslKeyPath = NULL;

// get accepted key path
    etStringCharGet( sslSession::pathAcceptedKeys, sslKeyPath );

// we build the full path
    etString* fullKeyPath;
    etStringAllocLen( fullKeyPath, 128 );
    etStringCharSet( fullKeyPath, sslKeyPath, -1 );
    etStringCharAdd( fullKeyPath, "/" );
    etStringCharAdd( fullKeyPath, fingerprint );
    etStringCharGet( fullKeyPath, sslKeyPath );

// remove key
    snprintf( etDebugTempMessage, etDebugTempMessageLen, "Remove '%s'", sslKeyPath );
    etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );
    unlink( sslKeyPath );

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
    if( coCore::ptr->config->nodeSelectByHostName(coCore::ptr->nodeName()) != true ){
    // debugging message
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "No Server Configuration found, do nothing." );
        etDebugMessage( etID_LEVEL_WARNING, etDebugTempMessage );

        coCore::ptr->config->nodesIterateFinish();
		return NULL;
	}
    coCore::ptr->config->nodeConnInfo( &serverHost, &serverPort );
    coCore::ptr->config->nodesIterateFinish();

// init server certs
    sslSession::globalServerInit( coCore::ptr->nodeName() );


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

    // send it out that we have an new client
        coCore::ptr->plugins->messageQueue->add(
            newSession, coCore::ptr->nodeName(), coCore::ptr->nodeName(),
            "cocom", "onNewInClient", clientHostName
        );

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

// send it out that we have an new client
    coCore::ptr->plugins->messageQueue->add(
        session, coCore::ptr->nodeName(), coCore::ptr->nodeName(),
        "cocom", "onDisconnectClient", session->host()
    );

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




    /*
        if( getnameinfo( (struct sockaddr *)&clientSocketAddress, sizeof(clientSocketAddress), clientHostName, clientHostNameSize, NULL, 0, 0 ) == 0 ){
            snprintf( etDebugTempMessage, etDebugTempMessageLen, "got connection from %s on port %d", clientHostName, ntohs (clientSocketAddress.sin_port) );
            etDebugMessage( etID_LEVEL_INFO, etDebugTempMessage );
        }
    */
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


// in_addr_t
    // setup address
//        memset( &session->socketChannelAddress, '\0', sizeof(session->socketChannelAddress) );
//        session->socketChannelAddress.sin_family = AF_INET;
        //session->socketChannelAddress.sin_addr.s_addr = inet_addr( clientHost );
        //session->socketChannelAddress.sin_port = htons( clientPort );      /* Server Port number */

    // connect
//        session->socketChannel = socket( AF_INET, SOCK_STREAM, 0 );
//        ret = connect( session->socketChannel, (const sockaddr*)&session->socketChannelAddress, session->socketChannelAddressLen );




    // get ip-address
        struct addrinfo*    clientHostAddrInfo;
        const char*         hostName = session->host();
        int                 hostPort = session->port(-1);
        char                hostPortString[10];
        snprintf( hostPortString, 10, "%i", hostPort );

        ret = getaddrinfo( hostName, hostPortString, NULL, &clientHostAddrInfo );
        if( ret != 0 ){
            snprintf( etDebugTempMessage, etDebugTempMessageLen, "Could not get address of %s: %s", session->host(), gai_strerror(ret) );
            etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );

            sleep(10);
            continue;
        }



    // from getaddrinfo
        session->socketChannel = socket( clientHostAddrInfo->ai_family, clientHostAddrInfo->ai_socktype, clientHostAddrInfo->ai_protocol );
        ret = connect( session->socketChannel, clientHostAddrInfo->ai_addr, clientHostAddrInfo->ai_addrlen );
        if( ret != 0 ){

            char* inetAddress;

            inetAddress = inet_ntoa( session->socketChannelAddress.sin_addr ),

            snprintf( etDebugTempMessage, etDebugTempMessageLen, "Could not connect to %s ( %s ) on %i: %s", session->host(), inetAddress, session->port(-1), strerror(errno) );
            etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );

            sleep(10);
            continue;
        }

        session->client();

        close( session->socketChannel );
        sleep(10);
    }


}





#endif
