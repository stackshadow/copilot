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

#include "intl.h"

#include "jansson.h"
//#include "doDBDws.h"
//#include "wsPlugins/doDBDPluginList.h"
//#include "wsPlugins/doDBDConnections.h"
#include <getopt.h>

#include "core/etInit.h"

#include "plugins/pubsub.h"
#include "coCore.h"

// plugins
//#include "plugins/qwebsocket.h"
#include "plugins/lsslService.h"

#include "plugins/sslService.h"
#include "plugins/sslSession.h"

#include "plugins/uwebsockets.h"
#include "plugins/coreService.h"
#include "plugins/sysState.h"
//#include "plugins/lxcService.h"
#include "plugins/nftService.h"
#include "plugins/ldapService.h"
//#include "plugins/mqttService.h"
//#include "plugins/ldapService.h"
#include "plugins/syslogd.h"
#include "plugins/mongodb.h"

#ifndef DISABLE_EDB
#include "plugins/eDB.h"
#endif



//#include <QtCore/QCoreApplication>

static struct option options[] = {
    { "help",                   no_argument,        NULL, 1 },
    { "debug",                  no_argument,        NULL, 2 },
    { "debugApp",               no_argument,        NULL, 3 },
    { "debugDB",                no_argument,        NULL, 4 },
    { "debugNet",               no_argument,        NULL, 5 },
    { "nodename",               required_argument,  NULL, 10 },
    { "configpath",             required_argument,  NULL, 11 },
    { "setup",                  no_argument,        NULL, 20 },
    { "listConections",         no_argument,        NULL, 30 },
    { "createConnection",       required_argument,  NULL, 31 },
    { "createServe",            required_argument,  NULL, 32 },
    { "keyReqList",             no_argument,        NULL, 40 },
    { "keyAccept",              required_argument,  NULL, 41 },
    #ifndef DISABLE_WEBSOCKET
    { "websocket",              no_argument,        NULL, 50 },
    #endif
    { "nonft",                  no_argument,        NULL, 60 },


    { NULL, 0, 0, 0 }
};




