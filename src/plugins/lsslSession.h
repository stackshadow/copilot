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

#ifndef lsslSession_H
#define lsslSession_H

#include "../libs/portable-2.7.4/include/tls.h"

#include "coCoreConfig.h"
#include "core/threadList.h"

/**
@defgroup lsslService TLS-Service
@detail
This service provide an copilot to copilot connection. \n
The connection is TLS-Encrypted with the power of libressl ( the libretls-library is super easy to use :D )


openssl s_server -cert /etc/copilot/ssl_private/hacktop7.crt -key /etc/copilot/ssl_private/hacktop7.key  -accept 5555
{"id":"sendMyNodeName","s":"testssl","t":"hacktop7","g":"co","c":"nodeName","v":"testssl"}

{"id":"requestRunningClients","s":"testssl","t":"hacktop7","g":"ssl","c":"connectedClientsGet","v":""}



*/

#include "lsslService.h"

#define MAX_BUF 20480
#define HANDSHAKE_ERROR -1
#define HANDSHAKE_NEXT 0
#define HANDSHAKE_OK 1

class lsslSession {
        private:
        threadList_t*       threadListClients = NULL;       // server: so we need to remember clients
        std::string         hostName;                       // server / client
        int                 hostPort;                       // server / client
        tls_config*         tlsConfig;                      // server / client
        struct tls*         tlsConnection = NULL;
        std::string         sha256;
        std::string         peerNodeName = "";
        std::string         authChallange = "";
        bool                authenticated = false;

    public:
                            lsslSession( threadList_t* threadListClients, tls_config* tlsConfig, struct tls* tlsConnection, const char* host, int port );
                                                          // incoming-client-constructor
                            ~lsslSession();

    const char*             peerNodeNameGet(){ return this->peerNodeName.c_str(); };
    bool                    sendJson( json_t* jsonObject );


    static void*            waitForClientThread( void* void_lsslService );
    static void*            connectToClientThread( void* void_lsslService );

    int                     communicateReadJson( char* buffer, size_t bufferSize, json_t** p_jsonMessage );
    bool                    communicateNodeNameHandshake();
    bool                    communicateAuthHandshake();
    static void*            communicateThread( void* void_lsslService );


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
    static int              onSubscriberJsonMessage( void* objectSource, json_t* jsonObject, void* userdata );
};

#endif