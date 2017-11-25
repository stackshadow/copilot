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

/* test:
gnutls-cli --no-ca-verification \
--x509keyfile=/etc/copilot/services/ssl_my_keys/hacktop7.mnet.local-key.pem \
--x509certfile=/etc/copilot/services/ssl_my_keys/hacktop7.mnet.local-cert.pem  \
localhost:4567

{ "id":"", "s":"testnode", "t":"all", "g":"cocom", "c":"ping", "v": "" }
*/

#include "jansson.h"
//#include "doDBDws.h"
//#include "wsPlugins/doDBDPluginList.h"
//#include "wsPlugins/doDBDConnections.h"
#include <getopt.h>

#include "core/etInit.h"

#include "coCore.h"

// plugins
//#include "plugins/qwebsocket.h"
#include "plugins/sslService.h"
#include "plugins/sslSession.h"
#include "plugins/websocket.h"
#include "plugins/coreService.h"
//#include "plugins/sysState.h"
//#include "plugins/lxcService.h"
#include "plugins/nftService.h"
#include "plugins/ldapService.h"
//#include "plugins/mqttService.h"
//#include "plugins/ldapService.h"

//#include <QtCore/QCoreApplication>

static struct option options[] = {
    { "help",       no_argument,        NULL, 'h' },
    { "debug",      no_argument,        NULL, 'd' },
    { "hostname",   required_argument,  NULL, 'n' },
	{ "setup",      no_argument,        NULL, 's' },
    { "connect",    required_argument,  NULL, 'c' },
    #ifndef DISABLE_WEBSOCKET
    { "websocket",  no_argument,        NULL, 'w' },
    #endif
    { "nonft",      no_argument,        NULL, 'a' },


    { NULL, 0, 0, 0 }
};


#include <libssh/libssh.h>
#include <libssh/server.h>
#include <libssh/callbacks.h>




int main( int argc, char *argv[] ){



    etInit(argc,(const char**)argv);
    etDebugLevelSet( etID_LEVEL_WARNING );
    etDebugLevelSet( etID_LEVEL_DETAIL_APP );
//    QCoreApplication a(argc, argv);


// create the core which contains all services
    coCore*         core = new coCore();



// parse options
    int             optionSelected = 0;
    bool            connectToHost = false;
    const char*     connectToHostName = NULL;
    const char*     connectToPortString = NULL;
    int             connectToPort = 0;
    bool            startWebSocket = false;
    bool            disablenft = false;

    while( optionSelected >= 0 ) {
        optionSelected = getopt_long(argc, argv, "", options, NULL);
        if( optionSelected < 0 )
            continue;

        switch ( optionSelected ) {
            case '?':
                exit(1);

            case 'd':
                etDebugLevelSet( etID_LEVEL_DETAIL_APP );
                break;

            case 'h':
                fprintf( stdout, "Usage: %s\n", argv[0] );
                fprintf( stdout, "--help: Show this help\n" );
				fprintf( stdout, "--debug: Enable debug messages\n" );
                fprintf( stdout, "--hostname <hostname>: Set the hostname ( not detect it automatically )\n" );
                fprintf( stdout, "--setup: Run setup of all plugins. \n" );
                fprintf( stdout, "--connect <hostname:port>: Connect to an copilotd-server \n" );
                #ifndef DISABLE_WEBSOCKET
                fprintf( stdout, "--websocket: start websocket-server \n" );
                #endif
                #ifndef DISABLE_NFT
                fprintf( stdout, "--nonft: Dont apply nft-rules on startup \n" );
                #endif
                exit(1);

            case 'n':
                printf ("Set hostname to '%s'\n", optarg);
                core->setHostName( optarg );
                break;

            case 'c':
                printf ("Try to connect to '%s'\n", optarg);

                connectToHostName = strtok( optarg, ":" );
                connectToPortString = strtok(NULL, ":");

                if( connectToHostName == NULL || connectToPortString == NULL ){
                    fprintf( stderr, "Wrong command line passed" );
                    exit(-1);
                }
                connectToPort = atoi(connectToPortString);
                connectToHost = true;

                break;

            #ifndef DISABLE_WEBSOCKET
            case 'w':
                printf ("Start Websocket server\n" );
                startWebSocket = true;
                break;
            #endif

            #ifndef DISABLE_NFT
            case 'a':
                printf ("We dont apply nft-rules\n" );
                disablenft = true;
                break;
            #endif

			case 's':
				coCore::setupMode = true;
				break;

		}


    }



// ssl service
#ifndef DISABLE_WOLFSSL
    sslService* ssl = new sslService();
    ssl->serve();
    ssl->connectAll();
#endif


// create services
    coreService* newCoreService = new coreService();

// sshd-service
#ifndef DISABLE_SSH
	ssh_threads_set_callbacks( ssh_threads_get_pthread() );
	ssh_init();
	sshService* newSshService = new sshService();

    if( connectToHost == true ){
        newSshService->connect( connectToHostName, connectToPort );
    }
#endif



// websocket-service
#ifndef DISABLE_WEBSOCKET
    if( startWebSocket == true ){
        websocket*      wsPlugin = new websocket( 3000 );
    }
#endif


// nft
#ifndef DISABLE_NFT
    nftService*      nftPlugin = new nftService();
    if( disablenft == false ){
        nftPlugin->applyRules();
    }

#endif

// ldap
#ifndef DISABLE_LDAP
    ldapService*    ldapPlugin = new ldapService();
#endif


// Setup All plugins
	core->plugins->setupAll();

// execute all plugins
	core->plugins->executeAll();


    //coCore::ptr->plugins->messageQueue->add( NULL, "test", "all", "", "ping", "" );


// because the most plugins run own threads we need a "busy" thread
    core->mainLoop();

// cleanup
    delete core;





    etDebugMessage( etID_LEVEL_WARNING, "Test" );
}



