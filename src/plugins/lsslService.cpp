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
#include <sys/random.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sstream>

#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/sha.h>


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

    psBus::inst->subscribe( this, coCore::ptr->nodeName(), "ssl", this, lsslService::onSubscriberMessage, NULL );
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


bool lsslService::              toBase64( unsigned char* sourceData, size_t size, unsigned char** p_base64String, size_t* p_base64Size ){

//vars
    unsigned char*      base64String;
    size_t              base64StringLen = size;
    size_t              base64StringLenReal;
    size_t              base64StringSize;


// allocate
    base64StringSize = base64StringLen * 2 + 10 + 2 + 1;
    base64String = (unsigned char*)malloc( base64StringSize );
    memset( base64String, 0, base64StringSize );

// encode
    base64StringLenReal = EVP_EncodeBlock( base64String, sourceData, size );

// return
    *p_base64String = base64String;
    *p_base64Size = base64StringLenReal;
    return true;
}


bool lsslService::              fromBase64( unsigned char* base64, unsigned char** p_plaintext, size_t* plaintextSize ){

//vars
    unsigned char*      plaintextString;
    size_t              plaintextStringLen = strlen((const char*)base64);
    size_t              plaintextStringLenReal;
    size_t              plaintextStringSize = plaintextStringLen * sizeof(char);

// allocate ciphertext
    plaintextString = (unsigned char*)malloc( plaintextStringSize );
    memset( plaintextString, 0 , plaintextStringSize );



// base64 -> plain
    plaintextStringLenReal = EVP_DecodeBlock( plaintextString, base64, plaintextStringLen );

    *p_plaintext = plaintextString;
    *plaintextSize = plaintextStringLenReal;
    return true;
}


std::string lsslService::       randomBase64( size_t size ){

// vars
    unsigned char       randomBuffer[size];
    char*               randomBase64 = NULL;
    size_t              randomBase64Size = 0;

    size_t              base64StringSize = size * 2 + 10 + 2 + 1;
    unsigned char       base64String[base64StringSize];
    size_t              base64StringLen = size;
    size_t              base64StringLenReal;



// get from urandom
    ssize_t randomReaded = getrandom( &randomBuffer, size, 0 );


// encode
    base64StringLenReal = EVP_EncodeBlock( base64String, randomBuffer, size );

    std::string returnString = "";
    returnString.assign( (char*)base64String );
    return returnString;
}


std::string lsslService::       sha256( const std::string input ){

// vars
    unsigned char   hash[SHA256_DIGEST_LENGTH];
    char            outputBuffer[ ( 2 * SHA256_DIGEST_LENGTH ) + 2 ];
    SHA256_CTX      sha256;


    SHA256_Init( &sha256 );
    SHA256_Update( &sha256, input.c_str(), input.size() );
    SHA256_Final( hash, &sha256 );


    int i = 0;
    for(i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        sprintf(outputBuffer + (i * 2), "%02x", hash[i]);
    }

    outputBuffer[64] = 0;

    return outputBuffer;
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


bool lsslService::              requestedKeysGet( json_t** jsonObject ){

// vars
    json_t* jsonRequested = NULL;

// accepted ?
    jsonRequested = coCore::ptr->config->section( "ssl_requested" );
    if( jsonRequested == NULL ){
        return false;
    }

    *jsonObject = json_deep_copy( jsonRequested );
    return true;
}


bool lsslService::              acceptKeyOfNodeName( const char* nodeName ){

// vars
    json_t* jsonNode = NULL;
    json_t* jsonHash = NULL;
    json_t* jsonRequested = NULL;

// get node
    coCore::ptr->config->nodesIterate();
    if( coCore::ptr->config->nodeSelect( nodeName ) == false ) return false;
    if( coCore::ptr->config->nodeGet( &jsonNode ) == false ) return false;


// get requested hash
    jsonRequested = coCore::ptr->config->section( "ssl_requested" );
    jsonHash = json_object_get( jsonRequested, nodeName );
    if( jsonHash != NULL ){

        json_object_set( jsonNode, "tlsHash", jsonHash );
        json_object_del( jsonRequested, nodeName );

        coCore::ptr->config->nodesIterateFinish();

        return true;
    }


    return false;
}


bool lsslService::              removeKeyOfNodeName( const char* nodeName ){

}



bool lsslService::              checkIfKeyIsAccepted( const char* peerNodeName, const char* hash ){

// vars
    json_t* jsonNode = NULL;
    json_t* jsonHash = NULL;
    json_t* jsonRequested = NULL;

// lock
    coCore::ptr->config->nodesIterate();

// get nodeName
    if( coCore::ptr->config->nodeSelect( peerNodeName ) == false ){
        coCore::ptr->config->nodeAppend( peerNodeName );
    }
    if( coCore::ptr->config->nodeGet( &jsonNode ) == false ){
        coCore::ptr->config->nodesIterateFinish();
        return false;
    }

// get hash
    jsonHash = json_object_get( jsonNode, "tlsHash" );
    if( jsonHash == NULL ){

    // debug
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "Node has no hash, we save %s as hash for nodeName %s", hash, peerNodeName );
        etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );

    // not accepted, save it to requested keys
        jsonRequested = coCore::ptr->config->section( "ssl_requested" );
        json_object_set_new( jsonRequested, peerNodeName, json_string( hash ) );
        coCore::ptr->config->nodesIterateFinish();

    // save it
        coCore::ptr->config->save();

        return false;
    }

