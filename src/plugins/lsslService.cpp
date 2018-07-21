/*
Copyright (C) 2018 by Martin Langlotz

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

#ifndef lsslService_C
#define lsslService_C

#include "coCore.h"

#include "lsslService.h"
#include "pubsub.h"
#include "uuid/uuid.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

/** @ingroup lsslService
@brief This class provide the TLS-Service based on libressl
*/
lsslService::                   lsslService(){

// alloc thread list
    etThreadListAlloc( &this->threadListServer );
    etThreadListAlloc( &this->threadListClients );


// we create a key-pair if needed
    this->generateKeyPair();


// we init tls-stuff
    tls_init();
    this->createTLSConfig();

// create server
    this->serve();

// connect to client
    this->connectToAllClients();
}


lsslService::                   ~lsslService(){

}




void lsslService::              createTLSConfig(){

// vars
    std::string     tempString;
    const char*     configPath = NULL;
    const char*     nodeName = NULL;

// get node name
    coCore::ptr->config->configPath( &configPath );
    coCore::ptr->config->myNodeName( &nodeName );


// init tls
    this->tlsConfig = tls_config_new();


// protocolls
    unsigned int protocols = 0;
    if( tls_config_parse_protocols(&protocols, "secure") < 0 ){
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "Error: %s", tls_config_error(this->tlsConfig) );
        etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
    }
    tls_config_set_protocols(this->tlsConfig, protocols);


// ciphers
    const char *ciphers = "ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES256-SHA384:ECDHE-ECDSA-AES256-SHA384";
    if(tls_config_set_ciphers(this->tlsConfig, ciphers) < 0) {
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "Error: %s", tls_config_error(this->tlsConfig) );
        etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
    }


// set key
    tempString =  configPath;
    tempString += "/ssl_private/";
    tempString += nodeName;
    tempString += ".key";
    if( tls_config_set_key_file( this->tlsConfig, tempString.c_str() ) != 0 ){
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "Error: %s", tls_config_error(this->tlsConfig) );
        etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
    }

// set cert
    tempString =  configPath;
    tempString += "/ssl_private/";
    tempString += nodeName;
    tempString += ".crt";
    if( tls_config_set_cert_file( this->tlsConfig, tempString.c_str() ) != 0 ){
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "Error: %s", tls_config_error(this->tlsConfig) );
        etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
    }

// server only
    tls_config_verify_client( this->tlsConfig );
    tls_config_insecure_noverifycert( this->tlsConfig );
    tls_config_insecure_noverifyname( this->tlsConfig );
    tls_config_insecure_noverifytime( this->tlsConfig );


}




bool lsslService::              generateKeyPair(){

// command
    int             rc = 1;
    std::string     command;
    const char*     configPath = NULL;
    const char*     nodeName = NULL;
    std::string     privFileNamePrefix;
    std::string     privKeyPass;
    std::string     privKey;
    std::string     privCSR;
    std::string     privCRT;

// get node name
    coCore::ptr->config->configPath( &configPath );
    coCore::ptr->config->myNodeName( &nodeName );

//
    privFileNamePrefix =  configPath;
    privFileNamePrefix += "/";
    privFileNamePrefix += nodeName;

    privKeyPass = privFileNamePrefix + ".pass.key";
    privKey = privFileNamePrefix + ".key";
    privCSR = privFileNamePrefix + ".csr";
    privCRT = privFileNamePrefix + ".crt";



// generate keyfile with password
    command =  "openssl genrsa -des3 -passout pass:x -out ";
    command += privKeyPass;
    if( access( privKeyPass.c_str(), F_OK ) != 0 ){
        rc = system( command.c_str() );
        if( rc != 0 ){
            snprintf( etDebugTempMessage, etDebugTempMessageLen, "Failed to create key." );
            etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
            return false;
        }
    }


// remove password from key
    command =  "openssl rsa -passin pass:x -in ";
    command += privKeyPass;
    command += " -out ";
    command += privKey;
    if( access( privKey.c_str(), F_OK ) != 0 ){
        rc = system( command.c_str() );
        if( rc != 0 ){
            snprintf( etDebugTempMessage, etDebugTempMessageLen, "Failed to create key." );
            etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
            return false;
        }
    }


// create key-signing-request
    command =  "openssl req -new -subj '/CN=";
    command += nodeName;
    command += "/C=US/ST=Ohio/L=Columbus/O=Widgets Inc/OU=Some Unit' -key ";
    command += privKey;
    command += " -out ";
    command += privCSR;
    if( access( privCSR.c_str(), F_OK ) != 0 ){
        rc = system( command.c_str() );
        if( rc != 0 ){
            snprintf( etDebugTempMessage, etDebugTempMessageLen, "Failed to create key." );
            etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
            return false;
        }
    }


    command =  "openssl x509 -req -sha256 -days 365 -in ";
    command += privCSR;
    command += " -signkey ";
    command += privKey;
    command += " -out ";
    command += privCRT;
    if( access( privCRT.c_str(), F_OK ) != 0 ){
        rc = system( command.c_str() );
        if( rc != 0 ){
            snprintf( etDebugTempMessage, etDebugTempMessageLen, "Failed to create key." );
            etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
            return false;
        }
    }
// openssl x509 -req -sha256 -days 365 -in server.csr -signkey server.key -out server.crt

    return true;
}


