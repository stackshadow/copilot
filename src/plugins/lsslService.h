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

/**
@defgroup lsslService TLS-Service
@detail
This service provide an copilot to copilot connection. \n
The connection is TLS-Encrypted with the power of libressl ( the libretls-library is super easy to use :D )
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

// key handling
    bool                    generateKeyPair();
    static bool             checkIfKeyIsAccepted( const char* hash );
    static bool             checkNodeNameOfHash( const char* hash, const char* nodeName );


// server
    static void*            serverListenThread( void* void_lsslService );
    void                    serve();
    void                    connectToAllClients();


};

class lsslSession {
        private:
        threadList_t*       threadListClients = NULL;       // server: so we need to remember clients
        std::string         hostName;                       // server / client
        int                 hostPort;                       // server / client
        tls_config*         tlsConfig;                      // server / client
        struct tls*         tlsConnection = NULL;           
        std::string         sha256;
        std::string         peerNodeName = "";

    public:
                            lsslSession( threadList_t* threadListClients, tls_config* tlsConfig, struct tls* tlsConnection, const char* host, int port );
                                                          // incoming-client-constructor
                            ~lsslSession();

    bool                    sendJson( json_t* jsonObject );

    static void*            waitForClientThread( void* void_lsslService );
    static void*            connectToClientThread( void* void_lsslService );
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