// compare
    if( coCore::strIsExact( hash, json_string_value(jsonHash), json_string_length(jsonHash) ) == true ){
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "Accept hash '%s' for nodeName '%s'", hash, peerNodeName );
        etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );

        coCore::ptr->config->nodesIterateFinish();
        return true;
    }

    snprintf( etDebugTempMessage, etDebugTempMessageLen, "Denied hash '%s' for nodeName '%s'", hash, peerNodeName );
    etDebugMessage( etID_LEVEL_ERR, etDebugTempMessage );
    return false;


}


int lsslService::               sharedSecretGet( const char* peerNodeName, const char** secret ){

// vars
    json_t*                     jsonPeerNode = NULL;


// lock
    coCore::ptr->config->nodesIterate();

// get nodeName
    if( coCore::ptr->config->nodeSelect( peerNodeName ) == false ){
        coCore::ptr->config->nodeAppend( peerNodeName );
    }
    if( coCore::ptr->config->nodeGet( &jsonPeerNode ) == false ){
        coCore::ptr->config->nodesIterateFinish();
        return -1;
    }


// secret
    json_t*                     jsonTLSSecret = NULL;

    jsonTLSSecret = json_object_get( jsonPeerNode, "tlsSecret" );
    if( jsonTLSSecret == NULL ){
        snprintf( etDebugTempMessage, etDebugTempMessageLen, "No secret aviable for node '%s'.", peerNodeName );
        etDebugMessage( etID_LEVEL_WARNING, etDebugTempMessage );
        coCore::ptr->config->nodesIterateFinish();
        return 0;
    }


    snprintf( etDebugTempMessage, etDebugTempMessageLen, "Secret aviable for node '%s'.", peerNodeName );
    etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );
    *secret = json_string_value( jsonTLSSecret );
    coCore::ptr->config->nodesIterateFinish();
    return 1;
}


int lsslService::               sharedSecretSet( const char* peerNodeName, const char* secret ){

// vars
    json_t*                     jsonPeerNode = NULL;


// lock
    coCore::ptr->config->nodesIterate();

// get nodeName
    if( coCore::ptr->config->nodeSelect( peerNodeName ) == false ){
        coCore::ptr->config->nodeAppend( peerNodeName );
    }
    if( coCore::ptr->config->nodeGet( &jsonPeerNode ) == false ){
        coCore::ptr->config->nodesIterateFinish();
        return -1;
    }


// secret
    json_t*                     jsonTLSSecret = NULL;

    jsonTLSSecret = json_object_get( jsonPeerNode, "tlsSecret" );
    if( jsonTLSSecret == NULL ){

        snprintf( etDebugTempMessage, etDebugTempMessageLen, "Set Secret of node '%s'.", peerNodeName );
        etDebugMessage( etID_LEVEL_DETAIL_APP, etDebugTempMessage );

        json_object_set_new( jsonPeerNode, "tlsSecret", json_string(secret) );
        coCore::ptr->config->nodesIterateFinish();

        coCore::ptr->config->save();
        return 1;
    }

    return 0;
}





// server / client

void lsslService::              serve(){


// vars
    coCoreConfig::nodeType  serverType = coCoreConfig::UNKNOWN;
    const char*             serverHost = NULL;
    int                     serverPort = 0;


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




// infos

bool lsslService::              runningClientsGet( json_t* object ){
    etThreadListIterate( this->threadListClients, lsslService::threadIterationAddServiceName, object );
};


void* lsslService::             threadIterationAddServiceName( threadListItem_t* threadListItem, void* jsonObject ){

// vars
    lsslSession*            session = NULL;

// get instance
    etThreadListUserdataGet( threadListItem, (void**)&session );

    json_object_set_new( (json_t*)jsonObject, session->peerNodeNameGet(), json_object() );

    return NULL;
}



// bus

int lsslService::               onSubscriberMessage(
                                    void* objectSource,
                                    const char* id,
                                    const char* nodeSource,
                                    const char* nodeTarget,
                                    const char* group,
                                    const char* command,
                                    const char* payload,
                                    void* userdata
                                ){

// vars
    lsslService*            service = (lsslService*)objectSource;


    if( coCore::strIsExact( command, "requestedKeysGet", 16 ) == true ){

    // vars
        json_t*     jsonClientsObject = NULL;

    // get requested keys
        service->requestedKeysGet( &jsonClientsObject );

    // dump json
        const char* jsonDump = json_dumps( jsonClientsObject, JSON_PRESERVE_ORDER | JSON_INDENT(4) );

    // add the message to list
        psBus::inst->publish( service, id, nodeTarget, nodeSource, group, "requestKeys", jsonDump );

    // cleanup and return
        free((void*)jsonDump);
        json_decref( jsonClientsObject );

    // finished
        return psBus::END;
    }


    if( coCore::strIsExact( command, "connectedClientsGet", 19 ) == true ){

    // vars
        json_t*     jsonClientsObject = json_object();
        void*       jsonIterator = NULL;

    // get running clients as json
        service->runningClientsGet( jsonClientsObject );

    // send for every client
        jsonIterator = json_object_iter( jsonClientsObject );
        while( jsonIterator != NULL ){

            const char* connectedClientNodeName = json_object_iter_key( jsonIterator );

            psBus::inst->publish( service, id, nodeTarget, nodeSource, group, "connectedClient", connectedClientNodeName );

            jsonIterator = json_object_iter_next( jsonClientsObject, jsonIterator );
        }

    // cleanup and return
        json_decref( jsonClientsObject );

    // finished
        return psBus::END;
    }







    return psBus::END;
}




#endif