bool lsslService::              checkIfKeyIsAccepted( const char* hash ){

// vars
    json_t* jsonAccepted = NULL;
    json_t* jsonRequested = NULL;


// accepted ?
    jsonAccepted = coCore::ptr->config->section( "ssl_accepted" );
    if( json_object_get(jsonAccepted,hash) != NULL ){
        return true;
    }

// requested
    jsonRequested = coCore::ptr->config->section( "ssl_requested" );
    json_object_set_new( jsonRequested, hash, json_object() );

    coCore::ptr->config->save(); // is thread-save

    return false;
}


bool lsslService::              checkNodeNameOfHash( const char* hash, const char* nodeName ){
    
// vars
    json_t* jsonAccepted = NULL;
    json_t* jsonHash = NULL;
    json_t* jsonString = NULL;

// get accepted-section
    jsonAccepted = coCore::ptr->config->section( "ssl_accepted" );
    jsonHash = json_object_get(jsonAccepted,hash);
    if( jsonHash == NULL ){
        return false;
    }

// get name
    jsonString = json_object_get( jsonHash, "nodeName" );
    if( jsonString == NULL ){
        jsonString = json_string( nodeName );
        json_object_set_new( jsonHash, "nodeName", jsonString );
        
        coCore::ptr->config->save(); // is thread-save
        return true;
    }

// compare name
    return coCore::strIsExact( json_string_value(jsonString), nodeName, strlen(nodeName) );

}





void lsslService::              serve(){
    

// vars
    coCoreConfig::nodeType  serverType = coCoreConfig::UNKNOWN;
    const char*             serverHost = NULL;
    int                     serverPort = 0;

    int                     serverSocket;
    struct sockaddr_in      serverSocketAddress;
    int                     clientSocket;
    struct sockaddr_in      clientSocketAddress;
    socklen_t               clientSocketAddressLen;
    int                     optval = 1;

    struct tls*             tlsServer = NULL;
    struct tls*             tlsClient = NULL;



// get the server infos
    coCore::ptr->config->nodesIterate();
    if( coCore::ptr->config->nodeSelect(coCore::ptr->nodeName()) != true ){
    // debugging message
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "No Config for node, do nothing." );
        etDebugMessage( etID_LEVEL_WARNING, etDebugTempMessage );

        coCore::ptr->config->nodesIterateFinish();
        return;
    }
    coCore::ptr->config->nodeInfo( NULL, &serverType, false );
    if( serverType != coCoreConfig::SERVER ){
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "Node has no Server-Config, do nothing." );
        etDebugMessage( etID_LEVEL_WARNING, etDebugTempMessage );

        coCore::ptr->config->nodesIterateFinish();
        return;
    }


// get config
    coCore::ptr->config->nodeConnInfo( &serverHost, &serverPort );
    coCore::ptr->config->nodesIterateFinish();


// start server-thread
    lsslSession* sslClient = new lsslSession( this->threadListClients, this->tlsConfig, NULL, serverHost, serverPort );
    etThreadListAdd( this->threadListServer, "ssl-server", lsslSession::waitForClientThread, NULL, sslClient );

}


void lsslService::              connectToAllClients(){

// vars
    const char*                 clientHost;
    int                         clientPort;
    coCoreConfig::nodeType      nodeType;
    struct sockaddr_in          clientSocketAddress;
    socklen_t                   clientSocketAddressLen;
    int                         clientSocket;
    int                         ret;


// start iteration ( and lock core )
    coCore::ptr->config->nodesIterate();
    while( coCore::ptr->config->nodeNext() == true ){

    // get type
        coCore::ptr->config->nodeInfo(&clientHost,&nodeType);

    // only clients
        if( nodeType != coCoreConfig::CLIENT ){
            continue;
        }

    // get host/port
        if( coCore::ptr->config->nodeConnInfo( &clientHost, &clientPort ) != true ){
            continue;
        }

    // new client
        lsslSession* sslClient = new lsslSession( NULL, this->tlsConfig, NULL, clientHost, clientPort );
    // we create a client thread
        etThreadListAdd( this->threadListClients, "ssl-client", lsslSession::connectToClientThread, NULL, sslClient );




    }

// finish
    coCore::ptr->config->nodesIterateFinish();

}













