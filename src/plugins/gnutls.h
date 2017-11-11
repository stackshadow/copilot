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

#ifndef wolfssl_H
#define wolfssl_H

#include <stdio.h>
#include <pthread.h>
#include <sys/sysinfo.h>
#include "memory/etList.h"

#include "coPlugin.h"
#include "coCoreConfig.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <gnutls/gnutls.h>
#include <gnutls/abstract.h>


#define wsslServerKeyPath baseFilePath "ssl_server_keys/"
#define wsslKeyReqPath baseFilePath "ssl_req_keys/"
#define wsslAcceptedKeyPath baseFilePath "ssl_accepted_keys/"
#define wsslClientKeyPath baseFilePath "ssl_client_keys/"

#define hostNameBufferSize 1024

class sslService : public coPlugin {

    public:
        typedef enum {
            UNKNOW              = 0,
            CONNECTING          = 1,
            DISCONNECTED        = 10,       /**< connection is established */
            CONNECTED_OUT       = 20,       /**< connected to external host */
            CONNECTED_INC       = 21,       /**< incoming connection */
            REQ_DISCONNECT      = 30,       /**< Request disconnect */
        } state_t;


    private:
        sslService::state_t                 sessionState = UNKNOW;
		etString*			                sessionHost = NULL;
        int                                 sessionPort = 1111;
        gnutls_certificate_credentials_t    x509Cred;

// public functions
	public:
                            sslService();
                            ~sslService();

        int                 port( int port );
        const char*         host( const char* hostName );

        bool                certInfo( const char* name );

        static bool         pubKeyGetId( gnutls_pubkey_t publicKey, char* outBuffer, size_t* outBufferSize );
        static bool         checkAcceptedKey( gnutls_pubkey_t publicKey, const char* peerHostName, bool pinning );

        static bool         import( const char* filename, gnutls_privkey_t privateKey );
        static bool         import( const char* filename, gnutls_pubkey_t  publicKey );

// callbakcs
        static int          verifyPublikKeyOnServerCallback( gnutls_session_t session ) { return sslService::verifyPublikKeyCallback( session, false ); };    // no pinning
        static int          verifyPublikKeyOnClientCallback( gnutls_session_t session ) { return sslService::verifyPublikKeyCallback( session, true ); };    // with pinning
        static int          verifyPublikKeyCallback( gnutls_session_t session, bool pinning );

// internal functions
    private:
        bool                generateKeyPair( const char* name, const char* folder );
        bool                credCreate( gnutls_certificate_credentials_t* xcred, const char* name, const char* folder, gnutls_certificate_verify_function* func );

// API
    public:
/*
		coPlugin::t_state	onBroadcastMessage( coMessage* message );
		bool 				onSetup();
		bool				onExecute();
*/
// server
        bool                serve();

// client
        bool                client();


};









#endif
