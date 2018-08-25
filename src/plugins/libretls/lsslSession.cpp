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

#ifndef lsslSession_C
#define lsslSession_C


#include "lsslSession.h"
//#include "pubsub.h"
#include "uuid/uuid.h"

#include "core/plugin.h"


#include <sys/types.h>
#include <sys/socket.h>
#include <sys/random.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>

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
    json_object_set_new( jsonObject, "s", json_string( myNodeName ) );

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
void* lsslSession::             waitForClientThread( void* void_threadListItem ){

// vars
    threadListItem_t*       threadListItem = (threadListItem_t*)void_threadListItem;
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
    etThreadListUserdataGet( threadListItem, (void**)&session );


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

    while( etThreadCancelRequestActive(threadListItem) == etID_NO ){

        etDebugMessage( etID_LEVEL_INFO, "Wait for client." );
        clientSocket = accept( serverSocket, (struct sockaddr *)&clientSocketAddress, &clientSocketAddressLen );
        if( tls_accept_socket( tlsServer, &tlsClient, clientSocket ) != 0 ){
            snprintf( etDebugTempMessage, etDebugTempMessageLen, "Error: %s", tls_error(tlsServer) );
            etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
            return NULL;
        }

    // perform a handshake to recieve peer cert
        tls_handshake( tlsClient );


    // cool, accepted, we create an client
        lsslSession* sslClient = new lsslSession( NULL, NULL, tlsClient, "", 0 );
        etThreadListAdd( session->threadListClients, "ssl-incoming", lsslSession::communicateThread, NULL, sslClient );



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
void* lsslSession::             connectToClientThread( void* void_threadListItem ){

// vars
    threadListItem_t*       threadListItem = (threadListItem_t*)void_threadListItem;
    int                     rc = 0;
    lsslSession*            sessionInst = NULL;
    struct sockaddr_in      clientSocketAddress;
    socklen_t               clientSocketAddressLen;
    int                     clientSocket;


// get
    etThreadListUserdataGet( threadListItem, (void**)&sessionInst );



// create address description
    memset( &clientSocketAddress, '\0', sizeof(clientSocketAddress) );
    clientSocketAddress.sin_family = AF_INET;
    clientSocketAddress.sin_addr.s_addr = inet_addr( sessionInst->hostName.c_str() );
    clientSocketAddress.sin_port = htons( sessionInst->hostPort );      /* Server Port number */


    while( etThreadCancelRequestActive(threadListItem) == etID_NO ){
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
        lsslSession::communicateThread( void_threadListItem );

    // finished with communication
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

    return NULL;
}



// static helper
/**
@return If jsonMessage was correctly readed
 - -2 If an TLS error occured
 - -1 connection was closed
 -  0 If no valid Message readed
 -  1 Message available
*/
int lsslSession::               communicateReadJson( char* buffer, size_t bufferSize, json_t** p_jsonMessage ){

// vars
    int                     ret = -1;
    json_error_t            jsonError;
    json_t*                 jsonMessage = NULL;

// remove current json-message
    if( *p_jsonMessage != NULL ){
        json_decref(*p_jsonMessage);
        *p_jsonMessage = NULL;
    }

// clear buffer
    memset( buffer, 0, bufferSize + 1 );

// read from connection
    ret = tls_read( this->tlsConnection, buffer, MAX_BUF );

// Error
    if( ret < 0 ){
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "Error: %s", tls_error(this->tlsConnection) );
        etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
        return -2;
    }

// nodata
    if( ret == 0 ){
        etDebugMessage( etID_LEVEL_ERR, "No Data readed, we close the connection." );
        return -1;
    }



// ######################################## Parse JSON ########################################

// try to parse the data into json
    jsonMessage = json_loads( (const char*)buffer, JSON_PRESERVE_ORDER, &jsonError );
    if( jsonMessage == NULL || jsonError.line >= 0 ){
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "JSON ERROR: %s", jsonError.text );
        etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
        return 0;
    }


// parse json to values
    const char *nodeSource;
    psBus::fromJson( jsonMessage, NULL, &nodeSource, NULL, NULL, NULL, NULL );


// an external message should not come from us
    if( coCore::ptr->isNodeName( nodeSource ) == true ){
        etDebugMessage( etID_LEVEL_WARNING,
        "We recieve a message from another host with our hostname as source. Message will be dropped" );

        json_decref(jsonMessage);
        return 0;
    }


    *p_jsonMessage = jsonMessage;
    return 1;
}


bool lsslSession::              communicateNodeNameHandshake(){

// var
    ssize_t                 ret = -1;
    const char*             myNodeName = coCore::ptr->nodeName();
    json_t*                 jsonMessage = NULL;
    char                    msgBuffer[MAX_BUF + 1];
    const char*             msgID;
    const char*             msgNodeSource;
    const char*             msgNodeTarget;
    const char*             msgGroup;
    const char*             msgCmd;
    const char*             msgPayload;
    uuid_t                  UUID;
    char                    UUIDString[37];

// send nodeNameGet
    uuid_generate( UUID );
    uuid_unparse( UUID, UUIDString );

    psBus::toJson( &jsonMessage, UUIDString, myNodeName, "unknown", "ssl", "nodeNameGet", "" );
    this->sendJson( jsonMessage );
    json_decref(jsonMessage); jsonMessage = NULL;


    while(1){


    // we wait for nodeNameGet
        ret = this->communicateReadJson( msgBuffer, MAX_BUF, &jsonMessage );
        if( ret != 1 ) return false;
        if( psBus::fromJson( jsonMessage, &msgID, &msgNodeSource, &msgNodeTarget, &msgGroup, &msgCmd, &msgPayload ) == false ) return false;


    // request nodeName
        if( coCore::strIsExact(msgCmd,"nodeNameGet",11) == true ){

        // send my nodeName
            uuid_generate( UUID ); uuid_unparse( UUID, UUIDString );
            psBus::toJson( &jsonMessage, UUIDString, myNodeName, "unknown", "ssl", "nodeName", myNodeName );
            this->sendJson( jsonMessage );
            json_decref(jsonMessage); jsonMessage = NULL;

        }

    // nodeName
        if( coCore::strIsExact(msgCmd,"nodeName",8) == true ){

            snprintf( etDebugTempMessage, etDebugTempMessageLen, "Peer told us her name. It is %s", msgPayload );
            etDebugMessage( etID_LEVEL_DETAIL_BUS, etDebugTempMessage );

        // check it
            ret = lsslService::checkIfKeyIsAccepted( tls_peer_cert_hash( this->tlsConnection ), msgPayload );
            if( ret != HASH_ACCEPTED ) break;

        // save peer-node-name
            this->peerNodeName = msgPayload;


            goto onSuccess;
        }

    // spend some cpu-time
        usleep( 5000 );
    }

onSuccess:
    if( jsonMessage != NULL ){
        json_decref(jsonMessage);
    }
    return true;

onFalse:
    if( jsonMessage != NULL ){
        json_decref(jsonMessage);
    }
    return false;
}


bool lsslSession::              communicateAuthHandshake(){


// var
    int                     ret = -1;
    const char*             myNodeName = coCore::ptr->nodeName();
    json_t*                 jsonMessage = NULL;
    char                    msgBuffer[MAX_BUF + 1];
    const char*             msgID;
    const char*             msgNodeSource;
    const char*             msgNodeTarget;
    const char*             msgGroup;
    const char*             msgCmd;
    const char*             msgPayload;
    uuid_t                  UUID;
    char                    UUIDString[37];
    const char*             sharedSecret = NULL;
    std::string             tempString;



// ############################################# get sharedSecret #############################################

// try to get shared secret
#ifndef DEVELOP
    ret = lsslService::sharedSecretGet( tls_peer_cert_hash( this->tlsConnection ), &sharedSecret );
    if( ret == SECRET_CREATED ){
        uuid_generate( UUID ); uuid_unparse( UUID, UUIDString );
        psBus::toJson(
            &jsonMessage, UUIDString,
            myNodeName, this->peerNodeName.c_str(),
            "ssl", "sharedSecretSet", sharedSecret
        );
        this->sendJson( jsonMessage );
        json_decref(jsonMessage); jsonMessage = NULL;
    }
    if( ret == SECRET_MISSING ){
        goto onError;
    }
#else
    etDebugMessage( etID_LEVEL_WARNING, "WE USE AN FIXED SHARED SECRET, THIS IS INSECURE !!!" );
    sharedSecret = "fixedSharedSecret";
#endif

// ############################################# send challange #############################################
#ifndef DEVELOP
    this->authChallange = lsslService::randomBase64( 32 );
#else
    etDebugMessage( etID_LEVEL_WARNING, "WE USE AN FIXED CHALLANGE, THIS IS INSECURE !!!" );
    this->authChallange = "fixedchallange";
#endif
    snprintf( etDebugTempMessage, etDebugTempMessageLen, "We create a new random as challange %s", this->authChallange.c_str() );
    etDebugMessage( etID_LEVEL_DETAIL_BUS, etDebugTempMessage );

// reqChallange
    uuid_generate( UUID ); uuid_unparse( UUID, UUIDString );
    psBus::toJson(
        &jsonMessage, UUIDString,
        myNodeName, this->peerNodeName.c_str(),
        "ssl", "reqChallange", this->authChallange.c_str()
    );
    this->sendJson( jsonMessage );
    json_decref(jsonMessage); jsonMessage = NULL;



// ############################################# challange #############################################

    while(1){

    // we wait for nodeNameGet
        ret = this->communicateReadJson( msgBuffer, MAX_BUF, &jsonMessage );
        if( ret != 1 ) goto onError;
        if( psBus::fromJson( jsonMessage, &msgID, &msgNodeSource, &msgNodeTarget, &msgGroup, &msgCmd, &msgPayload ) == false ) return false;


    // req of shared secret set
        if( coCore::strIsExact( msgCmd, "sharedSecretSet", 15 ) == true ){
#ifndef DEVELOP
            ret = lsslService::sharedSecretSet( this->peerNodeName.c_str(), msgPayload );
            if( ret != SECRET_CREATED ) break;
            goto onError;
#else
            etDebugMessage( etID_LEVEL_WARNING, "In develop-mode, we can not save shared secrets" );
            continue;
#endif
        }

    // check if we have a hallange random
        if( coCore::strIsExact( msgCmd, "reqChallange", 12 ) == true ){

        // create unhashed secret
            tempString = "";
            tempString += sharedSecret;
            tempString += msgPayload;

        // get the hash and answer
            tempString = lsslService::sha256( tempString );

        // send it back
            psBus::toJson( &jsonMessage, msgID, msgNodeTarget, msgNodeSource, msgGroup, "resChallange", tempString.c_str() );
            this->sendJson( jsonMessage );
            json_decref(jsonMessage); jsonMessage = NULL;

            continue;
        }

    // we respond an challange, compare it
        if( coCore::strIsExact( msgCmd, "resChallange", 12 ) == true ){

            tempString = "";
            tempString += sharedSecret;
            tempString += this->authChallange;

        // get the hash and answer
            tempString = lsslService::sha256( tempString );

#ifdef DEVELOP
            snprintf( etDebugTempMessage, etDebugTempMessageLen, "Expected hash: '%s', it is '%s'", tempString, msgPayload );
            etDebugMessage( etID_LEVEL_WARNING, etDebugTempMessage );
#endif
        // compare it
            if( tempString == msgPayload ){
                this->authenticated = true;
                goto onSuccess;
            }

            this->authenticated = false;
            goto onError;
        }

    // spend some cpu-time
        usleep( 5000 );
    }



onSuccess:
    if( jsonMessage != NULL ){
        json_decref(jsonMessage);
    }
    return true;

onError:
    if( jsonMessage != NULL ){
        json_decref(jsonMessage);
    }
    return false;
}


void* lsslSession::             communicateThread( void* void_threadListItem ){


// vars
    threadListItem_t*       threadListItem = (threadListItem_t*)void_threadListItem;
    lsslSession*            sessionInst = NULL;
    ssize_t                 ret = -1;
    char                    buffer[MAX_BUF + 1];

    json_t*                 jsonMessage = NULL;
    const char*             msgID;
    const char*             msgNodeSource;
    const char*             msgNodeTarget;
    const char*             msgGroup;
    const char*             msgCmd;
    const char*             msgPayload;



// get session
    etThreadListUserdataGet( threadListItem, (void**)&sessionInst );



// check hash
    ret = lsslService::checkIfKeyIsAccepted( tls_peer_cert_hash( sessionInst->tlsConnection ), NULL );
    if( ret != HASH_ACCEPTED ) goto doDisconnect;

// nodeName handshake
    if( sessionInst->communicateNodeNameHandshake() == false ){
        goto doDisconnect;
    }

// authentication handshake
    if( sessionInst->communicateAuthHandshake() == false ){
        goto doDisconnect;
    }


// send count
    psBus::publish(
        sessionInst, "",
        coCore::ptr->nodeName(), "websocket",
        "ssl", "newConnectedClient", sessionInst->peerNodeName.c_str()
    );


// we can now subscibe for remote messages
    psBus::subscribe( sessionInst, sessionInst->peerNodeName.c_str(), NULL, sessionInst, NULL, lsslSession::onSubscriberJsonMessage );


// ######################################## Read Loop ########################################

    while( etThreadCancelRequestActive(threadListItem) == etID_NO ){
        usleep( 5000 );

    // read json from tls
        ret = sessionInst->communicateReadJson( buffer, MAX_BUF, &jsonMessage );
        if( ret < 0 ) goto doDisconnect;
        if( ret == 0 ) continue;


    // parse json to values
        psBus::fromJson( jsonMessage, &msgID, &msgNodeSource, &msgNodeTarget, &msgGroup, &msgCmd, &msgPayload );



    // ######################################## publish ########################################
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "Received %s", buffer );
        etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );

        psBus::publish( sessionInst, msgID, msgNodeSource, msgNodeTarget, msgGroup, msgCmd, msgPayload );

    }

doDisconnect:
    if( jsonMessage != NULL ){
        json_decref( jsonMessage );
        jsonMessage = NULL;
    }

// finished with communication
    tls_close( sessionInst->tlsConnection );
    tls_free( sessionInst->tlsConnection );

    usleep( 5000 );
    return NULL;
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