/**
 * @brief Constructor for the TLS-Server, it just will save the provided parameter
 * @param threadListClients An pointer to an already created etThreadList. This is mainly for Servers which are waiting for clients and append new client-handling-threads to the threadlist
 * @param tlsConfig The TLS-Configuration
 * @param tlsConnection An already established TLS-Connection
 * @param host The hostname
 * @param port The Port
 * @return An new lsslSession instance
 * @detail Some of the parameter are optional. @see waitForClientThread() @see connectToClientThread() or @see communicateThread() for detail informations.
 */
lsslSession::                   lsslSession( threadList_t* threadListClients, tls_config* tlsConfig, struct tls* tlsConnection, const char* host, int port ){

// save parameter
    this->threadListClients = threadListClients;
    this->tlsConfig = tlsConfig;
    this->tlsConnection = tlsConnection;
    this->hostName = host;
    this->hostPort = port;




}



bool lsslSession::              sendJson( json_t* jsonObject ){

// vars
    const char*     myNodeName = coCore::ptr->nodeName();
    const char*     msgID = NULL;
    const char*     msgSource = NULL;
    const char*     msgTarget = NULL;
    const char*     msgGroup = NULL;
    const char*     msgCommand = NULL;
    char*           jsonString = NULL;



// get source/target from json
    psBus::fromJson( jsonObject, &msgID, &msgSource, &msgTarget, &msgGroup, &msgCommand, NULL );

// if there is a message for myHost we dont need to send it out to the world
    if( strncmp(msgTarget,myNodeName,strlen(myNodeName)) == 0 ){
        return false;
    }


// make sure we are the source
    json_object_set_new( jsonObject, "s", json_string( coCore::ptr->nodeName() ) );

// dump / send json
    jsonString = json_dumps( jsonObject, JSON_PRESERVE_ORDER | JSON_COMPACT );
    if( jsonString != NULL ){

    // debug
        snprintf( etDebugTempMessage, etDebugTempMessageLen,
        "[SEND] [%s -> %s] [%s - %s]",
        msgSource, msgTarget,
        msgGroup, msgCommand );
        etDebugMessage( etID_LEVEL_DETAIL_NET, etDebugTempMessage );

    // send it out
        tls_write( this->tlsConnection, jsonString, strlen(jsonString) );

    // cleanup
        free( jsonString );

    }

    return true;
}


/**
 * @brief Wait for an client
 * @param threadListItem
 * @detail This is an Server thread which wait for new clients that connect to us.
 * This function needs the following parameter from the constructor:
 * - threadListClients
 * - tlsConfig
 * - host
 * - port
 * tlsConnection can be set to NULL
 */
