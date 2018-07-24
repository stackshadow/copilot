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

#ifndef lsslService_H
#define lsslService_H

#include "../libs/portable-2.7.4/include/tls.h"

#include "coCoreConfig.h"
#include "core/threadList.h"

#include "lsslSession.h"

/**
@defgroup lsslService TLS-Service
@detail
This service provide an copilot to copilot connection. \n
The connection is TLS-Encrypted with the power of libressl ( the libretls-library is super easy to use :D )


openssl s_server -cert /etc/copilot/ssl_private/hacktop7.crt -key /etc/copilot/ssl_private/hacktop7.key  -accept 5555
{"id":"sendMyNodeName","s":"testssl","t":"hacktop7","g":"co","c":"nodeName","v":"testssl"}

{"id":"requestRunningClients","s":"testssl","t":"hacktop7","g":"ssl","c":"connectedClientsGet","v":""}



*/

#define MAX_BUF 20480

class lsslService {

    private:
        threadList_t*       threadListServer;
        threadList_t*       threadListClients;
        tls_config*         tlsConfig;


// public functions
    public:
                            lsslService();
                            ~lsslService();

// common
    void                    createTLSConfig();
    static bool             toBase64( unsigned char* sourceData, size_t size, unsigned char** p_base64String, size_t* p_base64Size );
    static bool             fromBase64( unsigned char* base64, unsigned char** p_plaintext, size_t* plaintextSize );
    static int              encrypt( unsigned char *plaintext,  unsigned char *key, unsigned char **ciphertextBase64 );
    static int              decrypt( unsigned char *ciphertextBase64, unsigned char *key, unsigned char **plaintext );




// key handling
    bool                    generateKeyPair();
    static bool             checkIfKeyIsAccepted( const char* nodeName, const char* hash );
    static bool             requestedKeysGet( json_t** jsonObject );
    static bool             acceptKeyOfNodeName( const char* nodeName );
    static bool             removeKeyOfNodeName( const char* nodeName );

// server / client
    void                    serve();
    void                    connectToAllClients();

// infos
    bool                    runningClientsGet( json_t* object );
    static void*            threadIterationAddServiceName( threadListItem_t* threadListItem, void* jsonObject );

// bus
    static int              onSubscriberMessage(
                                void* objectSource,
                                const char* id,
                                const char* nodeSource,
                                const char* nodeTarget,
                                const char* group,
                                const char* command,
                                const char* payload,
                                void* userdata
                            );

};


#endif