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



#include "jansson.h"
//#include "doDBDws.h"
//#include "wsPlugins/doDBDPluginList.h"
//#include "wsPlugins/doDBDConnections.h"
#include <getopt.h>

#include "core/etInit.h"

#include "coCore.h"

// plugins
//#include "plugins/qwebsocket.h"
#include "plugins/sshService.h"
#include "plugins/websocket.h"
#include "plugins/coreService.h"
//#include "plugins/sysState.h"
//#include "plugins/lxcService.h"
#include "plugins/nftService.h"
//#include "plugins/ldapService.h"
//#include "plugins/mqttService.h"
//#include "plugins/ldapService.h"

//#include <QtCore/QCoreApplication>

static struct option options[] = {
    { "help",       no_argument,        NULL, 'h' },
    { "debug",      no_argument,        NULL, 'd' },
    { "hostname",   required_argument,  NULL, 'n' },
	{ "setup",   no_argument,  NULL, 's' },
    { NULL, 0, 0, 0 }
};


#include <libssh/libssh.h>
#include <libssh/server.h>
#include <libssh/callbacks.h>




int main( int argc, char *argv[] ){



    etInit(argc,(const char**)argv);
    etDebugLevelSet( etID_LEVEL_DETAIL_APP );
//    QCoreApplication a(argc, argv);


// create the core which contains all services
    coCore*         core = new coCore();

// parse options
    int optionSelected = 0;
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
                exit(1);

            case 'n':
                printf ("Set hostname to '%s'\n", optarg);
                core->setHostName( optarg );
                break;

			case 's':
				coCore::setupMode = true;
				break;
		}


    }




// create services
    coreService* newCoreService = new coreService();

// sshd-service
#ifndef DISABLE_SSH
	ssh_threads_set_callbacks( ssh_threads_get_pthread() );
	ssh_init();
	sshService* newSshService = new sshService();
#endif

//	while(1) sleep(1);


/*
// sysState
	sysState*		newSysState = new sysState();

//
	lxcService*		newLxcService = new lxcService();

#ifndef DISABLE_MQTT
    mqttService*    mqqtPlugin = new mqttService();
#endif



#ifndef DISABLE_NFT
    nftService*     newNftService = new nftService();
#endif

#ifndef DISABLE_LDAP
    ldapService*    newLdapService = new ldapService();
#endif
*/

#ifndef DISABLE_WEBSOCKET
    websocket*      wsPlugin = new websocket( 3000 );
#endif



// Setup All plugins
	core->plugins->setupAll();

// execute all plugins
	core->plugins->executeAll();


    core->mainLoop();

// cleanup
    delete core;

/*
	delete newSysState;
	delete newLxcService;

#ifndef DISABLE_MQTT
//	delete mqqtPlugin;
#endif

#ifndef DISABLE_WEBSOCKET
	delete wsPlugin;
#endif

#ifndef DISABLE_NFT
	delete newNftService;
#endif

#ifndef DISABLE_LDAP
	delete newLdapService;
#endif
*/




    etDebugMessage( etID_LEVEL_WARNING, "Test" );
}