void* lsslSession::             waitForClientThread( void* threadListItem ){

// vars
	lsslSession*            session = NULL;
    struct sockaddr_in      serverSocketAddress;
    int                     serverSocket;
    
    int                     clientSocket;
    struct sockaddr_in      clientSocketAddress;
    socklen_t               clientSocketAddressLen;

    int                     optval = 1;

    struct tls*             tlsServer = NULL;
    struct tls*             tlsClient = NULL;

// get instance
    etThreadListUserdataGet( (threadListItem_t*)threadListItem, (void**)&session );


// create address description
    memset( &serverSocketAddress, '\0', sizeof(serverSocketAddress) );
    serverSocketAddress.sin_family = AF_INET;
    serverSocketAddress.sin_addr.s_addr = INADDR_ANY;
    serverSocketAddress.sin_port = htons( session->hostPort );

// create socket
    serverSocket = socket( AF_INET, SOCK_STREAM, 0 );
    setsockopt( serverSocket, SOL_SOCKET, SO_REUSEADDR, (void*)&optval, sizeof (int) );
    bind( serverSocket, (struct sockaddr*)&serverSocketAddress, sizeof(serverSocketAddress) );
    listen( serverSocket, 1 );
    snprintf( etDebugTempMessage, etDebugTempMessageLen, "Server ready. Listening to port '%d'.\n\n", session->hostPort );
    etDebugMessage( etID_LEVEL_INFO, etDebugTempMessage );


// tls server
    tlsServer = tls_server();

    if(tls_configure( tlsServer, session->tlsConfig ) < 0 ){
        printf("tls_configure error: %s\n", tls_error(tlsServer));
        exit(1);
    }

    while( etThreadListCancelRequestActive((threadListItem_t*)threadListItem) == etID_NO ){

        etDebugMessage( etID_LEVEL_INFO, "Wait for client." );
        clientSocket = accept( serverSocket, (struct sockaddr *)&clientSocketAddress, &clientSocketAddressLen );
        if( tls_accept_socket( tlsServer, &tlsClient, clientSocket ) != 0 ){
            snprintf( etDebugTempMessage, etDebugTempMessageLen, "Error: %s", tls_error(tlsServer) );
            etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
            return NULL;
        }

    // perform a handshake to recieve peer cert
        tls_handshake( tlsClient );

    // hash peer cert
        const char* hash = tls_peer_cert_hash( tlsClient );

    // check if key is accepted
        if( lsslService::checkIfKeyIsAccepted( hash ) == false ){
            snprintf( etDebugTempMessage, etDebugTempMessageLen, " [%s] Key not accepted, close connection", hash );
            etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );

            tls_close( tlsClient );
            tls_free( tlsClient );
            close( clientSocket );
        }

    // cool, accepted, we create an client
        lsslSession* sslClient = new lsslSession( NULL, NULL, tlsClient, NULL, 0 );
        etThreadListAdd( session->threadListClients, "ssl-client", lsslSession::communicateThread, NULL, sslClient );
        
    }



    return NULL;
}

/**
 * @brief Connect to an single client
 * @param void_lsslService
 * @detail This threaded function is used, when to TRY to connect to an client.
 * This function needs the following parameter from the constructor:
 * - tlsConfig
 * - host
 * - port
 * threadListClients can be set to NULL
 * tlsConnection can be set to NULL
 */
void* lsslSession::             connectToClientThread( void* void_lsslService ){

// vars
    int                     rc = 0;
    lsslSession*            sessionInst = NULL;
    struct sockaddr_in      clientSocketAddress;
    socklen_t               clientSocketAddressLen;
    int                     clientSocket;


// get
    etThreadListUserdataGet( (threadListItem_t*)void_lsslService, (void**)&sessionInst );

// create address description
    memset( &clientSocketAddress, '\0', sizeof(clientSocketAddress) );
    clientSocketAddress.sin_family = AF_INET;
    clientSocketAddress.sin_addr.s_addr = inet_addr( sessionInst->hostName.c_str() );
    clientSocketAddress.sin_port = htons( sessionInst->hostPort );      /* Server Port number */


    while(1){
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "Try to connect to client %s:%i", sessionInst->hostName.c_str(), sessionInst->hostPort );
        etDebugMessage( etID_LEVEL_INFO, etDebugTempMessage );


        clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        rc = connect( clientSocket, (struct sockaddr *) &clientSocketAddress, sizeof(clientSocketAddress) );
        if( rc < 0 ){
            snprintf( etDebugTempMessage, etDebugTempMessageLen, 
                "Could not connect to server %s:%i, %s", 
                sessionInst->hostName.c_str(), 
                sessionInst->hostPort,
                strerror( rc ) 
            );
            etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
            
            sleep( 10 );
            continue;
        }



    // tls server
        sessionInst->tlsConnection = tls_client();
        if(tls_configure( sessionInst->tlsConnection, sessionInst->tlsConfig ) < 0 ){
            printf("tls_configure error: %s\n", tls_error(sessionInst->tlsConnection));
            continue;
        }

    // upgrade port to tls
        if( tls_connect_socket( sessionInst->tlsConnection, clientSocket, "localhost" ) < 0 ) {
            printf("tls_connect error\n");
            printf("%s\n", tls_error( sessionInst->tlsConnection ));
            continue;
        }

    // perform a handshake
        tls_handshake( sessionInst->tlsConnection );

    // we communicate
        lsslSession::communicateThread( void_lsslService );
        
    // finished with communication
        tls_close( sessionInst->tlsConnection );
        tls_free( sessionInst->tlsConnection );
        close( clientSocket );

    //@todo Create unsubscribe ... 

        snprintf( etDebugTempMessage, etDebugTempMessageLen, 
            "Client %s:%i disconnected", 
            sessionInst->hostName.c_str(), 
            sessionInst->hostPort
        );
        etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
        
        sleep( 10 );
    }
}


