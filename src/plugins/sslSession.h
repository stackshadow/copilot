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

#ifndef sslSession_H
#define sslSession_H

#include <stdio.h>
#include <pthread.h>
#include <sys/sysinfo.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "memory/etList.h"

#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include <gnutls/abstract.h>

#include "coPlugin.h"
#include "coCoreConfig.h"

// functions for ALL sessions
bool sslSessionInit();
bool sslServerInit( const char* serverName );

#define hostNameBufferSize 1024
#define sslServerKeyPath baseFilePath "ssl_server_keys/"
#define sslKeyReqPath baseFilePath "ssl_req_keys/"
#define sslAcceptedKeyPath baseFilePath "ssl_accepted_keys/"
#define sslClientKeyPath baseFilePath "ssl_client_keys/"
#define sslMyKeyPath baseFilePath "ssl_my_keys/"

class sslSession : public coPlugin {

    public:
        typedef enum {
            UNKNOW              = 0,
            CONNECTING          = 1,
            DISCONNECTED        = 10,       /**< connection is established */
            CONNECTED_OUT       = 20,       /**< connected to external host */
            CONNECTED_INC       = 21,       /**< incoming connection */
            REQ_DISCONNECT      = 30,       /**< Request disconnect */
        } state_t;


    // callbacks
        typedef int sslSessionServerOnNewPeerCallback( void* userdata );

    private:
        static gnutls_priority_t priorityCache;
        static gnutls_certificate_credentials_t myCerts;
        static gnutls_certificate_credentials_t clientCerts;

        sslSession::state_t                 sessionState = UNKNOW;
		etString*			                sessionHost = NULL;
        int                                 sessionPort = 4567;

    public:
        int                                 socketChannel;
        struct sockaddr_in                  socketChannelAddress;
        socklen_t                           socketChannelAddressLen;
        sslSessionServerOnNewPeerCallback*  newPeerCallbackFunct = NULL;
        void*                               userdata;

// public functions
	public:
                            sslSession();
                            ~sslSession();

//
    public:
        static bool         globalInit(  const char* myHostName  );
        static bool         globalServerInit( const char* serverName );


// static functions ( for all sessions )
    public:
// key handling
        static bool         import( const char* filename, gnutls_privkey_t privateKey );
        static bool         import( const char* filename, gnutls_pubkey_t  publicKey );

        static bool         generateKeyPair( const char* name, const char* folder );

        static bool         credCreate( gnutls_certificate_credentials_t* xcred, const char* name, const char* folder, gnutls_certificate_verify_function* func );
        static bool         pubKeyGetId( gnutls_pubkey_t publicKey, char* outBuffer, size_t* outBufferSize );
        static bool         checkAcceptedKey( gnutls_pubkey_t publicKey, const char* peerHostName, bool pinning );

// callbakcs
        static int          verifyPublikKeyOnServerCallback( gnutls_session_t session ) { return sslSession::verifyPublikKeyCallback( session, false ); };    // no pinning
        static int          verifyPublikKeyOnClientCallback( gnutls_session_t session ) { return sslSession::verifyPublikKeyCallback( session, true ); };    // with pinning
        static int          verifyPublikKeyCallback( gnutls_session_t session, bool pinning );


// instance-functions
    public:
        int                 port( int port );
        const char*         host( const char* hostName = NULL );

        bool                certInfo( const char* name );



// internal functions
    private:


// API
    public:
/*
		coPlugin::t_state	onBroadcastMessage( coMessage* message );
		bool 				onSetup();
		bool				onExecute();
*/
// server
        bool                handleClient();

// client
        bool                client();
        static void*		clientSessionThread( void* void_service );


};









#endif