int main( int argc, char *argv[] ){

// locale
  setlocale (LC_ALL, "");
  bindtextdomain( PACKAGE, LOCALEDIR );
  textdomain( PACKAGE );


	etInit(argc,(const char**)argv);
	etDebugLevelSet( etID_LEVEL_DETAIL_APP );

// we need the pubsub-stuff
	new psBus();

//    QCoreApplication a(argc, argv);


// create the core which contains all services
    coCore*         core = new coCore();


// parse options
    int             optionSelected = 0;

    bool            connectToHost = false;
    const char*     connectToHostName = NULL;
    const char*     connectToPortString = NULL;
    int             connectToPort = 0;

    bool            serveConnection = false;
    const char*     serveHost = NULL;
    const char*     servePortString = NULL;
    int             servePort = 4567;

    bool            reqKeysList = false;
    bool            reqKeyAccept = false;
    bool            reqKeyCler = false;
    const char*     keyName = NULL;

    bool            startWebSocket = false;
    bool            disablenft = false;

    while( optionSelected >= 0 ) {
        optionSelected = getopt_long(argc, argv, "", options, NULL);
        if( optionSelected < 0 )
            continue;

        switch ( optionSelected ) {
            case '?':
                exit(1);

            case 1:
                fprintf( stdout, _("Usage: %s\n"), argv[0] );
                fprintf( stdout, _("--help: Show this help\n") );
				fprintf( stdout, _("--debug: Enable debug messages\n") );
                fprintf( stdout, _("--debugApp: Enable debug APP messages\n") );
				fprintf( stdout, _("--debugDB: Enable debug of querys ( WARNING, CAN CONTAIN SENSITIVE DATA, LIKE PASSWORDS )\n") );
                fprintf( stdout, _("--debugNet: Enable network debug messages\n") );
                fprintf( stdout, _("--nodename <nodename>: Set the nodename ( if you dont set the nodename, your hostname will be used )\n") );
                fprintf( stdout, "--configpath <path>: Path where all keys and config will be saved ( default to /etc/copilot )\n" );
                fprintf( stdout, "--setup: Run setup of all plugins. \n" );
                fprintf( stdout, "--listConections: List all connections \n" );
                fprintf( stdout, "--createConnection <hostname:port>: Create a new client connection and exit \n" );
                fprintf( stdout, "--createServe <hostname:port>: Create a new server connection and exist \n" );
                fprintf( stdout, "--keyReqList List keys which request an connection \n" );
                fprintf( stdout, "--keyAccept <name> Accept an requested key \n" );
                #ifndef DISABLE_WEBSOCKET
                fprintf( stdout, "--websocket: start websocket-server \n" );
                #endif
                #ifndef DISABLE_NFT
                fprintf( stdout, "--nonft: Dont apply nft-rules on startup \n" );
                #endif
                exit(1);

            case 2:
                etDebugLevelSet( etID_LEVEL_DETAIL );
                break;

            case 3:
                etDebugLevelSet( etID_LEVEL_DETAIL_APP );
                break;
                
            case 4:
                etDebugLevelSet( etID_LEVEL_DETAIL_DB );
                break;

            case 5:
                etDebugLevelSet( etID_LEVEL_DETAIL_NET );
                break;

            case 10:
                printf ("Set nodename to '%s'\n", optarg);
                core->nodeName( optarg );
                break;

            case 11:
                printf ("Set config path to '%s'\n", optarg);
                core->config->configPath( (const char**)&optarg );
                core->config->load( core->nodeName() );
                break;

            case 30:
                printf ("Not implemented yet\n");
                break;

            case 31:
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

            case 32:
                printf ("We serve '%s'\n", optarg);

                serveHost = strtok( optarg, ":" );
                servePortString = strtok(NULL, ":");

                if( serveHost == NULL || servePortString == NULL ){
                    fprintf( stderr, "Wrong command line passed" );
                    exit(-1);
                }
                servePort = atoi(servePortString);
                serveConnection = true;

                break;

            case 40:
                reqKeysList = true;
                break;

            case 41:
                reqKeyAccept = true;
                keyName = optarg;
                break;

            #ifndef DISABLE_WEBSOCKET
            case 50:
                printf ("Start Websocket server\n" );
                startWebSocket = true;
                break;
            #endif

            #ifndef DISABLE_NFT
            case 60:
                printf ("We dont apply nft-rules\n" );
                disablenft = true;
                break;
            #endif

			case 20:
				coCore::setupMode = true;
				break;

		}


    }



// ssl service
#ifndef DISABLE_SSL
    sslService* ssl = new sslService();

    if( connectToHost == true ){
        if( coCore::ptr->config->nodeSelectByHostName(connectToHostName) != true ){
            coCore::ptr->config->nodeAppend(connectToHostName);
        }

        coCoreConfig::nodeType connectNodeType = coCoreConfig::CLIENT;
        coCore::ptr->config->nodeInfo( &connectToHostName, &connectNodeType, true );
        coCore::ptr->config->nodeConnInfo( &connectToHostName, &connectToPort, true );
        coCore::ptr->config->save();
        exit(0);
    }

    if( serveConnection == true ){
        if( coCore::ptr->config->nodeSelectByHostName(serveHost) != true ){
            coCore::ptr->config->nodeAppend(serveHost);
        }

        coCoreConfig::nodeType serveNodeType = coCoreConfig::SERVER;
        coCore::ptr->config->nodeInfo( &serveHost, &serveNodeType, true );
        coCore::ptr->config->nodeConnInfo( &serveHost, &servePort, true );
        coCore::ptr->config->save();
        exit(0);
    }

// list keys
    if( reqKeysList == true ){
        json_t* jsonReqKeyList = json_object();
        ssl->reqKeysGet( jsonReqKeyList );
        json_dumpf( jsonReqKeyList, stdout, JSON_PRESERVE_ORDER | JSON_INDENT(4) );
        json_decref( jsonReqKeyList );
        fprintf( stdout, "\n" );
        fflush(stdout);
        exit(0);
    }

// accept key
    if( reqKeyAccept == true ){
        if( ssl->reqKeyAccept( keyName ) == true ){
            fprintf( stdout, "Key accepted: '%s'\n", keyName );
        } else {
            fprintf( stdout, "Key not accepted: '%s'\n", keyName );
        }
        fflush(stdout);
        exit(0);
    }



    ssl->serve();
    ssl->connectAll();
#endif



#ifndef DISABLE_LTLS

    lsslService* tls = new lsslService();

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
        uwebsocket*      wsPlugin = new uwebsocket( 3333 );
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


#ifndef DISABLE_SYSTEMD
    syslogd* syslogdService = new syslogd();
#endif

#ifndef DISABLE_SYSSTATE
    sysState*   state = new sysState();
#endif


#ifndef DISABLE_EDB
	new eDB();
#endif


#ifndef DISABLE_MDB
    new mongodb();

#endif


// Setup All plugins
	//core->plugins->setupAll();

// execute all plugins
	//core->plugins->executeAll();


    //coCore::ptr->plugins->messageQueue->add( NULL, "test", "all", "", "ping", "" );


// because the most plugins run own threads we need a "busy" thread
    core->mainLoop();

// cleanup
    delete core;




    etDebugLevelSet( etID_LEVEL_CRITICAL );
    etDebugMessage( etID_LEVEL_WARNING, "Test" );
}