void* lsslSession::             communicateThread( void* void_lsslService ){


// vars
    lsslSession*            sessionInst = NULL;
    ssize_t                 ret = -1;
    char                    buffer[MAX_BUF + 1];
    json_error_t            jsonError;
    json_t*                 jsonMessage = NULL;

// get session
    etThreadListUserdataGet( (threadListItem_t*)void_lsslService, (void**)&sessionInst );


// We subscribe on the remote-node for our node
    json_t*     jsonTempRequest = NULL;
    char        jsonTempRequestString[128];
    uuid_t      newUUID;
    char        newUUIDString[37];



// ######################################## Send nodename request ########################################
    uuid_generate( newUUID );
    uuid_unparse( newUUID, newUUIDString );
    psBus::toJson( &jsonTempRequest, newUUIDString, coCore::ptr->nodeName(), "", "co", "nodeNameGet", "" );
    sessionInst->sendJson( jsonTempRequest );
    json_decref(jsonTempRequest);



// ######################################## Read Loop ########################################

    while(1){

    // clear buffer
        memset( buffer, 0, MAX_BUF + 1 );

    // read from connection
        ret = tls_read( sessionInst->tlsConnection, buffer, MAX_BUF );

    // Error
        if( ret < 0 ){
            snprintf( etDebugTempMessage, etDebugTempMessageLen, "Error: %s", tls_error(sessionInst->tlsConnection) );
            etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
            return NULL;
        }
        
    // nodata
        if( ret == 0 ){
            etDebugMessage( etID_LEVEL_ERR, "No Data readed, we close the connection." );
            return NULL;
        }

    // debug
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "Received %s", buffer );
        etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );


    // ######################################## PARSe JSON ########################################

    // try to parse the data into json
        jsonMessage = json_loads( (const char*)buffer, JSON_PRESERVE_ORDER, &jsonError );
        if( jsonMessage == NULL || jsonError.line >= 0 ){
            snprintf( etDebugTempMessage, etDebugTempMessageLen, "JSON ERROR: %s", jsonError.text );
            etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
            continue;
        }


    // parse json to values
        const char* nodeSource, *nodeID, *nodeCmd, *nodePayload;
        psBus::fromJson( jsonMessage, &nodeID, &nodeSource, NULL, NULL, &nodeCmd, &nodePayload );


    // an external message should not come from us
        if( coCore::ptr->isNodeName( nodeSource ) == true ){
            etDebugMessage( etID_LEVEL_WARNING, "We recieve a message from another host with our hostname as source. Message will be dropped" );
        } else {
            
            
        // ######################################## nodename ########################################
            if( coCore::strIsExact("nodeName",nodeCmd,strlen(nodeCmd)) == true ){
                
                if( coCore::strIsExact(nodePayload,coCore::ptr->nodeName(),strlen(coCore::ptr->nodeName())) == true ){
                    snprintf( etDebugTempMessage, etDebugTempMessageLen, "Peer told us her name. It is %s ... BUT we are %s, close connection !", nodePayload, coCore::ptr->nodeName() );
                    etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
                    json_decref( jsonMessage );
                    return NULL;
                }
                
                snprintf( etDebugTempMessage, etDebugTempMessageLen, "Peer told us her name. It is %s", nodePayload );
                etDebugMessage( etID_LEVEL_DETAIL_BUS, etDebugTempMessage );
                
            // hash to name okay ?
            /// @todo CHECK THIS !
                const char* hash = tls_peer_cert_hash( sessionInst->tlsConnection );
                lsslService::checkNodeNameOfHash( hash, nodePayload );
                
            // save peer-node-name
                sessionInst->peerNodeName = nodePayload;
                
            // we can now subscibe for remote
                psBus::inst->subscribe( sessionInst, nodePayload, NULL, sessionInst, NULL, lsslSession::onSubscriberJsonMessage );
                
                json_decref( jsonMessage );
                usleep( 5000 );
                continue;
            }

        // ######################################## publish ########################################
            psBus::inst->publishOrSubscribe( sessionInst, jsonMessage, NULL, NULL, lsslSession::onSubscriberJsonMessage );

        }


    // cleanup
        json_decref( jsonMessage );

        usleep( 5000 );
    }



}


int lsslSession::               onSubscriberJsonMessage( void* objectSource, json_t* jsonObject, void* userdata ){


// vars
    lsslSession*    sslSessionInst = (lsslSession*)objectSource;

// send it out
    sslSessionInst->sendJson( jsonObject );

// we dont have anything to reply, because the answer comes asynchron
	return 0;


}


#endif